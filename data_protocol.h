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
int read_cmd(u_int8_t* cmd);
#endif
