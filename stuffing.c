#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "../include/constants.h"

/* What should be the return?
In case there's an error it should be -1 (makes sense to me)
If everything goes well, what do we return? 
->zero
->size of the new frame?--->if we do this then maybe it's not necessary to alloc that much space for trama?
*/
int stuffing(u_int8_t *buffer, int length)
{
    printf("I'm in stuffing \n",length);
    u_int8_t*frame; //making sure frame is null/empty
    /*The new frame needs to have a different length than the og frame
and by different I mean bigger, everytime it finds ESC or FLAG we need to replace it with FLAG and ESC (respectively)
and add a ESC_FOUND and FLAG_FOUND (respectively)
      Question: is it worth iterating through the old frame (buffer) and make a counter that increases
everytime we find an ESC or a FLAG and then we alloc space for the new frame array pointer with the size
length + counter or do we just do length * 2 (for example, it's never going to be bigger than that and *2 is already an exaggeration).
Which is better: waste resources with a for cycle or waste it with allocating memory?
    */
   int more=0;
   for(int i=0;i<length;i++){
       if(buffer[i] ==  FLAG || buffer[i] == ESC){
           more++;
       }
   }

    int nl = more+ length;
    frame = (u_int8_t*)malloc(sizeof(u_int8_t)*nl);
    printf("Frame here\n");

    int counter=length;
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
    printf("I did the process\n");
    printf("I'm leaving\n");
    return nl;
}

/* What should be the return?
In case there's an error it should be -1 (makes sense to me)
If everything goes well, what do we return? 
->zero
->size of the new/og frame?
*/
int destuffing(u_int8_t *buffer, int length)
{
    //With stuffing the frame goes back to its original size
    u_int8_t*frame = (u_int8_t *)malloc(sizeof(u_int8_t)*length);

    int new_size = 0;
    for (int i = 0; i < length; i++)
    {
        if (buffer[i] == ESC)
        {
            frame[new_size] = ESC_FOUND;
        }
        else
        {
           frame[new_size] = buffer[i];
        }

        new_size++;
    }

    memcpy(buffer,frame,new_size);
    printf("new_size %d\n",new_size);
    free(frame);
    return new_size;
}