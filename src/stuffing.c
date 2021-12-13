#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "constants.h"

int stuffing(u_int8_t *buffer, int length)
{
    
    /*The new frame needs to have a different length than the og frame
and by different I mean bigger, everytime it finds ESC or FLAG we need to replace it with FLAG and ESC (respectively)
and add a ESC_FOUND and FLAG_FOUND (respectively)
    */
   int more=0;
   for(int i=0;i<length;i++){
       if(buffer[i] ==  FLAG || buffer[i] == ESC){ //sees if it can find any Flags or ESCs inside of trama, if so this means one extra space because we have to add an ESC_FOUND or FLAG_FOUND
           more++;
       }
   }

    int nl = more + length;
    u_int8_t*frame = (u_int8_t*)malloc(sizeof(u_int8_t)*nl);

    int counter=0;
    int index=0;
    for (int i = 0; i < length; i++)
    {
        index = i + counter;
        if (buffer[i] == FLAG)
        { //Found 0x7E inside the trama
            frame[index] = ESC;
            frame[index + 1] = FLAG_FOUND;
            counter ++;
        }

        else if (buffer[i] == ESC)
        { //Found 0x7D inside the trama
            frame[index] = ESC;
            frame[index + 1] = ESC_FOUND;
            counter++;

        }

        else
        {
            frame[index] = buffer[i];
        }
    }
    memcpy(buffer,frame,nl);
    return nl;
}

int destuffing(u_int8_t *buffer, int length)
{
    //With stuffing the frame goes back to its original size
    u_int8_t*frame = (u_int8_t *)malloc(sizeof(u_int8_t)*length);

    int new_size = 0;
    for (int i = 0; i < length; i++)
    {
        if (buffer[i] == ESC) //common point
        {
            if(buffer[i+1] == FLAG_FOUND){
                frame[new_size]=FLAG;
            }
            else if(buffer[i+1] == ESC_FOUND){
                frame[new_size]=ESC;
            }
            i++;
        }
        else
        {
           frame[new_size] = buffer[i];
        }
        new_size++;
    }

    memcpy(buffer,frame,new_size);
    free(frame);
    return new_size;
}
