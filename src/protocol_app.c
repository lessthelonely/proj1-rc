
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>

#include "constants.h"
#include "data_protocol.h"
#include "stuffing.h"
#include "protocol_app.h"
#include "alarm.h"

struct termios newtio, oldtio;

int llopen()
{ //Don't need any of the arguments in the slides because all the data is stored in data structures 
  if (link_info.status != TRANSMITTER && link_info.status != RECEIVER)
  { //sender will be TRANSMITTER or RECEIVER
    printf("ERROR");
    return -1;
  }

  u_int8_t cmd;
  int res = -1;

  int fd = open(link_info.port, O_RDWR | O_NOCTTY);
  link_info.fileDescriptor=fd;
    if (fd < 0)
    {
        perror(link_info.port);
        return -1;
    }

  if (tcgetattr(fd, &oldtio) == -1)
  {
    printf("ERROR\n");
    return -1;
  }

  bzero(&newtio, sizeof(newtio));
  if(link_info.baudRate==-1){
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  }
  else{
    newtio.c_cflag = link_info.baudRate | CS8 | CLOCAL | CREAD;
  }
  
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 1;  /* blocking read until 1 chars received */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1)
  {
    printf("ERROR\n");
    return -1;
  }

  if (link_info.status == TRANSMITTER)
  {
    while (res != 0)
    {
      alarm(link_info.timeout);
      if (send_cmd(0, TRANSMITTER, fd) < 0)
      { //Send SET
        printf("ERROR\n");
      }
      else
      {
        printf("Wrote SET\n");
      }

      if ((res = read_cmd(&cmd, fd)) >= 0){
        printf("Received UA\n");
      }
    }

    if (res == 0){
      deactivate_alarm(); //We got a response so timeout is over
    }
      
  }

  else 
  {
    while (res < 0)
    {
      read_cmd(&cmd, fd);
      printf("Read SET\n");

      if ((res = send_cmd(2, TRANSMITTER, fd)) < 0)
      {
        printf("ERROR\n");
      }
      else
      {
        printf("Sent UA\n");
      }
    }
  }
  return 0;
}

int llwrite(u_int8_t *buffer, int length)
{
  
/*Orders protocol to send the Info trauma
char* buffer is char array we want to transmit so it will the be data part of the info trauma?
In order to make this function work->need to build the information trama
F A C BCC1 buffer BCC2 F
TRANSMITTER is the only one who calls this function
*/
  
  static int seqNum = 0; //keeps track of sequence number
  static int repeat=0;
  u_int8_t *trama = (u_int8_t *)malloc(MAX_FRAME_SIZE * sizeof(u_int8_t)); //Allocs space to write info trama
  u_int8_t *copy = (u_int8_t *)malloc(MAX_FRAME_SIZE * sizeof(u_int8_t)); //Allocs space to write info trama

  if (length < 0)
  {
    printf("Value should be positive in order to actually transfer data\n");
    free(trama);
    return -1;
  }
  int write_length;
  u_int8_t cmd;
  memcpy(copy,buffer,length);
  //Need to create info trauma + send it
  while (TRUE)
  { 
    if(repeat){
      memcpy(buffer,copy,length); 
      memcpy(copy,buffer,length);
    }
    memset(trama, 0, strlen(trama)); //initialize trama array
    write_length = create_info_trama(buffer, trama, length,seqNum); 
    /*In create_info_trama, we use memcpy to extract the info in buffer and put it in trama.
      To make sure we have information to resend (in case receiver sends out a REJ as an ACK), 
      we make a new array (copy) and we copy the buffer's info to it before the cycle starts so 
      that it stays safe, before we call create_info_trama, in case it's not the first iteration 
      (verified by int gotREJ) we copy the info in copy to buffer and vice-versa (for the future)  */
 
    alarm(link_info.timeout);
    if (write(link_info.fileDescriptor, trama, write_length) < 0)
    {
      printf("ERROR");
    }
    else
    {
      printf("Sent message with sequence number %d\n", seqNum);
    }

    if (read_cmd(&cmd, link_info.fileDescriptor) < 0)
    {
      printf("ERROR");
    }
    else
    {
      printf("Read ACK with sequence number %d\n", !seqNum);
    }
    
    //RR->means it was accepted
    if ((cmd == C_RR_ONE && seqNum == 0) || (cmd == C_RR_ZERO && seqNum == 1))
    {
      deactivate_alarm();
      if(seqNum == 0){
        seqNum=1;
      }
      else{
        seqNum=0;
      }
      free(trama);
      repeat=0;
      return write_length;
    }

    if (cmd == C_REJ_ZERO || cmd == C_REJ_ONE)
    {
      deactivate_alarm();
      printf("Received REJ\n");//Need to retransmit message
      repeat = 1;
    }
    repeat = 1; //We didn't receive any ACK from the receiver so we are gonna resend information x times
  }
}

void check_BCC2(u_int8_t * info_trama, u_int8_t* BCC2, int length)
{
  //Creates BCC2 by doing the XOR operation with all the components of the trama(data) array
    for (int i = 0 ; i < length; i++){
        *BCC2 ^= info_trama[i];         
    } 
}


