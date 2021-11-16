#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include "app.h"

int flag=1, conta=1;

void install_alarm(){
   (void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao
}

void atende()                   // atende alarme
{
	printf("alarme # %d\n", conta);
	conta++;
   if(conta > link_info.numTransmissions){
      printf("Number of transmission tries exceed");
      exit(-1);
   }
}

void deactivate_alarm(){
   conta=0;
   alarm(0);
}