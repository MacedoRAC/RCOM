#ifndef LINKLAYER_H_
#define LINKLAYER_H_

#include "validation.h"

int status;
struct termios oldtio;

void config(int fd);
int stuffing(unsigned char* data, unsigned char* stuffed, int n);
int destuffing(unsigned char recv, unsigned char* lastChar, unsigned char* destuffedChar);
int llopen(int porta, int mode);
int llwrite(int fd, unsigned char* buffer, int length);
int llread(int fd, unsigned char* buffer);
int llclose(int fd);

#endif /* LINKLAYER_H_ */
