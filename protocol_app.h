#ifndef PROTOCOL_APP_H
#define PROTOCOL_APP_H

int llopen(char* porta,int sender);
int llwrite(int fd, u_int8_t*buffer,int length);
int llread(int fd,u_int8_t*buffer);
int llclose(int fd, int sender);
int create_info_trama(u_int8_t*buffer,u_int8_t*trama,int length,int s_writer);

#endif