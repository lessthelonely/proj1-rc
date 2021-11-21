//RECEIVER
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "../include/app.h"
#include "../include/protocol_app.h"
#include "../include/constants.h"

// ./receiver /dev/ttySx (-f file name)

int main(int argc, char **argv)
{
    u_int8_t *package = (u_int8_t *)malloc(sizeof(u_int8_t) * MAX_FRAME_SIZE);
    u_int8_t *frame = (u_int8_t *)malloc(sizeof(u_int8_t) * MAX_FRAME_SIZE);
    app_info.status = RECEIVER;
    char *filename = (char*)malloc(sizeof(char)*MAX_FRAME_SIZE);
    char *new_file = (char*)malloc(sizeof(char)*MAX_FRAME_SIZE);
    int file_size;
    int index=-1;
    FILE *fptr;

    //Parse arguments
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "") != 0)
        {
            if (!strncmp(argv[i], "/dev/ttyS", 9))
            {

                strcpy(link_info.port, argv[i]);
            }

            if (!strcmp(argv[i], "-f"))
            {
                index=i+1;
            }
        }
    }

    if(index==-1){
        new_file=filename;
    }
    else{
        new_file=argv[index];
    }

    //Open connection between app and data protocol
    //And connection between TRANSMITTER and RECEIVER
    llopen(); 

    int size;
    int received_ctrl_pack_start = FALSE;
    //Gonna receive a Control Package with START
    while (!received_ctrl_pack_start)
    {
        if ((size = llread(package)) < 0)
        {
            printf("ERROR\n");
            free(package);
            free(frame);
            return 1;
        }
    
        if (package[0] == CTRL_START)
        { 
            if (read_control_package(package, filename, &file_size, size) < 0)
            {
                printf("ERROR\n");
                free(package);
                free(frame);
                return 1;
            }
            received_ctrl_pack_start = TRUE;
        }
    }

    
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
        if ((size = llread(package)) < 0)
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
            fwrite(frame, sizeof(u_int8_t), size, fptr);
        }

        memset(frame,0,strlen(frame));

        if (package[0] == CTRL_END)
        {
            u_int8_t*end_outputfile[MAX_FRAME_SIZE];
            int end_file_size;
            if (read_control_package(package, end_outputfile, &end_file_size, size) < 0)
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
    if (llclose() < 0)
    {
        printf("ERROR\n");
        free(package);
        free(frame);
        return 1;
    }
    free(package);
    free(frame);
    free(filename);
    if( (fclose(fptr)) == EOF ) {
        printf("ERROR\n");
        return 1;
    }
   return 0;
}

