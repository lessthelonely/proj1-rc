//TRANSMITTER
#include "../include/app.h"
#include "../include/constants.h"
#include "../include/protocol_app.h"
#include "../include/alarm.h"

FILE *fprt;
//Here we are gonna use llwrite
int main(int argc, char **argv)
{
    char *package[MAX_FRAME_SIZE]; //should it be larger? Yes-->256*L2+L1-->needs to address this, make it larger
    char *filename[255];
    int fd = 0;
    app_info.status = TRANSMITTER;
    //Parse arguments
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "") != 0)
        {
            if (!strncmp(argv[i], "/dev/ttyS", 9))
            {
                strcpy(link_info.port, argv[i]);
            }
            if (!strcmp(argv[i], "-t"))
            {
                link_info.timeout = atoi(argv[i + 1]);
            }

            if (!strcmp(argv[i], "-n"))
            {
                link_info.numTransmissions = atoi(argv[i + 1]);
            }

            if (!strcmp(argv[i], "-b"))
            {
                link_info.baudRate = atoi(argv[i + 1]);
            }

            if (!strcmp(argv[i], "-f"))
            {
                strcpy(filename, argv[i + 1]);
            }
        }
    }

    struct stat st;
    stat(filename, &st);
    int size = st.st_size; //get file's size

    //Open file
    if ((fprt = fopen(filename, "r")) == NULL)
    { //can I use fopen or should it be open? Also r or rb (read in binary)?
        printf("ERROR: file doesn't exist\n");
        return -1;
    }

    //printf("SIZE %d\n",size); //for pinguim.gif is 10968

    fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(argv[1]);
        exit(-1);
    }
    printf("TRANSMITTER FD ");
    printf("%d\n", fd);
    app_info.fileDescriptor = fd;

    install_alarm();
    //Open connection between app and data protocol
    //And connection between TRANSMITTER and RECEIVER
    llopen(link_info.port, app_info.status);

    //Send control package with START
    int package_size;
    if ((package_size = create_control_package(CTRL_START, filename, size, package)) < 0)
    {
        printf("ERROR\n");
        return 1;
    }
    printf("CONTROL PACKAGE SIZE %d\n",package_size);

    //Until here, everything is alright
    
    int write_length;
    if ((write_length = llwrite(fd, package, package_size)) < 0)
    {
        printf("ERROR\n");
        return 1;
    }
    printf("I came back to transmitter.c\n");
    //Keep sending Data packages until the end of the file
    int c_size = MAX_FRAME_SIZE;
    int n = 0, line_size = 0;
    char *line[c_size];
    char *frame = (char *)malloc(sizeof(char) * MAX_FRAME_SIZE);

    while (TRUE)
    {
        if (size - n * c_size < c_size)
        {
            c_size = size % c_size;
        }
        if ((line_size = fread(line, 1, c_size, fprt) <= 0))
        { //could be an error or could be EOF
            break;
        }

        if (create_data_package(n, line_size, line, frame) < 0)
        {
            printf("ERROR\n");
            free(frame);
            return 1;
        }

        int frame_size = line_size + 4;
        if (llwrite(fd, frame, frame_size) < 0)
        {
            printf("ERROR\n");
            free(frame);
            return 1;
        }

        n++;
    }

    //Send control package with END
    if ((package_size = create_control_package(CTRL_END, filename, size, package)) < 0)
    { //again should I put the control stuff in constants?
        printf("ERROR\n");
        free(frame);
        return 1;
    }
    if ((write_length = llwrite(fd, package, package_size)) < 0)
    {
        printf("ERROR\n");
        free(frame);
        return 1;
    }

    //Close connection
    if (llclose(fd, app_info.status) < 0)
    {
        printf("ERROR\n");
        free(frame);
        return 1;
    }
    free(frame);
    return 0;
}
