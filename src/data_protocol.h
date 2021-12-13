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

/**
    * Function that writes supervision and non numbered tramas (SET, DISC, UA, RR, REJ).
     * @param command: int from 0 to 6, to specificy which trama is to be sent (SET, DISC, UA, RR with sequence number one, 
     RR with sequence number zero, REJ with sequence number one or REJ with sequence number zero).
     * @param sender: indicates who wishes to send the trama, TRANSMITTER or RECEIVER.
     * @return: number of bytes written (return value of function write) or -1 in case of error.
*/
int send_cmd(int command, int sender);

/**
    * Function that reads the information trama and stores the information portion of it to an array.
     * @param info_trama: array where information portion of information trama is stored.
     * @param cmd: variable where C (campo de controlo) is stored.
     * @return: size of info_trama array or -1 in case of error.
*/
int read_info_trama(u_int8_t* info_trama,u_int8_t*cmd);

/**
    * Function that reads supervision and non numbered tramas (SET, DISC, UA, RR, REJ).
     * @param cmd: variable where C (campo de controlo) is stored.
     * @return: 0 or -1 in case of error.
*/
int read_cmd(u_int8_t* cmd);
#endif
