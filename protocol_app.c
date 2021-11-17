#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>

#include "../include/constants.h"
#include "../include/data_protocol.h"
#include "../include/stuffing.h"
#include "../include/app.h"
#include "../include/protocol_app.h"
#include "../include/alarm.h"

struct termios newtio, oldtio;

/*Returns 0 or -1 in case of error*/
int llopen(char *porta, int sender)
{ //Slides uses int porta but it's more practical in char* because it's serial ports are in char*

  if (sender != TRANSMITTER && sender != RECEIVER)
  { //sender will be TRANSMITTER or RECEIVER
    printf("ERROR");
    return -1;
  }

  char *cmd[1];
  int res = -1;

  if (tcgetattr(app_info.fileDescriptor, &oldtio) == -1)
  {
    printf("ERROR\n");
    return -1;
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 1;  /* blocking read until 1 chars received */

  tcflush(app_info.fileDescriptor, TCIOFLUSH);

  if (tcsetattr(app_info.fileDescriptor, TCSANOW, &newtio) == -1)
  {
    printf("ERROR\n");
    return -1;
  }

  if (sender == TRANSMITTER)
  {
    while (res != 0)
    {
      alarm(link_info.timeout);
      if (send_cmd(0, TRANSMITTER) < 0)
      { //Send SET
        printf("ERROR");
      }
      else
        printf("Written CMD_SET.");

      if ((res = read_cmd(app_info.fileDescriptor, cmd)) >= 0)
        printf("Received UA.");
    }

    if (res == 0)
      deactivate_alarm();
  }

  else if (sender == RECEIVER)
  {
    //Set the file descriptor.

    while (res < 0)
    {
      // Establishment of the connection.
      read_cmd(app_info.fileDescriptor, cmd);
      printf("Received CMD_SET with success.");

      if ((res = send_cmd(2, TRANSMITTER)) < 0)
      {
        printf("ERROR\n");
      }
      else
      {
        printf("Sent UA with success\n");
      }
    }
  }
  return 0;
}

/*Orders protocol to send the Info trauma
char* buffer is char array we want to transmit so it will the be data part of the info trauma?
In order to make this function work->need to build the information trama
F A C BCC1 char*buffer? BCC2 F

Return value should -1 in case of error or number of caracters written

TRANSMITTER is the only one who calls this function
*/
int llwrite(int fd, char *buffer, int length)
{
  printf("I'm in llwrite\n");
  char *trama = (char *)malloc(MAX_SIZE * sizeof(char)); //Allocs space to write info trama
  memset(trama, 0, strlen(trama));                       //initialize trama array
  if (length < 0)
  {
    printf("Value should be positive in order to actually transfer data\n");
    free(trama);
    return -1;
  }
  int write_length;
  char *cmd;
  //Need to create info trauma + send it(should use the timeout mechanic here right? Might need to put alarm in a different file and change the routine)
  while (TRUE)
  { //might need a better condition-->thought for later
    //We initilized trauma array with the biggest possible size (MAX_SIZE) however most times, there won't actually be 255 bits to be written so we need the actual correct number in order to return it to fulfill the function's purpose
    write_length = create_info_trama(buffer, trama, length);

    //not gonna call send_cmd because info trama is a special case
    if (write(fd, trama, write_length) < 0)
    {
      printf("ERROR");
    }
    else
    {
      //Okay only TRANSMITTER sends info trama however it can send sequence number 0 or 1
      //Sends sequence number 0 first and then number 1 (slide 14 Ns=0/1)
      printf("TRANSMITTER sent sucessfully sequence number %d\n", link_info.sequenceNumber);
    }
    alarm(link_info.timeout);

    //need to change read_cmd in order for it to return the command that was written
    //Not the return...the return still needs to be an int in order to see if there was an error or not
    //I need to add a parameter to read_cmd, maybe a pointer?
    //need to know if receiver sent back a RR or REJ
    //do I need to check if RR_one is sent when sequence number is one and all that?
    //Maybe it's better

    if (read_cmd(app_info.fileDescriptor, cmd) < 0)
    {
      printf("ERROR");
    }
    else
    {
      printf("Command was read sucessfully sequence number %d\n", link_info.sequenceNumber);
    }

    //RR->means it was accepted
    if ((cmd == C_RR_ZERO && link_info.sequenceNumber == 0) || (cmd == C_RR_ONE && link_info.sequenceNumber == 1))
    {
      deactivate_alarm();
      if (link_info.sequenceNumber == 0)
      {
        link_info.sequenceNumber = 1;
      }
      else
      {
        link_info.sequenceNumber = 0;
      }
      free(trama);
      return write_length;
    }

    if ((cmd == C_REJ_ZERO && link_info.sequenceNumber == 0) || (cmd == C_REJ_ONE && link_info.sequenceNumber == 1))
    {
      deactivate_alarm();
      printf("Received REJ\n");
      free(trama);
      return -1; //wait is it consider error if we get REJ?
    }
  }
}

/* RECEIVER is the only one who calls this function
   Return length of array of caracters read or -1 in case of error
*/
int llread(int fd, char *buffer)
{
  int length;

  while (TRUE)
  {
    //read info trama-->can't use read_cmd because of the data segment
    char *info_trama, cmd;
    if (length = read_info_trama(info_trama, cmd) < 0)
    {
      //should try to read again?
      //or error?
    }
    else
    {
      printf("Read successfully info message with sequence number %d\n", link_info.sequenceNumber);
    }

    char *frame;
    //Need to destuff before storing
    destuffing(info_trama, length, &frame);

    //Check if BCC2 is correct, if not dump info
    //First we create BCC2 based on the info and then we check if it matches up with the last bit of info trama (that should be BCC2)
    char BCC2 = frame[0];
    for (int i = 1; i < (length - 1); i++)
    {
      BCC2 ^= frame[i];
    }

    /*Okay let's interpret slide 11:
    We need to check if BCC2 is right however maybe this is not the first thing we need to do
    We send REJ if BCC2 is wrong and if info is new
    We send RR: 1) if BCC2 is wrong but info is duplicated
                2) if BCC2 is right but info is duplicated-->just dump it afterwards
                3) if BCC2 is right and info is new-->save it
    
    Maybe let's check if info is duplicated first and then see if BCC2 is wrong
    Never mind, I'll do BCC2 first but think I need a bool 
    so I have no idea what duplicate means
    I thought maybe it was info trama having a C_I_ZERO when the sequence number was one...but not sure  
    */

    int bcc2_is_not_okay = FALSE;
    if (BCC2 != info_trama[length - 1])
    {
      printf("Wrong BCC2-->gonna send negative ACK\n");
      bcc2_is_not_okay = TRUE;
    }

    //Info is duplicated
    if ((cmd == C_I_ZERO && link_info.sequenceNumber == 1) || (cmd == C_I_ONE && link_info.sequenceNumber == 0))
    {
      if (bcc2_is_not_okay)
      {
        //Send RR
        if (link_info.sequenceNumber == 0)
        {
          /*When analyzing the whole program I don't think we will ever use A_R because I don't think receiver sends
        a command without it being an answer to the transmitter but in those cases we should use A_E
        ASK THE TEACHER-->when do we use A_R (00000001 (0x01) em Comandos enviados pelo Receptor e Respostas 
        enviadas pelo Emissor)->what qualifies as this*/
          send_cmd(4, TRANSMITTER);
        }
        else
        {
          send_cmd(3, TRANSMITTER);
        }
      }
      else
      {
        //Send RR & dump info
        if (link_info.sequenceNumber == 0)
        {
          send_cmd(4, TRANSMITTER);
        }
        else
        {
          send_cmd(3, TRANSMITTER);
        }
      }
    }
    else
    { //info is new
      if (bcc2_is_not_okay)
      {
        //Send REJ
        if (link_info.sequenceNumber == 0)
        {
          send_cmd(6, TRANSMITTER);
        }
        else
        {
          send_cmd(5, TRANSMITTER);
        }
      }
      else
      {
        //Send RR + store info
        //Should change sequence number?
        if (link_info.sequenceNumber == 0)
        {
          send_cmd(4, TRANSMITTER);
          link_info.sequenceNumber = 1;
        }
        else
        {
          send_cmd(3, TRANSMITTER);
          link_info.sequenceNumber = 0;
        }
        return length;
      }
    }
  }
  return -1;
}

//need to send DISC message and get that message back and then send UA
//also close fd of course
//in slides llclose just has fd as a parameter but we probably should specify if whoever called this function
//since transmitter has to send DISC to receiver, however receiver doesn't have to do that
//need to do alarm + timeout each time
int llclose(int fd, int sender)
{
  int check = -1, conta = 0;
  int cmd_received = FALSE;
  char *cmd;
  if (sender != TRANSMITTER || sender != RECEIVER)
  { //sender will be TRANSMITTER or RECEIVER
    printf("ERROR");
  }

  if (sender == TRANSMITTER)
  {
    while (!cmd_received)
    {
      if (send_cmd(1, TRANSMITTER) < 0)
      {
        printf("ERROR\n");
      }

      if (read_cmd(app_info.fileDescriptor, cmd) < 0)
      {
        printf("Try again\n");
      }
      if (cmd == C_DISC)
      {
        deactivate_alarm();
        cmd_received = TRUE;
      }

      if (send_cmd(2, TRANSMITTER) < 0)
      {
        printf("Couldn't send UA, turning connection off\n");
      }
    }
  }
  else
  {
    while (!cmd_received)
    {
      if (read_cmd(app_info.fileDescriptor, cmd) < 0)
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

      if (send_cmd(1, TRANSMITTER) < 0)
      {
        printf("ERROR\n");
        continue;
      }

      if (read_cmd(app_info.fileDescriptor, cmd) < 0)
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
  sleep(1);
  /*if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
  {
    perror("tcsetattr");
    return -1;
  }*/
  close(fd);
  return 0;
}

int create_info_trama(char *buffer, char *trama, int length)
{
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
  printf("I'm in create_info_trama\n");
  char BCC2 = buffer[0];
  for (int i = 1; i < (length - 1); i++)
  {
    BCC2 ^= buffer[i];
  }
  printf("BCC2 %02x\n",BCC2);

  //We need to stuff the data + BCC2
  char *data_stuffed, bcc2_stuffed;
  int data_length = stuffing(buffer, length, &data_stuffed);
  printf("I returned from stuffing\n");
  int bcc2_length = stuffing(&BCC2, 1, &bcc2_stuffed); //I mean BCC2 has length==1 sooooo I don't really know why it's necessary to stuff them tbh but I know it is according to the slides

  //Assemble info trama
  int new_length = data_length + bcc2_length + 5; //5 because F A C BCC1 F
  trama[0] = FLAG;
  trama[1] = A_E;
  if (link_info.sequenceNumber == 0)
  {
    trama[2] = C_I_ZERO;
    trama[3] = BCC_C_I_ZERO;
  }
  else
  {
    trama[2] = C_I_ONE;
    trama[3] = BCC_C_I_ONE;
  }

  memcpy(&trama[4], data_stuffed, data_length);
  int index_bcc2 = data_length + 4;
  memcpy(&trama[index_bcc2], bcc2_stuffed, bcc2_length);
  trama[new_length - 1] = FLAG; //second flag is at the end of the frame

  //Need to keep track of pointers to free and think where I can free them
  return new_length;
}
