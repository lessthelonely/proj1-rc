/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>

#define FLAG 0x7E
#define A_E 0x03 //Comandos do Emissor e Respostas do Receptor
#define A_R 0x01 //Comandos do Receptor e Respostas do Emissor
#define C_SET 0x03
#define C_DISC 0x0B
#define C_UA 0x07
#define C_RR_ONE 0x85
#define C_RR_ZERO 0x05
#define C_REJ_ONE 0x11
#define C_REJ_ZERO 0x01
#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;
int got_UA = FALSE;

const u_int8_t BCC = A_E ^ C_SET;  //BCC=XOR(A,C)
int flag =1, conta=1;
int fd;
u_int8_t set[5] = {FLAG, A_E, C_SET, BCC, FLAG};

void send_set(){ //emissor (writenoncanonical.c)
  int res;
  res = write(fd,set,sizeof(set));  
  if(res==-1){
      printf("ERROR\n");
      exit(1);
  } 
}

void read_ua(){ //emissor (writenoncanonical.c)
  int i=0;
  int res;
  u_int8_t buf[255];

  while (STOP==FALSE) {       /* loop for input */
    u_int8_t byte_received;
    res = read(fd,&byte_received,1);   /* returns after 5 chars have been input */
    if(res==-1){
      printf("ERROR\n");
      exit(1);
    }
    if(i==0 && byte_received !=FLAG){
      continue;
    }
    if(i>0 && byte_received == FLAG){ //last byte sent by SET
      STOP =TRUE;
    }
    buf[i++]=byte_received;               /* so we can printf... */
    }

    got_UA = TRUE;

    printf("Printing UA:\n");
    for(int j=0;j<5;j++){
      printf("%u\n",buf[j]);
    }
}

void atende() // atende alarme--->emissor(writenonical.c)
{
   if(!got_UA){
    send_set();
    printf("alarme # %d\n", conta);
    conta++;
    if(conta < 4){
      alarm(3);
    }
    else{
      printf("ERROR - Timeout over. It wasn't possible to receive UA successfully\n");
      exit(1);
    }
  }
}

int main(int argc, char** argv)
{
    int c;
    struct termios oldtio,newtio;
    int i, sum = 0, speed = 0;
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS10", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS11", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

    // /dev/ttyS10 is emissor and /dev/ttyS11 is receiver

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

    if(strcmp("/dev/ttyS10", argv[1])==0){
        
    signal(SIGALRM, atende);  //instala rotina que atende interrupção

   
    send_set();//Send SET
    
    alarm(3);
    
    read_ua();//Receive UA
    }

    if(strcmp("/dev/ttyS11", argv[1])==0){
        int res;
        u_int8_t buf[255];
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
        u_int8_t ua[5]={FLAG,A_E,C_SET,BCC,FLAG};

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
    }
    
    sleep(1);
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    close(fd);
    return 0;
}
