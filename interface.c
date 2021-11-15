//TRANSMITTER
//RECEIVER
#include "app.h"
#include "constants.h"
#include "protocol_app.h"

int main(int argc, char** argv){
    char*filename;
    for(int i=1;i<argc;i++){
        if(strcmp(argv[i],"") != 0){
             if (!strncmp(argv[i], "/dev/ttyS", 9)){
                 strcpy(link_info.port,argv[i]);
             }

             //Issuer == Emissor
             if(!strcmp(argv[i],"-i")){
                 app_info.status = TRANSMITTER;
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

              if(!strcmp(argv[i],"-f")){
                  strcpy(filename,argv[i+1]);
             }
        }
    }

    int fd;
    if(fd= llopen(link_info.port,app_info.status)){
        printf("ERROR\n");
        free(filename);
        return 1;
    }

    if(llclose(fd,app_info.status) < 0){
        printf("ERROR\n");
        free(filename);
        return 1;
    }
    free(filename);
    return 0;
}