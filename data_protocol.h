#ifndef DATA_PROTOCOL_H
#define DATA_PROTOCOL_H

typedef enum
{
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    D_RCV,
    STOP
} messageState;

int send_cmd(int command, int sender);
int read_info_trama(u_int8_t* info_trama,u_int8_t*cmd);
int read_cmd(int fd,u_int8_t* cmd);
int read_frame_supervision(int fd, u_int8_t *CMD);
int read_frame_i(int fd, u_int8_t *buffer, u_int8_t *CMD);
#endif
