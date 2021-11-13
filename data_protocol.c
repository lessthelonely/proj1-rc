/*Non-Canonical Input Processing*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include "constants.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP = FALSE;
int got_CMD = FALSE;
u_int8_t BCC; //BCC=XOR(A,C)

int flag = 1, conta = 1;
int fd;
u_int8_t cmd[5];
u_int8_t buf[255];

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
    }
  } else if (sender == RECEIVER) { // Sender
    switch (command) {
      case 0: // DISC
        cmd[0] = FLAG; cmd[1] = A_R; cmd[2] = C_DISC;
        cmd[3] = BCC_DISC_R; cmd[4] = FLAG;
        break;
      case 1: // UA
        BCC = A_R ^ C_UA;
        cmd[0] = FLAG; cmd[1] = A_R; cmd[2] = C_UA;
        cmd[3] = BCC; cmd[4] = FLAG;
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

int read_cmd()
{ //emissor (writenoncanonical.c)
  int i = 0;
  int res;

  while (STOP == FALSE)
  { /* loop for input */
    u_int8_t byte_received;
    res = read(fd, &byte_received, 1); /* returns after 5 chars have been input */
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
    buf[i++] = byte_received; /* so we can printf... */
  }

  got_CMD = TRUE;

  printf("I JUST RECEIVED THIS COMMAND: %d | %d | %d | %d | %d \n", buf[0], buf[1], buf[2], buf[3], buf[4]);
  return res;
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

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

  fd = open(argv[1], O_RDWR | O_NOCTTY);
  if (fd < 0)
  {
    perror(argv[1]);
    exit(-1);
  }

  if (tcgetattr(fd, &oldtio) == -1)
  { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
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
}