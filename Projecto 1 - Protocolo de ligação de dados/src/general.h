#ifndef GENERAL_H_
#define GENERAL_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#define BAUDRATE B9600
#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define MAX_PACKET_SIZE 200
#define PACKET_DATA 0x01
#define PACKET_START 0x02
#define PACKET_END 0x03
#define ESC_BYTE 0x7d

#define FALSE 0
#define TRUE 1
#define RECEIVER 0
#define TRANSMITTER 1

#define FLAG 0x7e
#define A 0x03
#define AR 0x01
#define C_SET 0x03
#define C_UA 0x07
#define C_RR0 0x05
#define C_REJ0 0x01
#define C_RR1 0x85
#define C_REJ1 0x81
#define C0 0x00
#define C1 0x40
#define C_START 0x02
#define C_END 0x03
#define C_DISC 0x0b

int flag, conta;
unsigned int sequenceNumber;
volatile int STOP;

void atende();

#endif /* GENERAL_H_ */