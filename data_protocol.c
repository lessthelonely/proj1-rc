#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "../include/data_protocol.h"
#include "../include/constants.h"
#include "../include/app.h"
#include "../include/alarm.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

u_int8_t cmd[5];
u_int8_t buf[255];


/*Function to use for every cmd except when it's an info trama*/
int send_cmd(int command, int sender)
{ //emissor (writenoncanonical.c)
  if (sender == TRANSMITTER)
  { //Emitter
    switch (command)
    {
    case 0: // SET
      cmd[0] = FLAG;
      cmd[1] = A_E;
      cmd[2] = C_SET;
      cmd[3] = BCC_SET;
      cmd[4] = FLAG;
      break;
    case 1: // DISC
      cmd[0] = FLAG;
      cmd[1] = A_E;
      cmd[2] = C_DISC;
      cmd[3] = BCC_DISC_E;
      cmd[4] = FLAG;
      break;
    case 2: // UA
      cmd[0] = FLAG;
      cmd[1] = A_E;
      cmd[2] = C_UA;
      cmd[3] = BCC_UA_E;
      cmd[4] = FLAG;
      break;
    case 3: // RR
      printf("LET'S GOOOOO 3 \n");
      cmd[0] = FLAG;
      cmd[1] = A_E;
      cmd[2] = C_RR_ONE;
      cmd[3] = BCC_RR_ONE;
      cmd[4] = FLAG;
      break;
    case 4: // RR
      printf("LET'S GOOOOO 4 \n");
      cmd[0] = FLAG;
      cmd[1] = A_E;
      cmd[2] = C_RR_ZERO;
      cmd[3] = BCC_RR_ZERO;
      cmd[4] = FLAG;
      break;
    case 5:
      printf("LET'S GOOOOO 5 \n");
      cmd[0] = FLAG;
      cmd[1] = A_E;
      cmd[2] = C_REJ_ONE;
      cmd[3] = BCC_REJ_ONE;
      cmd[4] = FLAG;
      break;
    case 6:
      printf("LET'S GOOOOO 6 \n");
      cmd[0] = FLAG;
      cmd[1] = A_E;
      cmd[2] = C_REJ_ZERO;
      cmd[3] = BCC_REJ_ZERO;
      cmd[4] = FLAG;
      break;
    }
  }
  else if (sender == RECEIVER)
  { // Sender
    switch (command)
    {
    case 0: // DISC
      cmd[0] = FLAG;
      cmd[1] = A_R;
      cmd[2] = C_DISC;
      cmd[3] = BCC_DISC_R;
      cmd[4] = FLAG;
      break;
    case 1: // UA
      cmd[0] = FLAG;
      cmd[1] = A_R;
      cmd[2] = C_UA;
      cmd[3] = BCC_UA_R;
      cmd[4] = FLAG;
      break;
    case 2: // RR
      cmd[0] = FLAG;
      cmd[1] = A_E;
      cmd[2] = C_RR_ONE;
      cmd[3] = BCC_RR_ONE;
      cmd[4] = FLAG;
      break;
    case 3: // RR
      cmd[0] = FLAG;
      cmd[1] = A_E;
      cmd[2] = C_RR_ZERO;
      cmd[3] = BCC_RR_ZERO;
      cmd[4] = FLAG;
      break;
    case 4:
      cmd[0] = FLAG;
      cmd[1] = A_E;
      cmd[2] = C_REJ_ONE;
      cmd[3] = BCC_REJ_ONE;
      cmd[4] = FLAG;
      break;
    case 5:
      cmd[0] = FLAG;
      cmd[1] = A_E;
      cmd[2] = C_REJ_ZERO;
      cmd[3] = BCC_REJ_ZERO;
      cmd[4] = FLAG;
      break;
    }
  }

  printf("I'M SENDING THIS COMMAND: %d | %d | %d | %d | %d \n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
  int res = write(app_info.fileDescriptor, cmd, 5);
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
int read_info_trama(u_int8_t *info_trama, u_int8_t *cmd)
{
  int r=-1;
  printf("I'm in read_info_trama\n");
  u_int8_t byte_received;
  messageState state = START;
  //should check the value of BCC in order to see if we can move on to BCC_OK state
  static int is_bcc_okay = 0;

  while (TRUE)
  {
    if (read(app_info.fileDescriptor, &byte_received, 1) < 0)
    {
      printf("ERROR\n");
      return -1;
    }

    switch (state)
    {
    case START:
      r=0;
      printf("RES %d\n",r);
      printf("START %02x\n", byte_received);
      if (byte_received == FLAG)
      {
        is_bcc_okay = 0;
        state = FLAG_RCV;
        printf("LEAVING START\n");

      }
      break;
    case FLAG_RCV:
      printf("RES %d\n",r);
      printf("IN FLAG %02x\n", byte_received);
      if (byte_received == A_E)
      { //info trama is only sent by TRANSMITTER
        is_bcc_okay ^= byte_received;
        state = A_RCV;
        printf("LEAVING FLAG\n");
      }
      else if (byte_received != FLAG)
      { //according to the teacher's state machine if it's a FLAG we should stay in this state if not we should go to START
        state = START;
        printf("GOING START\n");
      }
      break;
    case A_RCV:
      printf("RES %d\n",r);
      //C  Campo de Controlo 0 S 0 0 0 0 0 0 S = N(s) -->slide 7
       printf("IN A %02x\n", byte_received);
      if (byte_received == C_I_ONE || byte_received == C_I_ZERO)
      {
        is_bcc_okay ^= byte_received;
        cmd = byte_received;
        state = C_RCV;
        printf("LEAVING A\n");
      }
      else if (byte_received == FLAG)
      {
        state = FLAG_RCV;
        printf("GOING TO FLAG\n");
      }
      else
      {
        state = START;
      }
      break;
    case C_RCV:
      printf("RES %d\n",r);
      printf("IN C %02x\n", byte_received);
      is_bcc_okay ^= byte_received;
      if (is_bcc_okay == 0)
      {
        state = BCC_OK;
        printf("LEAVING C\n");
      }
      else if (byte_received == FLAG)
      {
        state = FLAG_RCV;
        printf("GOING FLAG\n");
      }
      else
      {
        state = START;
        printf("GOING START\n");
      }
      break;
    case BCC_OK:
      printf("RES %d\n",r);
      printf("IN BCC_OK %02x\n", byte_received);
      if (byte_received == FLAG)
      {
        state = FLAG_RCV;
      }
      else
      {
        printf("RES %d\n",r);
        info_trama[r] = byte_received;
        r++;
        printf("RES %d\n",r);
        state = D_RCV;
      }
      break;
    case D_RCV:
      printf("RES %d\n",r);
      printf("IN D %02x\n", byte_received);
      if (byte_received == FLAG)
      {
        state = STOP;
        printf("LEAVING BCC_OK\n");
        printf("BYE BYE BYE\n");
        printf("RES %d\n",r);
        return r;
      }
      else
      {
        printf("RES %d\n",r);
        info_trama[r] = byte_received;
        r++;
        printf("RES %d\n",r);
      }
      break;
    case STOP:
      return r;
    }
  }
  return -1;
}

int read_cmd(int fd, u_int8_t*cmd)
{
  u_int8_t byte_received;
  messageState state = START;
  //should check the value of BCC in order to see if we can move on to BCC_OK state
  static int is_bcc_okay = 0;
  printf("--READ FRAME [UA, DISC, SET]--\n");

  while (TRUE)
  {

    if (read(fd, &byte_received, 1) == -1)
      return -1;

    printf("ENTERING STATE MACHINE\n");

    switch (state)
    {
    case START:
      printf("%02x\n", byte_received);
      if (byte_received == FLAG)
      {
        is_bcc_okay = 0;
        state = FLAG_RCV;
        printf("LEAVING START\n");
      }
      break;
    case FLAG_RCV:
      //deactivate_alarm();
      printf("IN FLAG\n");
      printf("%02x\n", byte_received);
      if (byte_received == A_E || byte_received == A_R)
      {
        is_bcc_okay ^= byte_received;
        state = A_RCV;
        printf("LEAVING FLAG\n");
      }
      else if (byte_received != FLAG)
      { //according to the teacher's state machine if it's a FLAG we should stay in this state if not we should go to START
        state = START;
        printf("GOING START\n");
      }
      break;
    case A_RCV:
      printf("%02x\n", byte_received);
      if (byte_received == C_UA || byte_received == C_REJ_ONE || byte_received == C_REJ_ZERO || byte_received == C_RR_ONE || byte_received == C_RR_ZERO || byte_received == C_SET || byte_received == C_DISC)
      {
        *cmd = byte_received; //need this to know how the previous message was received
        is_bcc_okay ^= byte_received;
        state = C_RCV;
        printf("LEAVING A\n");
      }
      else if (byte_received == FLAG)
      {
        state = FLAG_RCV;
        printf("GOING TO FLAG\n");
      }
      else
      {
        state = START;
        printf("GOING START\n");
      }
      break;
    case C_RCV:
      printf("%02x\n", byte_received);
      is_bcc_okay ^= byte_received;
      if (is_bcc_okay == 0)
      {
        state = BCC_OK;
        printf("LEAVING C\n");
      }
      else if (byte_received == FLAG)
      {
        state = FLAG_RCV;
        printf("GOING FLAG\n");
      }
      else
      {
        state = START;
        printf("GOING START\n");
      }
      break;
    case BCC_OK:
      printf("%02x\n", byte_received);
      if (byte_received == FLAG)
      {
        state = STOP;
        printf("LEAVING BCC_OK\n");
        printf("BYE BYE BYE\n");
        return 0;
      }
      else
      {
        state = START;
        printf("GOING START\n");
      }
      break;
    }
  }
  return -1;
}

int read_frame_supervision(int fd, u_int8_t *CMD){
   int curr_state = 0; /* byte that is being read. From 0 to 4.*/
  u_int8_t byte;

    printf("--READ SUPERVISION FRAME [RR, REJ]--\n"); 
    while (TRUE)
    {
        if (read(fd, &byte, 1) == -1) 
            return -1;

        switch (curr_state)
        {
        // RECEIVE FLAG
        case 0: 

            printf("case 0: %02x\n", byte);
            if (byte == FLAG)
                curr_state++;
            break;

        // RECEIVE ADDR
        case 1:
            printf("case 1: %02x\n", byte);
            if (byte == A_E)
                curr_state++;
            else if (byte != FLAG)
                curr_state = 0;
            break;

        // RECEIVE CMD
        case 2:
            printf("case 2: %02x\n", byte);
            if (byte == C_REJ_ONE || byte == C_RR_ONE || byte == C_RR_ZERO || byte == C_REJ_ZERO){
                *CMD = byte; 
                printf("%02x\n", *CMD);
                curr_state++;
            } 
            else if (byte == FLAG)
                curr_state = 1;
            else
                curr_state = 0;
            break;
        // RECEIVE BCC
        case 3:
            printf("case 3: %02x\n", byte);
            if (byte == (*CMD ^ A_E))
                curr_state++;
            else if (byte == FLAG)
                curr_state = 1;
            else
                curr_state = 0;
            break;

        // RECEIVE FLAG
        case 4:
            printf("case 4: %02x\n", byte);
            if (byte == FLAG) 
                return 0; 
            else
                curr_state = 0;
        }
    }
    return curr_state;
}

int read_test(int fd, u_int8_t *cmd)
{
  u_int8_t byte_received;
  messageState state = START;
  //should check the value of BCC in order to see if we can move on to BCC_OK state
  static int is_bcc_okay = 0;
  printf("--READ FRAME [UA, DISC, SET]--\n");

  while (TRUE)
  {

    if (read(fd, &byte_received, 1) == -1)
      return -1;

    printf("ENTERING STATE MACHINE\n");

    switch (state)
    {
    case START:
      printf("%02x\n", byte_received);
      if (byte_received == FLAG)
      {
        is_bcc_okay = 0;
        state = FLAG_RCV;
        printf("LEAVING START\n");
      }
      break;
    case FLAG_RCV:
      //deactivate_alarm();
      printf("IN FLAG\n");
      printf("%02x\n", byte_received);
      if (byte_received == A_E || byte_received == A_R)
      {
        is_bcc_okay ^= byte_received;
        state = A_RCV;
        printf("LEAVING FLAG\n");
      }
      else if (byte_received != FLAG)
      { //according to the teacher's state machine if it's a FLAG we should stay in this state if not we should go to START
        state = START;
        printf("GOING START\n");
      }
      break;
    case A_RCV:
      printf("%02x\n", byte_received);
      if (byte_received == C_UA || byte_received == C_REJ_ONE || byte_received == C_REJ_ZERO || byte_received == C_RR_ONE || byte_received == C_RR_ZERO || byte_received == C_SET || byte_received == C_DISC)
      {
        *cmd = byte_received; //need this to know how the previous message was received
        is_bcc_okay ^= byte_received;
        state = C_RCV;
        printf("LEAVING A\n");
      }
      else if (byte_received == FLAG)
      {
        state = FLAG_RCV;
        printf("GOING TO FLAG\n");
      }
      else
      {
        state = START;
        printf("GOING START\n");
      }
      break;
    case C_RCV:
      printf("%02x\n", byte_received);
      is_bcc_okay ^= byte_received;
      if (is_bcc_okay == 0)
      {
        state = BCC_OK;
        printf("LEAVING C\n");
      }
      else if (byte_received == FLAG)
      {
        state = FLAG_RCV;
        printf("GOING FLAG\n");
      }
      else
      {
        state = START;
        printf("GOING START\n");
      }
      break;
    case BCC_OK:
      printf("%02x\n", byte_received);
      if (byte_received == FLAG)
      {
        state = STOP;
        printf("LEAVING BCC_OK\n");
        printf("BYE BYE BYE\n");
        return 0;
      }
      else
      {
        state = START;
        printf("GOING START\n");
      }
      break;
    }
  }
  return -1;
}