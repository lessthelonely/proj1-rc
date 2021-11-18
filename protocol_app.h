#ifndef PROTOCOL_APP_H
#define PROTOCOL_APP_H

int llopen(char* porta,int sender);
int llwrite(int fd, char*buffer,int length);
int llread(int fd,char*buffer);
int llclose(int fd, int sender);
int create_info_trama(char*buffer,char*trama,int length,int s_writer);

#endif