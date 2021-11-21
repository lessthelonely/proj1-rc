#ifndef STUFFING_H
#define STUFFING_H

//We should use char** because it's a pointer to char*

int stuffing(u_int8_t* buffer, int length);
int destuffing(u_int8_t* buffer, int length);

#endif