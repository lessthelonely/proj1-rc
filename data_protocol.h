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
int read_info_trama(char* info_trama);
int read_cmd();
int read_cmd(char*cmd);
void atende();

#endif
