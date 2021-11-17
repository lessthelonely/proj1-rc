#ifndef STUFFING_H
#define STUFFING_H

//We should use char** because it's a pointer to char*

int stuffing(char *buffer, int length, char **frame);
int destuffing(char *buffer, int length, char **frame);

#endif