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

int create_data_package(int length, char*data,char*package){
}

int create_control_package(int c,char* file_name, int length, char*package){
    
}

int read_data_package(){}

int read_control_package(){}