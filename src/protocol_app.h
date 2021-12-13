#ifndef PROTOCOL_APP_H
#define PROTOCOL_APP_H

/**
    * Function that opens serial ports and connects transmitter and receiver.
     * @return: 0 or -1 in case of error.
*/
int llopen();

/**
    * Function that writes information from the buffer into the serial port, sending it to the receiver.
     * @param buffer: information to be written.
     * @param length: size of buffer.
     * @return: number of characters written in sucess or -1 in case of error.
*/
int llwrite(u_int8_t*buffer,int length);

/**
    * Function that reads the information in the serial port.
     * @param buffer: array to store the information.
     * @return: number of characters read and stored in buffer or -1 in case of error.
*/
int llread(u_int8_t*buffer);

/**
    * Function that closes the connection between transmitter and receiver and the serial ports.
     * @return: 0 or -1 in case of error.
*/
int llclose();

/**
    * Function that creates the information trama.
     * @param buffer: array that contains the information to be placed in the information part of the trama.
     * @param trama: array to store the trama contents.
     * @param length: size of buffer.
     * @param s_writer: current sequece number.
     * @return: size of trama array.
*/
int create_info_trama(u_int8_t*buffer,u_int8_t*trama,int length,int s_writer);

/**
    * Function that creates BCC2 by doing the XOR operation with all the components of the info_trama array (that has data).
     * @param info_trama: array that contains data.
     * @param BCC2: array to store the value of BCC2.
     * @param length: size of info_trama array
*/
void check_BCC2(u_int8_t * info_trama, u_int8_t* BCC2, int length)

#endif
