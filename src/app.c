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

int create_data_package(int n, int length, u_int8_t *data, u_int8_t *package)
{
    /*We are gonna ask the user for stuff to fill out the struct linkLayer 
    Need to make two packets: control and data
    Control packets have two types: 1) symbolizes transmission start
                                    2) symbolizes transmission end
    Contents of each packet->slide 23
    Packages are sent by TRANSMITTER-->need to make functions to read them? Yeah, probs
    */
    package[0] = CTRL_DATA; //C – campo de controlo (valor: 1 – dados)
    package[1] = n % 255;  
    package[2] = length / 256; //L2
    package[3] = length % 256; //L1
    if (memcpy(&package[4], data, length) == NULL)
    { //memcpy returns destination pointer
        printf("ERROR\n");
        return -1;
    }
    return 0;
}

int create_control_package(u_int8_t c, u_int8_t *file_name, int file_size, u_int8_t *package)
{
    int size = 0;
    package[0] = c; //need to be informed if it's supposed to be the start (2) or end (3)
    /*Going to have two sets of TLV:
    First one is about the size of the file
    Second about the name of the file
    */
    char *lstring = (char *)malloc(sizeof(int));
    sprintf(lstring, "%d", file_size);

    package[1] = 0; //file size (Type)
    package[2] = strlen(lstring); //size of file size (Length)
    if (memcpy(&package[3], lstring, strlen(lstring)) == NULL) //file size (Value)
    {
        printf("ERROR\n");
        free(lstring);
        return -1;
    }
    size = 3 + strlen(lstring);

    package[size] = 1; //file name (Type)
    size++;
    package[size] = strlen(file_name);//size of file name (Length)
    size++;
    if (memcpy(&package[size], file_name, strlen(file_name)) == NULL) //file name (Value)
    {
        printf("ERROR\n");
        free(lstring);
        return -1;
    }
    size += strlen(file_name);
    free(lstring);
    return size;
}

int read_data_package(u_int8_t *data, u_int8_t *package)
{
    /*Okay package[0] doesn't matter here, will matter where we call it tho
    package[2] & [3] are gonna be used to know the size of the info
    Rest of the package is gonna be copied into u_int8_t*data because data
    */
    
    /*app_info.sequenceNumber is where we store the sequence number we are expecting (packet wise).
    This value is initially 0, thus the first package we expect has got to have 0 as a sequence number.
    This is what's expected because we still haven't gotten that package. If we get it, we then update 
    the value of app_info.sequenceNumber for future runs of read_data_package().
    If we receive a value that is lower than the one in app_info.sequenceNumber then it's 
    repeated: we don't want it because we already have it. If the value in the package received is 
    *higher* than expected, then it means a package was jumped over and, because of that, lost - thus 
    the file transmission has failed */


    if (package[1] < app_info.sequenceNumber)
    {
        return -1; //repeated packet
    }
    app_info.sequenceNumber = (app_info.sequenceNumber + 1) % 255; //Update

    int size = 256 * package[2] + package[3];
    if (memcpy(data, &package[4], size) == NULL)
    {
        printf("ERROR\n");
        return -1;
    }
    return size;
}

int read_control_package(u_int8_t *package, u_int8_t *file_name, int *file_size, int package_size)
{
    u_int8_t *sizes = (u_int8_t*)malloc(sizeof(int));
    int size;
    for (int i = 1; i < package_size; i++)
    {
        if (package[i] == 0)
        { //file size
            i++;
            size = package[i];
            i++;
            if (memcpy(sizes, &package[i], size) == NULL)
            {
                printf("ERROR\n");
                free(sizes);
                return -1;
            }
            sscanf(sizes, "%d", file_size);
            i += size;
        }
        if (package[i] == 1)
        { //file name
            i++;
            size = package[i];
            i++;
            if (memcpy(file_name, &package[i], size) == NULL)
            {
                printf("ERROR\n");
                free(sizes);
                return -1;
            }
            i += size-1;
        }
    }
    free(sizes);
    return 0;
}
