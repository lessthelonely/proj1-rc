#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>

#include "app.h"

/*We are gonna ask the user for stuff to fill out the struct linkLayer 

Need to make two packets: control and data
Control packets have two types: 1) symbolizes transmission start
                                  2) symbolizes transmission end
Contents of each packet->slide 23

Packages are sent by TRANSMITTER-->need to make functions to read them? Yeah, probs
*/

/* Return -1 if error, 0 otherwise
*/
int create_data_package(int length, char*data,char*package){
    //ASK TEACHER: check how N, L2 and L1 are calculated
    package[0] = 1; //C – campo de controlo (valor: 1 – dados)
    package[1] = link_info.sequenceNumber; //%255->does it only want the sequenceNumber or is it, sequenceNumber % 255
    package[2] = length / 256; //L2
    package[3] = length % 256; //L1
    if(memcpy(&package[4],data,length) == NULL){ //memcpy returns destination pointer
       printf("ERROR\n");
       return -1;
    }
    printf("Data package was created");
    return 0;
}

int create_control_package(int c,char* file_name, int length, char*package){

}

int read_data_package(){}

int read_control_package(){}