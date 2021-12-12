#ifndef STUFFING_H
#define STUFFING_H

/**
     * Function that executes the stuffing process in a frame, adding new bytes to it depending on the bytes found in it. Called before trama is sent to receiver.
     * @param buffer: frame.
     * @param length: size of frame buffer.
     * @return: size of the new frame.
*/
int stuffing(u_int8_t* buffer, int length);

/**
    * Function that executes the destuffing process in a frame, removing specific bytes from it. Called by the receiver in order to store information.
     * @param buffer: frame.
     * @param length: size of frame buffer.
     * @return: size of the new frame.
*/
int destuffing(u_int8_t* buffer, int length);

#endif
