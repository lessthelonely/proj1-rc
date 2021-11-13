#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>

#include "constants.h"
#include "data_protocol.h"
#include "stuffing.h"

struct termios oldtio, newtio;

/*Returns fd or -1 in case of error*/
int llopen(char* porta,int sender){ //Slides uses int porta but it's more practical in char* because it's serial ports are in char*
    int fd;
    if(sender != TRANSMITTER || sender != RECEIVER){ //sender will be TRANSMITTER or RECEIVER
        printf("ERROR"); 
    }

    fd = open(porta, O_RDWR | O_NOCTTY);
  if (fd < 0)
  {
    perror(porta);
    return -1;
  }

  if (tcgetattr(fd, &oldtio) == -1)
  { /* save current port settings */
    perror("tcgetattr");
    return -1;
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 5;  /* blocking read until 5 chars received */

  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1)
  {
    perror("tcsetattr");
    return -1;
  }

    if(sender == TRANSMITTER){
        signal(SIGALRM, atende); //instala rotina que atende interrupção

        if(send_cmd(0, TRANSMITTER)<0){ //Send SET
           printf("ERROR");
        }

        alarm(3);

        if(read_cmd()<0){ //Receive UA
          printf("ERROR");
        }
    }
    else{

        //should it have a while here?
        if(read_cmd()<0){
            printf("ERROR");
        }
        
        if(send_cmd(2,TRANSMITTER)<0){//it's answering the transmitter
           printf("ERROR");
        }
    }

    return fd;
}

/*Orders protocol to send the Info trauma
char* buffer is char array we want to transmit so it will the be data part of the info trauma?
In order to make this function work->need to build the information trama
F A C BCC1 char*buffer? BCC2 F

Return value should -1 in case of error or number of caracters written
*/
int llwrite(int fd, char*buffer,int length){
  
  char* trama = (char*) malloc(MAX_SIZE*sizeof(char)); //Allocs space to write info trama
  memset(trama,0,strlen(trama)); //initialize trama array
  if(length<0){ 
    printf("Value should be positive in order to actually transfer data\n");
    return -1;
  }
  //Need to create info trauma + send it(should use the timeout mechanic here right? Might need to put alarm in a different file and change the routine)
  while(TRUE){ //might need a better condition-->thought for later
     //We initilized trauma array with the biggest possible size (MAX_SIZE) however most times, there won't actually be 255 bits to be written so we need the actual correct number in order to return it to fulfill the function's purpose


  }
}

int llread(int fd,char*buffer){

}

//need to send DISC message and get that message back and then send UA 
//also close fd of course
//in slides llclose just has fd as a parameter but we probably should specify if whoever called this function
//since transmitter has to send DISC to receiver, however receiver doesn't have to do that
//need to do alarm + timeout each time
int llclose(int fd, int sender){
  int check=-1,conta=0;
  if(sender != TRANSMITTER || sender != RECEIVER){ //sender will be TRANSMITTER or RECEIVER
      printf("ERROR"); 
    }
  
  if(sender == TRANSMITTER){
    while(check < 0 && conta < 3){
    if((check=send_cmd(1,TRANSMITTER))<0){ //send DISC
        //send again every 3 seconds for 3 times
        sleep(3);
        conta++;
    }
    else{
      conta = 0;
    }
    if(read_cmd()<0){ //read DISC
       //should it try to send it again?
       printf("Couldn't read DISC message");
    }
    if(send_cmd(2,TRANSMITTER)<0){ //send UA
       //receiver doesn't have to send it back so we don't have to do the timeout to make sure it was sent?
       printf("Couldn't send UA message");
    }
    }
  }
  else{
     while(check < 0 && conta < 3){
    if(read_cmd()<0){ //read DISC
       printf("Couldn't read DISC message");
    }
    if(send_cmd(1,TRANSMITTER)<0){ //send DISC back as a response
        //Do we have to do timeout for receiver, we were told we had to for trasmitter
        sleep(3);
        conta++;
    }
    else{
      conta = 0;
    }
    if(read_cmd()<0){ //read UA
       printf("Couldn't read UA messages");
    }
    }
  }

  //close fd
   sleep(1);
   if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
  {
    perror("tcsetattr");
    return -1;
  }
  close(fd);
  return 0;
}

int create_info_trauma(char*buffer,char*trauma,int length){
  /* Okay like got to add flag, A_E because it's the transmitter that sends info, C_I_ZERO (check slide 14) but wait it's
  C_I_ZERO for the first time, should I make a counter to see if it's the first or second time transmitter is sending the info trama
  doesn't make sense to make a new function just because of that-->let's do it with C_I_ZERO first just to structure it,
  BCC1 is BCC_C_I_ZERO(in the case that C==C_I_ZERO), then char*buffer (I think), BCC2 is going to be obtained by a for cycle 
  with every char from the char*buffer array (doing xor in all those guys) and finally adding another flag

  CAREFUL--->this is where bit stuffing comes in (and destuffing)
  Think it's after the info trama is assembled tho because slide 7 says "(antes de stuffing e após destuffing)"
  
  Okay new idea about stuffing (and destuffing):
    We have the original data->this data goes through stuffing before it's sent to the receiver because it's a way to ensure 
  that the transmission starts and ends at the correct places
    Receiver gets the trama stuffed, before it stores the information tho, sends the trama to go through the destuffing process

  With that in mind makes sense to have stuffing and destuffing in a separate file (maybe...)
  */

  //Let's start by defining BCC2-->it will need to be stuffed (like data) but according to slide 7 and 13, it is created before
  char BCC2 = buffer[0];
  for(int i = 1;i<(length-1);i++){
    BCC2 ^= buffer[i];
  }

  //We need to stuff the data + BCC2
  char* frame,bcc2_stuffed;
  stuffing(buffer,length,&frame); 
  stuffing(&BCC2,1,&bcc2_stuffed); //I mean BCC2 has length==1 sooooo I don't really know why it's necessary to stuff them tbh but I know it is according to the slides
  
}