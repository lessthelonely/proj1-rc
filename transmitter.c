//TRANSMITTER
#include <stdio.h>

#include "app.h"
#include "constants.h"
#include "protocol_app.h"

FILE *fprt;
//Here we are gonna use llwrite

int main(int argc, char** argv){
    char*filename;
    //Parse arguments
    for(int i=1;i<argc;i++){
        if(strcmp(argv[i],"") != 0){
             if (!strncmp(argv[i], "/dev/ttyS", 9)){
                 strcpy(link_info.port,argv[i]);
             }

             //Issuer == Emissor
             if(!strcmp(argv[i],"-i")){
                 app_info.status = TRANSMITTER;
             }
             
             if(!strcmp(argv[i],"-t")){
                 link_info.timeout = atoi(argv[i+1]);
             }

             if(!strcmp(argv[i],"-n")){
                 link_info.numTransmissions = atoi(argv[i+1]);
             }

             if(!strcmp(argv[i],"-b")){
                 link_info.baudRate = atoi(argv[i+1]);
             }

              if(!strcmp(argv[i],"-f")){
                  strcpy(filename,argv[i+1]);
             }
        }
    }

    int fd;
    //Open connection between app and data protocol
    //And connection between TRANSMITTER and RECEIVER
    if(fd= llopen(link_info.port,app_info.status)){
        printf("ERROR\n");
        free(filename);
        return 1;
    }

    //Open the file + size of file
    int size;
    if(size = open_file(filename) <0){
        printf("ERROR\n");
        free(filename);
        return 1;
    }
    
    //Send control package with START
    //Keep sending Data packages until the end of the file
    //Send control package with END

    //Close connection 
    if(llclose(fd,app_info.status) < 0){
        printf("ERROR\n");
        free(filename);
        return 1;
    }
    free(filename);
    return 0;
}

//File: https://www.programiz.com/c-programming/c-file-input-output
//File size: https://www.geeksforgeeks.org/c-program-find-size-file/
/*Returns -1 in case of error
Returns the size of the file otherwise*/
int open_file(char* filename){
    //Open file
    if((fprt = fopen(filename,"r")) == NULL){ //can I use fopen or should it be open? Also r or rb (read in binary)?
        printf("ERROR: file doesn't exist\n");
        return -1;
    }
    
    //Get size of file
    if(fseek(fprt,0L,SEEK_END) != 0){
        printf("ERROR\n");
        return -1;
    }

    int size= ftell(fprt);
    
    //Should I put the pointer at the start with fseek + SEEK_SET?
    
    return size;
}