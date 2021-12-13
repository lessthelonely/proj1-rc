#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "protocol_app.h"
#include "constants.h"


int flag = 1, conta = 0;

void atende() // atende alarme
{
   conta++;

   printf("alarme # %d\n", conta);

   if (conta > link_info.numTransmissions)
   {
      printf("Number of transmission tries exceed");
      close(link_info.fileDescriptor);
      exit(-1);
   }
}

void install_alarm()
{
   signal(SIGALRM, atende);
   siginterrupt(SIGALRM, TRUE);
}

void deactivate_alarm()
{
   conta = 0;
   alarm(0);
}
