/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define FLAG 0x7E
#define A 0x01
#define C 0x03
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

u_int8_t BCC = A ^ C;  //BCC=XOR(A,C)

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    u_int8_t buf[255];

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS10", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS11", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
  
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");


    //Receive SET
    int i=0;
    while (STOP==FALSE) {       /* loop for input */
      u_int8_t byte_received;
      res = read(fd,&byte_received,1);   /* returns after 5 chars have been input */
      if(res==-1){
         printf("ERROR\n");
         exit(1);
      }
      if(i>0 && byte_received == FLAG){ //last byte sent by SET
          STOP =TRUE;
      }
      buf[i++]=byte_received;               /* so we can printf... */
    }

    printf("Printing SET:\n");
    for(int j=0;j<5;j++){
      printf("%u\n",buf[j]);
    }

    //Check if SET is correct
    u_int8_t ACK = buf[1] ^ buf[2] ^ buf[3];
    u_int8_t ua[5]={FLAG,A,C,BCC,FLAG};

    if(ACK == 0x00){ //Send UA
       res=0;
       res= write(fd,ua,sizeof(ua));
       if(res==-1){
          printf("ERROR\n");
          exit(1);
        }
    }
    else{ //the message isn't correct
      printf("ERROR\n");
    }

    sleep(1);
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
