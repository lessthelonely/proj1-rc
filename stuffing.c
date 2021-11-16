#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#include "constants.h"

/* What should be the return?
In case there's an error it should be -1 (makes sense to me)
If everything goes well, what do we return? 
->zero
->size of the new frame?--->if we do this then maybe it's not necessary to alloc that much space for trama?
*/
int stuffing(char* buffer, int length, char** frame) {
    *frame = NULL; //making sure frame is null/empty
    /*The new frame needs to have a different length than the og frame
and by different I mean bigger, everytime it finds ESC or FLAG we need to replace it with FLAG and ESC (respectively)
and add a ESC_FOUND and FLAG_FOUND (respectively)
      Question: is it worth iterating through the old frame (buffer) and make a counter that increases
everytime we find an ESC or a FLAG and then we alloc space for the new frame array pointer with the size
length + counter or do we just do length * 2 (for example, it's never going to be bigger than that and *2 is already an exaggeration).
Which is better: waste resources with a for cycle or waste it with allocating memory?
    */
    *frame = (char*) malloc(length * 2);

    int new_size = 0;
    for (int i = 0; i < length; i++) {
        if (buffer[i] == FLAG) { //Found 0x7E inside the trama
            (*frame)[new_size] = ESC;
            (*frame)[++new_size] = FLAG_FOUND;
        }

        else if (buffer[i] == ESC) { //Found 0x7D inside the trama
            (*frame)[new_size] = ESC;
            (*frame)[++new_size] = ESC_FOUND;
        }

        else{
            (*frame)[new_size]=buffer[i];
        }
        new_size++;
    }
    free(frame);
    return new_size; 
}

/* What should be the return?
In case there's an error it should be -1 (makes sense to me)
If everything goes well, what do we return? 
->zero
->size of the new/og frame?
*/
int destuffing(char* buffer, int length, char** frame) {
    *frame = NULL;
    //With stuffing the frame goes back to its original size
    *frame = (char*) malloc(length);

    int new_size = 0;
    for (int i = 0; i < length; i++) {
        if (buffer[i] == ESC){
            (*frame)[new_size] = ESC_FOUND;
        }
        else{
            (*frame)[new_size] = buffer[i];
        }

        new_size++;
    }

    free(frame);
    return new_size; 
}