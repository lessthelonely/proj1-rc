#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "data_protocol.h"
#include "constants.h"
#include "app.h"
#include "alarm.h"

int send_cmd(int command, int sender)
{ 
/*Function to use for every cmd except when it's an info trama*/
  u_int8_t cmd[5];
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
      cmd[0] = FLAG;
      cmd[1] = A_E;
      cmd[2] = C_RR_ONE;
      cmd[3] = BCC_RR_ONE;
      cmd[4] = FLAG;
      break;
    case 4: // RR
      cmd[0] = FLAG;
      cmd[1] = A_E;
      cmd[2] = C_RR_ZERO;
      cmd[3] = BCC_RR_ZERO;
      cmd[4] = FLAG;
      break;
    case 5:
      cmd[0] = FLAG;
      cmd[1] = A_E;
      cmd[2] = C_REJ_ONE;
      cmd[3] = BCC_REJ_ONE;
      cmd[4] = FLAG;
      break;
    case 6:
      cmd[0] = FLAG;
      cmd[1] = A_E;
      cmd[2] = C_REJ_ZERO;
      cmd[3] = BCC_REJ_ZERO;
      cmd[4] = FLAG;
      break;
    }
  }

  printf("I'M SENDING THIS COMMAND: %02x | %02x | %02x | %02x | %02x \n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
  int res = write(app_info.fileDescriptor, cmd, 5);
  if (res == -1)
  {
    printf("ERROR IN SENDING.\n");
  }
  return res;
}

int read_info_trama(u_int8_t *info_trama, u_int8_t *cmd)
{
  int r=-1;
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
      if (byte_received == FLAG)
      {
        is_bcc_okay = 0;
        state = FLAG_RCV;
      }
      break;
    case FLAG_RCV:
      if (byte_received == A_E)
      { //info trama is only sent by TRANSMITTER
        is_bcc_okay ^= byte_received;
        state = A_RCV;
      }
      else if (byte_received != FLAG)
      { 
        state = START;
      }
      break;
    case A_RCV:
      //C  Campo de Controlo 0 S 0 0 0 0 0 0 S = N(s) -->slide 7
      if (byte_received == C_I_ONE || byte_received == C_I_ZERO)
      {
        is_bcc_okay ^= byte_received;
        cmd = byte_received;
        state = C_RCV;
      }
      else if (byte_received == FLAG)
      {
        state = FLAG_RCV;
      }
      else
      {
        state = START;
      }
      break;
    case C_RCV:
      is_bcc_okay ^= byte_received;
      if (is_bcc_okay == 0)
      {
        state = BCC_OK;
      }
      else if (byte_received == FLAG)
      {
        state = FLAG_RCV;
      }
      else
      {
        state = START;
      }
      break;
    case BCC_OK:
      if (byte_received == FLAG)
      {
        state = FLAG_RCV;
      }
      else
      {
        info_trama[r] = byte_received;
        r++;
        state = D_RCV;
      }
      break;
    case D_RCV:
      if (byte_received == FLAG)
      {
        state = STOP;
        return r;
      }
      else
      {
        info_trama[r] = byte_received;
        r++;
      }
      break;
    case STOP:
      return r;
    }
  }
  return -1;
}

int read_cmd(u_int8_t*cmd)
{
  u_int8_t byte_received;
  messageState state = START;
  //should check the value of BCC in order to see if we can move on to BCC_OK state
  static int is_bcc_okay = 0;

  while (TRUE)
  {

    if (read(app_info.fileDescriptor, &byte_received, 1) < 0){
      return -1;
    }

    switch (state)
    {
    case START:
      if (byte_received == FLAG)
      {
        is_bcc_okay = 0;
        state = FLAG_RCV;
      }
      break;
    case FLAG_RCV:
      if (byte_received == A_E || byte_received == A_R)
      {
        is_bcc_okay ^= byte_received;
        state = A_RCV;
      }
      else if (byte_received != FLAG)
      { //according to the teacher's state machine if it's a FLAG we should stay in this state if not we should go to START
        state = START;
      }
      break;
    case A_RCV:
      if (byte_received == C_UA || byte_received == C_REJ_ONE || byte_received == C_REJ_ZERO || byte_received == C_RR_ONE || byte_received == C_RR_ZERO || byte_received == C_SET || byte_received == C_DISC)
      {
        *cmd = byte_received; //need this to know how the previous message was received
        is_bcc_okay ^= byte_received;
        state = C_RCV;
      }
      else if (byte_received == FLAG)
      {
        state = FLAG_RCV;
      }
      else
      {
        state = START;
      }
      break;
    case C_RCV:
      is_bcc_okay ^= byte_received;
      if (is_bcc_okay == 0)
      {
        state = BCC_OK;
      }
      else if (byte_received == FLAG)
      {
        state = FLAG_RCV;
      }
      else
      {
        state = START;
      }
      break;
    case BCC_OK:
      if (byte_received == FLAG)
      {
        state = STOP;
        return 0;
      }
      else
      {
        state = START;
      }
      break;
    }
  }
  return -1;
}
