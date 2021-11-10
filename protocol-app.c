#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include "constants.h"
#include "data_protocol.h"

/*Returns fd or -1 in case of error*/
int llopen(char* porta,int sender){ //Slides uses int porta but it's more practical in char* because it's serial ports are in char*
    struct termios oldtio, newtio;
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

int llwriter(int fd, char*buffer,int length){

}

int llread(int fd,char*buffer){

}

int llclose(int fd){

}