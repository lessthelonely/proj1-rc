#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#include "constants.h"

//We will need to change variable types + place but here's the logic 

int byte_stuffing(unsigned char* packet, int length, unsigned char** frame) {
    *frame = NULL;
    *frame = (unsigned char*) malloc(length * 2);

    int index = 0;
    for (int c = 0; c < length; c++) {
        if (packet[c] == FLAG) { //Found 0x7E inside the trama
            (*frame)[index] = ESC;
            (*frame)[++index] = FLAG_FOUND;
        }

        else if (packet[c] == ESC) { //Found 0x7D inside the trama
            (*frame)[index] = ESC;
            (*frame)[++index] = ESC_FOUND;
        }

        else{
            (*frame)[index]=packet[c];
        }
        index++;
    }

    return index; 
}


int byte_destuffing(unsigned char* packet, int length, unsigned char** frame) {
    *frame = NULL;
    *frame = (unsigned char*) malloc(length);

    int index = 0;
    for (int c = 0; c < length; c++) {
        if (packet[c] == ESC){
            (*frame)[index] = ESC_FOUND;
        }
        else{
            (*frame)[index] = packet[c];
        }

        index++;
    }

    return index; 
}