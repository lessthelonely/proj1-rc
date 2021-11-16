//RECEIVER
#include "app.h"
#include "constants.h"
#include "protocol_app.h"

//Here we are gonna use llread

int main(int argc, char** argv){
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
        return 1;
    }

    //Gonna receive a Control Package with START
    //Gonna receive a Data Package as well
    //Gonna receive a Control package with END
    //Should create a new file where I recreate the file I was sent?

    
    //Close connection 
    if(llclose(fd,app_info.status) < 0){
        printf("ERROR\n");
        return 1;
    }
    return 0;
}