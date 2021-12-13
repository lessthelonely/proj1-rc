//TRANSMITTER
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "app.h"
#include "constants.h"
#include "protocol_app.h"
#include "alarm.h"

// ./transmitter /dev/ttySx -t TIMEOUT -n numTries -f filename

int main(int argc, char **argv)
{
    u_int8_t *package[MAX_FRAME_SIZE]; //it should be bigger than 255 because K=256*L2+L1
    link_info.status = TRANSMITTER;
    int index_t=-1,index_n=-1,index_f=-1;
    FILE *fprt;

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
                index_t = i+1;
            }

            if (!strcmp(argv[i], "-n"))
            {
                link_info.numTransmissions = atoi(argv[i + 1]);
                index_n = i+1;
            }

            if (!strcmp(argv[i], "-b"))
            {
                link_info.baudRate = atoi(argv[i + 1]);
            }

            if (!strcmp(argv[i], "-f"))
            {
                index_f=i+1;
            }
        }
    }

    if(index_f < 0){
        printf("ERROR: Please introduce a file so we can start the transmission process\n");
    }
    u_int8_t * filename= argv[index_f];
    
    if(index_n < 0){
        link_info.numTransmissions = 3; //default value
    }

    if(index_t < 0){
        link_info.timeout = 3; //default 
    }
    
    struct stat st;
    stat(filename, &st);
    int size = st.st_size; //get file's size

    //Open file
    if ((fprt = fopen(filename, "r")) == NULL)
    { 
        printf("ERROR: file doesn't exist\n");
        return -1;
    }

    install_alarm();
    //Open connection between app and data protocol
    //And connection between TRANSMITTER and RECEIVER
    llopen();

    //Send control package with START
    int package_size;
    if ((package_size = create_control_package(CTRL_START, filename, size, package)) < 0)
    {
        printf("ERROR\n");
        return 1;
    }

    int write_length;
    if ((write_length = llwrite(package, package_size)) < 0)
    {
        printf("ERROR\n");
        return 1;
    }

    //Keep sending Data packages until the end of the file
    int c_size = LINE_SIZE;
    int n = 0, line_size = 0;
    char*line[c_size];
    u_int8_t *frame = (u_int8_t *)malloc(sizeof(u_int8_t) * MAX_FRAME_SIZE);

    while (TRUE)
    {
        if (size - n * c_size < c_size) //File size - sequence number * line size 
        {
            c_size = size % c_size;
        }
        if ((line_size = fread(line, 1, c_size, fprt)) <= 0)
        { //could be an error or could be EOF
            break;
        }

        if (create_data_package(n, line_size, line, frame) < 0)
        {
            printf("ERROR\n");
            return 1;
        }

        int frame_size = line_size + 4; //4 because line_size only has the size of P1...Pk, we need it to have C, N, L2 and L1 into consideration
        if (llwrite(frame, frame_size) < 0)
        {
            printf("ERROR\n");
            return 1;
        }

        n++;
    }

    //Send control package with END
    if ((package_size = create_control_package(CTRL_END, filename, size, package)) < 0)
    { 
        printf("ERROR\n");
        return 1;
    }
    if ((write_length = llwrite(package, package_size)) < 0)
    {
        printf("ERROR\n");
        return 1;
    }

    //Close connection
    if (llclose() < 0)
    {
        printf("ERROR\n");
        return 1;
    }
    return 0;
}
