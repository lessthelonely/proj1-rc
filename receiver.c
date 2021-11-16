//RECEIVER
#include "app.h"
#include "constants.h"
#include "protocol_app.h"

//Here we are gonna use llread
FILE *fptr;

int main(int argc, char** argv){
    char*package=(char*)malloc(sizeof(char)*MAX_FRAME_SIZE);
    char*frame=(char*)malloc(sizeof(char)*MAX_FRAME_SIZE);
    //Parse arguments
    //Should reader select timeout, number of sequences, baudRate (whatever that is, I think it's only important for the last part)
    for(int i=1;i<argc;i++){
        if(strcmp(argv[i],"") != 0){
             if (!strncmp(argv[i], "/dev/ttyS", 9)){
                 strcpy(link_info.port,argv[i]);
             }

             if(!strcmp(argv[i],"-r")){
                 app_info.status = RECEIVER;
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
        }
    }

    int fd;
    //Open connection between app and data protocol
    //And connection between TRANSMITTER and RECEIVER
    if(fd= llopen(link_info.port,app_info.status)){
        printf("ERROR\n");
        free(package);
        free(frame);
        return 1;
    }
    app_info.fileDescriptor = fd;

    int size;
    int received_ctrl_pack_start =  FALSE;
    //Gonna receive a Control Package with START
    while(!received_ctrl_pack_start){
        if((size = llread(fd,package))<0){
            printf("ERROR\n");
            free(package);
            free(frame);
            return 1;
        }
        char*filename;
        int file_size;
        if(package[0] == CTRL_START){ //okay maybe I should have them in constants
           if(read_control_package(package,filename,&file_size,size)<0){
               printf("ERROR\n");
               free(package);
               free(frame);
               return 1;
           }
           received_ctrl_pack_start = TRUE;
        }
    }

    char*new_file = "new_file.txt"; //Should I ask the receiver to tell me the file name?
    if((fptr = fopen(new_file,"w")) == NULL){
        printf("ERROR\n");
        free(package);
        free(frame);
        return 1;
    }

    //Gonna receive Data Packages as well and a Control package with END
    int not_end = FALSE;
    while(!not_end){
        if((size = llread(fd,package)) < 0){
            printf("ERROR\n");
            free(package);
            free(frame);
            return 1;
        }
        if(package[0]==CTRL_DATA){
            if((size=read_data_package(frame,package))<0){
                printf("ERROR\n");
                free(package);
                free(frame);
                return 1;
            }
            fwrite(frame,sizeof(char),size,fptr);
        }
        if(package[0] == CTRL_END){
            int file_size;
            if(read_control_package(package,new_file,&file_size,size) <0){
                printf("ERROR\n");
                free(package);
                free(frame);
                return 1;
            }
            not_end=TRUE;
        }
    }

    //Close connection 
    if(llclose(fd,app_info.status) < 0){
        printf("ERROR\n");
        free(package);
        free(frame);
        return 1;
    }
    free(package);
    free(frame);
    return 0;
}