#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/app.h"
#include "../include/constants.h"

int flag = 1, conta = 0;

void atende() // atende alarme
{
   conta++;

   printf("alarme # %d\n", conta);

   if (conta > link_info.numTransmissions)
   {
      printf("Number of transmission tries exceed");
      exit(-1);
   }
}

void install_alarm()
{
   if (signal(SIGALRM, atende) == SIG_ERR)
   {
      printf("Not possible to install signal, SIG_ERR.");
   }
   siginterrupt(SIGALRM, TRUE);
}

void deactivate_alarm()
{
   conta = 0;
   alarm(0);
}