int llread(u_int8_t *buffer)
{
   /* RECEIVER is the only one who calls this function
  */
  static int seqNum=0; //keeps track of the sequence Number
  int length;
  u_int8_t cmd;

  while (TRUE)
  {
    //read info trama-->can't use read_cmd because of the data segment
    
    if ((length = read_info_trama(buffer,&cmd, link_info.fileDescriptor)) < 0)
    {
    }
    else
    {
      printf("Read info message with sequence number %d\n", seqNum);
    }

    //Need to destuff before storing
    int new_length=destuffing(buffer, length);

    //Check if BCC2 is correct, if not dump info
    //First we create BCC2 based on the info and then we check if it matches up with the last bit of info trama (that should be BCC2)
    u_int8_t BCC2;
    BCC2 = 0x00;
    check_BCC2(buffer,&BCC2,new_length-1);

    /*Okay let's interpret slide 11:
    We need to check if BCC2 is right however maybe this is not the first thing we need to do
    We send REJ if BCC2 is wrong and if info is new
    We send RR: 1) if BCC2 is wrong but info is duplicated
                2) if BCC2 is right but info is duplicated-->just dump it afterwards
                3) if BCC2 is right and info is new-->save it
     */

    int bcc2_is_not_okay = FALSE;
    if (BCC2 != buffer[new_length - 1])
    {
      printf("Wrong BCC2 - gonna send negative ACK (REJ)\n");
      bcc2_is_not_okay = TRUE;
    }

    //Info is duplicated
    if ((cmd == C_I_ZERO && seqNum == 1) || (cmd == C_I_ONE && seqNum == 0))
    {
       //Send RR - Slide 14
        if (cmd==C_I_ZERO) //I(Ns=0)
        {
          send_cmd(3, TRANSMITTER, fd); //RR(Nr=1)
        }
        else //I(Ns=1)
        {
          send_cmd(4, TRANSMITTER, fd);//RR(Nr=0)
        }
      }
    else
    { //info is new
      if (bcc2_is_not_okay)
      {
        //Send REJ
        if (cmd==C_I_ZERO) //I(Ns=0)
        {
          send_cmd(5, TRANSMITTER, fd); //REJ(Nr=1)
        }
        else //I(Ns=1)
        { 
          send_cmd(6, TRANSMITTER, fd); //REJ(Nr=0)
        }
        continue;
      }
      else
      {
        //Send RR + store info
        //Change sequence number
        if (seqNum==0) //I(Ns=0)
        {
          send_cmd(3, TRANSMITTER, fd); //RR(Nr=1)
          seqNum=1;
        }
        else //I(Ns=1)
        {
          send_cmd(4, TRANSMITTER, fd);//RR(Nr=0)
          seqNum=0;
        }
        return length;
      }
    }
  }
  return -1;
}

int llclose() //Don't need any of the arguments in the slides because all the data is stored in data structures 
{
  int cmd_received = FALSE;
  u_int8_t cmd;
  if (link_info.status!= TRANSMITTER && link_info.status != RECEIVER)
  { //sender will be TRANSMITTER or RECEIVER
    printf("ERROR\n");
  }

  if (link_info.status == TRANSMITTER)
  {
    while (!cmd_received)
    {
      alarm(link_info.timeout);
      if (send_cmd(1, TRANSMITTER, link_info.fileDescriptor) < 0) //Send DISC
      {
        printf("ERROR\n");
      }

      if (read_cmd(&cmd, link_info.fileDescriptor) < 0) //Read DISC from Receiver
      {
        printf("Try again\n");
      }
      if (cmd == C_DISC)
      {
        deactivate_alarm();
        cmd_received = TRUE;
      }

      if (send_cmd(2, TRANSMITTER, link_info.fileDescriptor) < 0) //Send UA
      {
        printf("ERROR\n");
      }
    }
  }
  else
  {
    while (!cmd_received)
    {
      if (read_cmd(&cmd, link_info.fileDescriptor) < 0) //Read DISC
      {
        printf("ERROR\n");
        continue;
      }
      else
      {
        if (cmd == C_DISC)
        {
          cmd_received = TRUE;
        }
      }

      if (send_cmd(1, TRANSMITTER, link_info.fileDescriptor) < 0) //Send DISC back
      {
        printf("ERROR\n");
        continue;
      }

      if (read_cmd(&cmd, link_info.fileDescriptor) < 0) //Read UA
      {
        printf("ERROR\n");
        continue;
      }
      else
      {
        if (cmd == C_UA)
        {
          cmd_received = TRUE;
        }
      }
    }
  }

  //close fd
  close(link_info.fileDescriptor);
  return 0;
}

int create_info_trama(u_int8_t *buffer, u_int8_t*trama, int length,int seqNum)
{
  //Let's start by defining BCC2-->it will need to be stuffed (like data) but according to slide 7 and 13, it is created before
  u_int8_t* BCC2 =(u_int8_t*)malloc(sizeof(u_int8_t));
  BCC2[0] = 0x00; //neutral operand
  check_BCC2(buffer,BCC2,length);

  //We need to stuff the data + BCC2
  int bcc2_length=1;
  int new_bcc2_length = stuffing(BCC2, bcc2_length); 
  int new_data_length = stuffing(buffer, length);

  //Assemble info trama
  int new_length = new_data_length + new_bcc2_length + 5; //5 because F A C BCC1 F
  trama[0] = FLAG;
  trama[1] = A_E;
  if (seqNum == 0)
  {
    trama[2] = C_I_ZERO;
    trama[3] = BCC_C_I_ZERO;
  }
  else
  {
    trama[2] = C_I_ONE;
    trama[3] = BCC_C_I_ONE;
  }

  memcpy(&trama[4],buffer,new_data_length);


  int index_bcc2 = new_data_length + 4;
  memcpy(&trama[index_bcc2], BCC2, new_bcc2_length);
  trama[new_length - 1] = FLAG; //second flag is at the end of the frame
  return new_length;
}
