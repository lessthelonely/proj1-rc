#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>

#include "app.h"
#include "constants.h"

/*We are gonna ask the user for stuff to fill out the struct linkLayer 

Need to make two packets: control and data
Control packets have two types: 1) symbolizes transmission start
                                2) symbolizes transmission end
Contents of each packet->slide 23

Packages are sent by TRANSMITTER-->need to make functions to read them? Yeah, probs
*/

/* Return -1 if error, 0 otherwise
*/
int create_data_package(int n,int length, char*data,char*package){
    //ASK TEACHER: check how N, L2 and L1 are calculated
    package[0] = CTRL_DATA; //C – campo de controlo (valor: 1 – dados)
    package[1] = n % 255 ; //%255->does it only want the sequenceNumber or is it, sequenceNumber % 255
    app_info.sequenceNumber = package[1];
    package[2] = length / 256; //L2
    package[3] = length % 256; //L1
    if(memcpy(&package[4],data,length) == NULL){ //memcpy returns destination pointer
       printf("ERROR\n");
       return -1;
    }
    printf("Data package was created");
    return 0;
}

int create_control_package(int c,char* file_name, int file_size, char*package){
    int size=0;
    package[0] = c; //need to be informed if it's supposed to be the start (2) or end (3)-->should I make constants?
    /*Going to have two sets of TLV:
    First one is about the size of the file
    Second about the name of the file
    */

    char*lstring = (char*)malloc(sizeof(int));
    sprintf(lstring,"%d",file_size);
    package[1] = 0; //file size (should it be a constant?)
    package[2] = strlen(lstring);
    if(memcpy(&package[3],lstring,strlen(lstring) == NULL)){
        printf("ERROR\n");
        free(lstring);
        return -1;
    }
    size=3+strlen(lstring);

    package[size] = 1;
    size++;
    package[size] = strlen(file_name);
    size++;
   if(memcpy(&package[size],file_name,strlen(file_name)) == NULL){
        printf("ERROR\n");
        free(lstring);
        return -1;
    }
    size+=strlen(file_name);
    printf("Control package created\n");
    free(lstring);
    return size;
}

int read_data_package(char*data,char*package){
    /*Okay package[0] doesn't matter here, will matter where we call it tho
    should we change the sequence number to whatever sequence number is in package[1]?
    package[2] & [3] are gonna be used to know the size of the info
    Rest of the package is gonna be copied into char*data because data
*/
    
    //What if there's more values to sequence number than 0 and 1 and that's why we got the %255 stuff? Idk tbh

    if(package[1]>app_info.sequenceNumber){
        return -1; //repeated packet
    }
    app_info.sequenceNumber = (app_info.sequenceNumber + 1) %255; //Update
    

    int size = 256*package[2] + package[3];
    if(memcpy(data,&package[4],size) == NULL){
        printf("ERROR\n");
        return -1;
    }
    return size;
}

int read_control_package(char*package,char*file_name,int*file_size,int package_size){
    char*sizes =(char*)malloc(sizeof(int));
    int size;
    /*Idk if package is written differently if C is start or end
    Let's assume it's the same I guess
    I think if it's start, then you send the data package and if it's stop...you don't? Don't really know*/

    for(int i = 1;i<package_size;i++){
        if(package[i] == 0){ //file size
           i++;
           size=package[i];
           i++;
           if(memcpy(sizes,&package[i],size) == NULL){
               printf("ERROR\n");
               free(sizes);
               return -1;
           }
           sscanf(sizes,"%d",file_size);
           i+=size;
        }
        if(package[i] == 1){ //file name
           i++;
           size=package[i];
           i++;
           if(memcpy(file_name,&package[i],size) == NULL){
               printf("ERROR\n");
               free(sizes);
               return -1;
           }
           i+=size;
        }
    }
    free(sizes);
    return 0;
}