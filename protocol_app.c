
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
int llopen()
{ //Don't need any of the arguments in the slides because all the data is stored in data structures 
  if (app_info.status != TRANSMITTER && app_info.status != RECEIVER)
  { //sender will be TRANSMITTER or RECEIVER
    printf("ERROR");
    return -1;
  }

  u_int8_t cmd;
  int res = -1;

  int fd = open(link_info.port, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(link_info.port);
        return -1;
    }

  app_info.fileDescriptor = fd;

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

  if (app_info.status == TRANSMITTER)
  {
    while (res != 0)
    {
      alarm(link_info.timeout);
      if (send_cmd(0, TRANSMITTER) < 0)
      { //Send SET
        printf("ERROR\n");
      }
      else
      {
        printf("Wrote SET\n");
      }

      if ((res = read_cmd(&cmd)) >= 0){
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
      read_cmd(&cmd);
      printf("Read SET\n");

      if ((res = send_cmd(2, TRANSMITTER)) < 0)
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

/*Orders protocol to send the Info trauma
char* buffer is char array we want to transmit so it will the be data part of the info trauma?
In order to make this function work->need to build the information trama
F A C BCC1 char*buffer? BCC2 F
Return value should -1 in case of error or number of caracters written
TRANSMITTER is the only one who calls this function
*/
int llwrite(u_int8_t *buffer, int length)
{
  static int seqNum = 0; //keeps track of sequence number
  u_int8_t *trama = (u_int8_t *)malloc(MAX_SIZE * sizeof(u_int8_t)); //Allocs space to write info trama
  if (length < 0)
  {
    printf("Value should be positive in order to actually transfer data\n");
    free(trama);
    return -1;
  }
  int write_length;
  u_int8_t cmd;
  //Need to create info trauma + send it
  while (TRUE)
  { 
    //We initilized trauma array with the biggest possible size (MAX_SIZE) 
     memset(trama, 0, strlen(trama)); //initialize trama array
     write_length = create_info_trama(buffer, trama, length,seqNum);
 
    alarm(link_info.timeout);
    if (write(app_info.fileDescriptor, trama, write_length) < 0)
    {
      printf("ERROR");
    }
    else
    {
      printf("Sent message with sequence number %d\n", seqNum);
    }

    if (read_cmd(&cmd) < 0)
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
      seqNum=SWITCH(seqNum); //time for a new sequence
      free(trama);
      return write_length;
    }

    if (cmd == C_REJ_ZERO || cmd == C_REJ_ONE)
    {
      deactivate_alarm();
      printf("Received REJ\n");//Need to retransmit message
    }
  }
}

//Creates BCC2 by doing the XOR operation with all the components of the trama(data) array
void check_BCC2(u_int8_t * info_trama, u_int8_t* BCC2, int length)
{
    for (int i = 0 ; i < length; i++){
        *BCC2 ^= info_trama[i];         
    } 
}

/* RECEIVER is the only one who calls this function
   Return length of array of caracters read or -1 in case of error
*/
int llread(int fd, u_int8_t *buffer)
{
  static int s_reader=0,curr_s=0; //keeps track of the sequence Number
  int length;
  u_int8_t cmd;

  while (TRUE)
  {
    //read info trama-->can't use read_cmd because of the data segment
    
    if ((length = read_frame_i(app_info.fileDescriptor,buffer,&cmd)) < 0)
    {
    }
    else
    {
      printf("Read successfully info message with sequence number %d\n", s_reader);
    }

    if(cmd==C_I_ZERO){
      curr_s=0;
    }
    else{
      curr_s=1;
    }


   printf("LLREAD LENGTH %d\n",length);

    //Need to destuff before storing
    int new_length=destuffing(buffer, length);


    //printf("PACKAGE[10] %02x\n",buffer[10]);


    //Check if BCC2 is correct, if not dump info
    //First we create BCC2 based on the info and then we check if it matches up with the last bit of info trama (that should be BCC2)
    /*char BCC2 = info_trama[0];
    printf("BCC2 %02x\n",BCC2);
    for (int i = 1; i < length; i++)
    {
      printf("i %d\n",i);
      BCC2 ^= *info_trama[i];
    }*/

    u_int8_t BCC2;
    BCC2 = 0x00;
    printf("NEW LENGTH AFTER DESTUFFING %d\n",new_length);
    check_BCC2(buffer,&BCC2,new_length-1);
    printf("CHECKING %02x\n",BCC2); 

   /* printf("BCC2 %02x\n",BCC2); 

    printf("We alive\n");*/

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
    if (BCC2 != buffer[new_length - 1])
    {
      printf("Wrong BCC2-->gonna send negative ACK\n");
      bcc2_is_not_okay = TRUE;
    }

   /* printf("%02x\n",cmd);
    printf("%d\n",s_reader);
*/
    //Info is duplicated
    if ((cmd == C_I_ZERO && s_reader == 1) || (cmd == C_I_ONE && s_reader == 0))
    {
      //printf("HERERWRWR\n");
      if (bcc2_is_not_okay) //BCC2 is wrong 
      {
        //Send RR
        if (curr_s == 0)
        {
          /*When analyzing the whole program I don't think we will ever use A_R because I don't think receiver sends
        a command without it being an answer to the transmitter but in those cases we should use A_E
        ASK THE TEACHER-->when do we use A_R (00000001 (0x01) em Comandos enviados pelo Receptor e Respostas 
        enviadas pelo Emissor)->what qualifies as this*/
          send_cmd(3, TRANSMITTER);
        }
        else
        {
        //  printf("I'm here\n");
          send_cmd(4, TRANSMITTER);
        }
      }
      else
      {
        //Info duplicated but BCC2 right
        //Send RR & dump info
        if (curr_s== 0)
        {
          send_cmd(3, TRANSMITTER);
        }
        else
        {
          send_cmd(4, TRANSMITTER);
        }
      }
    }
    else
    { //info is new
      if (bcc2_is_not_okay)
      {
        //Send REJ
        if (cmd==C_I_ZERO)
        {
          send_cmd(5, TRANSMITTER);
        }
        else
        {
          send_cmd(6, TRANSMITTER);
        }
      }
      else
      {
        //Send RR + store info
        //Should change sequence number?
        if (s_reader == 0)
        {
          send_cmd(3, TRANSMITTER);
          s_reader=SWITCH(s_reader);
         // printf("HEeew\n");
        }
        else
        {
         // printf("HIHRI$HR\n");
          send_cmd(4, TRANSMITTER);
          s_reader=SWITCH(s_reader);
        }
        //printf("LENGTH %d\n",length);
        return length;
      }
    }
  }
  return -1;
}

int llclose() //Don't need any of the arguments in the slides because all the data is stored in data structures 
{
  int check = -1, conta = 0;
  int cmd_received = FALSE;
  u_int8_t cmd;
  if (app_info.status!= TRANSMITTER && app_info.status != RECEIVER)
  { //sender will be TRANSMITTER or RECEIVER
    printf("ERROR\n");
  }

  if (app_info.status == TRANSMITTER)
  {
    while (!cmd_received)
    {
      alarm(link_info.timeout);
      if (send_cmd(1, TRANSMITTER) < 0) //Send DISC
      {
        printf("ERROR\n");
      }

      if (read_cmd(&cmd) < 0) //Read DISC from Receiver
      {
        printf("Try again\n");
      }
      if (cmd == C_DISC)
      {
        deactivate_alarm();
        cmd_received = TRUE;
      }

      if (send_cmd(2, TRANSMITTER) < 0) //Send UA
      {
        printf("ERROR\n");
      }
    }
  }
  else
  {
    while (!cmd_received)
    {
      if (read_cmd(&cmd) < 0) //Read DISC
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

      if (send_cmd(1, TRANSMITTER) < 0) //Send DISC back
      {
        printf("ERROR\n");
        continue;
      }

      if (read_cmd(&cmd) < 0) //Read UA
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
  if (tcsetattr(app_info.fileDescriptor, TCSANOW, &oldtio) == -1)
  {
    perror("tcsetattr");
    return -1;
  }
  close(app_info.fileDescriptor);
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