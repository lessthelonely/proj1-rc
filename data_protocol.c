/*Non-Canonical Input Processing*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>

#include "data_protocol.h"
#include "constants.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

int got_CMD = FALSE;

int flag = 1, conta = 1;
int fd;
u_int8_t cmd[5];
u_int8_t buf[255];

/*Function to use for every cmd except when it's an info trama*/
int send_cmd(int command, int sender) { //emissor (writenoncanonical.c)
  if (sender == TRANSMITTER) { //Emitter
    switch (command) {
      case 0: // SET
        cmd[0] = FLAG; cmd[1] = A_E; cmd[2] = C_SET;
        cmd[3] = BCC_SET; cmd[4] = FLAG;
        break;
      case 1: // DISC
        cmd[0] = FLAG; cmd[1] = A_E; cmd[2] = C_DISC;
        cmd[3] = BCC_DISC_E; cmd[4] = FLAG;
        break;
      case 2: // UA
        cmd[0] = FLAG; cmd[1] = A_E; cmd[2] = C_UA;
        cmd[3] = BCC_UA_E; cmd[4] = FLAG;
        break;
      case 3: // RR
        cmd[0] = FLAG; cmd[1] = A_E; cmd[2] = C_RR_ONE;
        cmd[3] = BCC_RR_ONE; cmd[4] = FLAG;
        break;
      case 4: // RR
        cmd[0] = FLAG; cmd[1] = A_E; cmd[2] = C_RR_ZERO;
        cmd[3] = BCC_RR_ZERO; cmd[4] = FLAG;
        break;
      case 5:
        cmd[0] = FLAG; cmd[1] = A_E; cmd[2] = C_REJ_ONE;
        cmd[3] = BCC_REJ_ONE; cmd[4] = FLAG;
        break;
      case 6:
        cmd[0] = FLAG; cmd[1] = A_E; cmd[2] = C_REJ_ZERO;
        cmd[3] = BCC_REJ_ZERO; cmd[4] = FLAG;
        break;
    }
  } else if (sender == RECEIVER) { // Sender
    switch (command) {
      case 0: // DISC
        cmd[0] = FLAG; cmd[1] = A_R; cmd[2] = C_DISC;
        cmd[3] = BCC_DISC_R; cmd[4] = FLAG;
        break;
      case 1: // UA
        cmd[0] = FLAG; cmd[1] = A_R; cmd[2] = C_UA;
        cmd[3] = BCC_UA_R; cmd[4] = FLAG;
        break;
      case 2: // RR
        cmd[0] = FLAG; cmd[1] = A_E; cmd[2] = C_RR_ONE;
        cmd[3] = BCC_RR_ONE; cmd[4] = FLAG;
        break;
      case 3: // RR
        cmd[0] = FLAG; cmd[1] = A_E; cmd[2] = C_RR_ZERO;
        cmd[3] = BCC_RR_ZERO; cmd[4] = FLAG;
        break;
      case 4:
        cmd[0] = FLAG; cmd[1] = A_E; cmd[2] = C_REJ_ONE;
        cmd[3] = BCC_REJ_ONE; cmd[4] = FLAG;
        break;
      case 5:
        cmd[0] = FLAG; cmd[1] = A_E; cmd[2] = C_REJ_ZERO;
        cmd[3] = BCC_REJ_ZERO; cmd[4] = FLAG;
        break;
    }
  }
  
  printf("I'M SENDING THIS COMMAND: %d | %d | %d | %d | %d \n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
  int res = write(fd, cmd, sizeof(cmd));
  if (res == -1)
  {
    printf("ERROR IN SENDING.\n");
  }
  return res;
}

/*do I have to know the sequence number?
do I have to get the char*cmd like read_cmd?
I don't need a char*cmd or well I do but not for the same reason
I need a char array in order to store the data part of the info trama
Because I'll need to check if BCC2 is correct, if it's not, we need to dump this trama

Returns -1 in case of error or length of the trama written
*/
int read_info_trama(char*info_trama){
  char byte_received;
  int res;
  messageState state = START;
  //should check the value of BCC in order to see if we can move on to BCC_OK state
  static int is_bcc_okay = 0;

  while(TRUE){
    if(res=read(fd,&byte_received,1) < 0){
      printf("ERROR\n");
      return res;
    }
    res=0;

   switch(state){
    case START:
      if(byte_received == FLAG){
        is_bcc_okay = 0;
        state=FLAG_RCV;
      }
      break;
    case FLAG_RCV:
      if(byte_received == A_E){ //info trama is only sent by TRANSMITTER
        is_bcc_okay ^= byte_received;
        state=A_RCV;
      }
      if(byte_received != FLAG){ //according to the teacher's state machine if it's a FLAG we should stay in this state if not we should go to START
        state = START;
      }
      break;
    case A_RCV:
      //C  Campo de Controlo 0 S 0 0 0 0 0 0 S = N(s) -->slide 7
      if(byte_received == C_I_ONE || byte_received == C_I_ZERO){
        is_bcc_okay ^= byte_received;
        state=C_RCV;
      }
      else if(byte_received == FLAG){
        state=FLAG_RCV;
      }
      else{
        state = START;
      }
      break;
    case C_RCV:
      is_bcc_okay ^=byte_received;
      if(is_bcc_okay == 0){
        state= BCC_OK;
      }
      else if(byte_received == FLAG){
        state=FLAG_RCV;
      }
      else{
        state = START;
      }
      break;
    case BCC_OK:
      if(byte_received == FLAG){
        state=FLAG_RCV;
      }
      else{
        info_trama[res] = byte_received;
        res++;
        state=D_RCV;
      }
      break;
    case D_RCV:
      if(byte_received == FLAG){
        state = STOP;
      }
      else{
        info_trama[res] = byte_received;
        res++;
      }
    case STOP:
      return res;
  }
  }
}


/*int read_cmd()
{ //emissor (writenoncanonical.c)
  int i = 0;
  int res;

  while (STOP == FALSE)
  { 
    u_int8_t byte_received;
    res = read(fd, &byte_received, 1);
    if (res == -1)
    {
      printf("ERROR\n");
      return res;
    }
    if (i == 0 && byte_received != FLAG)
    {
      continue;
    }
    if (i > 0 && byte_received == FLAG)
    { //last byte sent by SET
      STOP = TRUE;
    }
    buf[i++] = byte_received; 
  }

  got_CMD = TRUE;

  printf("I JUST RECEIVED THIS COMMAND: %d | %d | %d | %d | %d \n", buf[0], buf[1], buf[2], buf[3], buf[4]);
  return res;
}*/

int read_cmd(char* cmd){
  char byte_received;
  int res;
  messageState state = START;
  //should check the value of BCC in order to see if we can move on to BCC_OK state
  static int is_bcc_okay = 0;

  while(TRUE){
    if(res=read(fd,&byte_received,1) < 0){
      printf("ERROR\n");
      return res;
    }

  switch(state){
    case START:
      if(byte_received == FLAG){
        is_bcc_okay = 0;
        state=FLAG_RCV;
      }
      break;
    case FLAG_RCV:
      if(byte_received == A_E || byte_received == A_R){
        is_bcc_okay ^= byte_received;
        state=A_RCV;
      }
      if(byte_received != FLAG){ //according to the teacher's state machine if it's a FLAG we should stay in this state if not we should go to START
        state = START;
      }
      break;
    case A_RCV:
      if(byte_received == C_UA || byte_received == C_REJ_ONE || byte_received == C_REJ_ZERO || byte_received == C_RR_ONE || byte_received == C_RR_ZERO || byte_received == C_SET || byte_received == C_DISC){
        *cmd = byte_received; //need this to know how the previous message was received
        is_bcc_okay ^= byte_received;
        state=C_RCV;
      }
      else if(byte_received == FLAG){
        state=FLAG_RCV;
      }
      else{
        state = START;
      }
      break;
    case C_RCV:
      is_bcc_okay ^=byte_received;
      if(is_bcc_okay == 0){
        state= BCC_OK;
      }
      else if(byte_received == FLAG){
        state=FLAG_RCV;
      }
      else{
        state = START;
      }
      break;
    case BCC_OK:
      if(byte_received == FLAG){
        state=STOP;
      }
      else{
        state=START;
      }
      break;
    case STOP:
      res=0;
      return res;
  }
  }
}

void atende() // atende alarme--->emissor(writenonical.c)
{
  if (!got_CMD)
  {
    send_cmd(2, TRANSMITTER);
    printf("alarme # %d\n", conta);
    conta++;
    if (conta < 4)
    {
      alarm(3);
    }
    else
    {
      printf("ERROR - Timeout over. It wasn't possible to receive command successfully\n");
      exit(1);
    }
  }
}

/*
int main(int argc, char **argv)
{
  int c;
  struct termios oldtio, newtio;
  int i, sum = 0, speed = 0;

  if ((argc < 2) ||
      ((strcmp("/dev/ttyS10", argv[1]) != 0) &&
       (strcmp("/dev/ttyS11", argv[1]) != 0)))
  {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

  // /dev/ttyS10 is emissor and /dev/ttyS11 is receiver

  

  fd = open(argv[1], O_RDWR | O_NOCTTY);
  if (fd < 0)
  {
    perror(argv[1]);
    exit(-1);
  }

  if (tcgetattr(fd, &oldtio) == -1)
  { 
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 0; 
  newtio.c_cc[VMIN] = 5;  
  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n");

  if (strcmp("/dev/ttyS10", argv[1]) == 0) // Emissor
  {
    signal(SIGALRM, atende); //instala rotina que atende interrupção

    send_cmd(0, TRANSMITTER); //Send SET

    alarm(3);

    read_cmd(); //Receive UA
  }

  if (strcmp("/dev/ttyS11", argv[1]) == 0) // Recetor
  {
    read_cmd();

    //Check if SET is correct--->Needs to be changed--->idk if we will keep this...
    u_int8_t ACK = buf[1] ^ buf[2] ^ buf[3];
    u_int8_t ua[5] = {FLAG, A_E, C_SET, BCC_SET, FLAG};

    if (ACK == 0x00)
    { //Send UA
      send_cmd(2, RECEIVER);
    }
    else
    { //the message isn't correct
      printf("ERROR\n");
    }
  }

  sleep(1);
  if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }
  close(fd);
  return 0;
}*/