//TRANSMITTER
#include <stdio.h>

#include "app.h"
#include "constants.h"
#include "protocol_app.h"
#include "alarm.h"

FILE *fprt;
//Here we are gonna use llwrite

int main(int argc, char** argv){
    char*filename;
    char*package [MAX_SIZE]; //should it be larger?
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

    install_alarm();
    int fd;
    //Open connection between app and data protocol
    //And connection between TRANSMITTER and RECEIVER
    if(fd= llopen(link_info.port,app_info.status)){
        printf("ERROR\n");
        return 1;
    }
    app_info.fileDescriptor = fd;


    //Open the file + size of file
    int size;
    if(size = open_file(filename) <0){
        printf("ERROR\n");
        return 1;
    }
    
    //Send control package with START
    int package_size;
    if((package_size = create_control_package(CTRL_START,filename,size,package)) < 0){
        printf("ERROR\n");
        return 1;
    }
    int write_length;
    if((write_length = llwrite(fd,package,package_size)) <0){
        printf("ERROR\n");
        return 1;
    }
    //Keep sending Data packages until the end of the file
    int c_size = MAX_FRAME_SIZE;
    int n=0,line_size=0;
    char*line[c_size];
    char*frame=(char*)malloc(sizeof(char)*MAX_FRAME_SIZE);

    while(TRUE){
        if(size - n*c_size < c_size){
            c_size=size % c_size;
        }
        if((line_size=fread(line,1,c_size,fprt) <=0)){ //could be an error or could be EOF
            break;
        }

        if(create_data_package(n,line_size,line,frame) <0){
            printf("ERROR\n");
            free(frame);
            return 1;
        }

        int frame_size = line_size + 4;
        if(llwrite(fd,frame,frame_size) <0){
            printf("ERROR\n");
            free(frame);
            return 1;
        }

        n++;
    }
     
    //Send control package with END
    if((package_size = create_control_package(CTRL_END,filename,size,package)) < 0){ //again should I put the control stuff in constants?
        printf("ERROR\n");
        free(frame);
        return 1;
    }
    if((write_length = llwrite(fd,package,package_size)) <0){
        printf("ERROR\n");
        free(frame);
        return 1;
    }

    //Close connection 
    if(llclose(fd,app_info.status) < 0){
        printf("ERROR\n");
        free(frame);
        return 1;
    }
    free(frame);
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