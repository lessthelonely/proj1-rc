#ifndef DATA_PROTOCOL_H
#define DATA_PROTOCOL_H

typedef enum
{
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    STOP
} messageState;

int send_cmd(int command, int sender);
int read_cmd();
int read_cmd(char*cmd);
void atende();

#endif
