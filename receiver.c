//RECEIVER
#include "../include/app.h"
#include "../include/protocol_app.h"
#include "../include/constants.h"

//Here we are gonna use llread
FILE *fptr;

int main(int argc, char **argv)
{
    char *package = (char *)malloc(sizeof(char) * MAX_FRAME_SIZE);
    char *frame = (char *)malloc(sizeof(char) * MAX_FRAME_SIZE);
    int fd = 0;
    app_info.status = RECEIVER;

    //Parse arguments
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "") != 0)
        {
            if (!strncmp(argv[i], "/dev/ttyS", 9))
            {

                strcpy(link_info.port, argv[i]);
            }
        }
    }
    fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(argv[1]);
        exit(-1);
    }
    printf("RECEIVER ");
    printf("%d\n", fd);
    app_info.fileDescriptor = fd;

    //Open connection between app and data protocol
    //And connection between TRANSMITTER and RECEIVER
    llopen(link_info.port, app_info.status); //Tested until here, I'm fixing problems with stuff dealing with transmitter.c

    int size;
    int received_ctrl_pack_start = FALSE;
    //Gonna receive a Control Package with START
    while (!received_ctrl_pack_start)
    {
        if ((size = llread(fd, package)) < 0)
        {
            printf("ERROR\n");
            free(package);
            free(frame);
            return 1;
        }
        received_ctrl_pack_start=TRUE;
        sleep(3);/*
        char *filename[MAX_SIZE];
        int file_size;
        if (package[0] == CTRL_START)
        { //okay maybe I should have them in constants
            if (read_control_package(package, filename, &file_size, size) < 0)
            {
                printf("ERROR\n");
                free(package);
                free(frame);
                return 1;
            }
            received_ctrl_pack_start = TRUE;
        }*/
    }
/*
    char *new_file = "new_file.txt"; //Should I ask the receiver to tell me the file name?
    if ((fptr = fopen(new_file, "w")) == NULL)
    {
        printf("ERROR\n");
        free(package);
        free(frame);
        return 1;
    }

    //Gonna receive Data Packages as well and a Control package with END
    int not_end = FALSE;
    while (!not_end)
    {
        if ((size = llread(fd, package)) < 0)
        {
            printf("ERROR\n");
            free(package);
            free(frame);
            return 1;
        }
        if (package[0] == CTRL_DATA)
        {
            if ((size = read_data_package(frame, package)) < 0)
            {
                printf("ERROR\n");
                free(package);
                free(frame);
                return 1;
            }
            fwrite(frame, sizeof(char), size, fptr);
        }
        if (package[0] == CTRL_END)
        {
            int file_size;
            if (read_control_package(package, new_file, &file_size, size) < 0)
            {
                printf("ERROR\n");
                free(package);
                free(frame);
                return 1;
            }
            not_end = TRUE;
        }
    }

    //Close connection
    if (llclose(fd, app_info.status) < 0)
    {
        printf("ERROR\n");
        free(package);
        free(frame);
        return 1;
    }
    free(package);
    free(frame);*/
    return 0;
}