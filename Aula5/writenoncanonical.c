/*Non-Canonical Input Processing*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7e
#define A 0x03
#define AR 0x01
#define C 0x03
int flag = 1, conta = 0;
unsigned int sequenceNumber = 0;
void atende() // handler atende alarme
{
printf("alarme # %d\n", conta);
flag=1;
conta++;
}

int stateMachine(unsigned char* array)
{
int state=0;
int i=0;
unsigned char content;
for (i; i < sizeof(array)/sizeof(unsigned char); i++)
{
content=array[i];
switch(state)
{
case 0:
if (content == FLAG) state=1;
else state=0;
break;
case 1:
if (content == AR) state=2;
else if (content == FLAG) state=1;
else state=0;
break;
case 2:
if (content == C) state=3;
else if (content == FLAG) state=1;
else state=0;
break;
case 3:
if (content == AR^C) state=4;
else if (content == FLAG) state=1;
else state=0;
break;
case 4:
if (content == FLAG) return 1;
else state = 0;
break;
}
}	
return 0;
}
volatile int STOP=FALSE;

int llopen(int porta, int transmitter)
{
	int fd,c, res, numChar;
	struct termios oldtio,newtio;
	(void) signal(SIGALRM, atende); // instala rotina que atende interrupcao
	unsigned char SET[5];
	unsigned char UA[5];
	unsigned char buf[255];
	SET[0] = FLAG;
	SET[1] = A;
	SET[2] = C;
	SET[3] = SET[1]^SET[2];
	SET[4] = FLAG;
	UA[0] = FLAG;
	UA[1] = AR;
	UA[2] = C;
	UA[3] = UA[1]^UA[2];
	UA[4] = FLAG;
	char port[10] = {'/', 'd', 'e','v','/','t','t','y','S',(porta+'0')};
	fd = open(port, O_RDWR | O_NOCTTY );
	if (fd <0) {perror(argv[1]); exit(-1); }
	if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
	perror("tcgetattr");
	exit(-1);
	}
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;
	newtio.c_cc[VTIME] = 3; /* inter-character timer unused */
	newtio.c_cc[VMIN] = 0; /* blocking read until 5 chars received */
	/*
	VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
	leitura do(s) próximo(s) caracter(es)
	*/
	tcflush(fd, TCIOFLUSH);
	if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
	perror("tcsetattr");
	exit(-1);
	}
	printf("New termios structure set\n");

	if(transmitter)
	{
		while(conta < 4 && !STOP)
		{
			if(flag)
			{
				res = write(fd,SET,5);
				printf("%d bytes written\n", res);
				alarm(3); // activa alarme de 3s
				flag=0;
			}
			while(flag==0 && !STOP)
			{
				res = read(fd, buf, 255);
				if(stateMachine(buf) == 1)
					STOP = TRUE;
			}
		}
		if(conta == 4)
			return -1;
	}
	else
 	{
		while(!STOP)
		{
		      res = read(fd, buf, 255);
		      if(stateMachine(buf) == 1)
				STOP = TRUE;
		}
		write(fd, UA, strlen(UA));
	}
	return fd;
}

int llwrite(int fd, char* buffer, int length)
{
	flag = 1;
	conta = 0;
	int res, i;
	(void) signal(SIGALRM, atende); // instala rotina que atende interrupcao
	char trama[255];
	char answer[255];
	char BBC2 = 0;
	trama[0] = FLAG;
	trama[1] = A;
	trama[2] = (sequenceNumber << 6);
	trama[3] = trama[1]^trama[2];
	for(i = 0; i < sizeof(buffer)/sizeof(char*); i++)
	{
		trama[i+4] = buffer[i];
		BCC2 = BCC2 ^ buffer[i];
	}
	trama[sizeof(buffer)/sizeof(char*)] = BCC2;
	trama[sizeof(buffer)/sizeof(char*) + 1] = FLAG;
	while(conta < 4 && !STOP)
	{
		if(flag)
		{
			res = write(fd, trama, length);
			alarm(3); // activa alarme de 3s
			flag=0;
		}
		while(flag==0 && !STOP)
		{
			read(fd, answer, 255);
			if(stateMachine(answer) == 1)
			{
				STOP = TRUE;
			}
		}
	}
	return res;
}


int main(int argc, char** argv)
{
(void) signal(SIGALRM, atende); // instala rotina que atende interrupcao
unsigned char SET[5];
unsigned char UA[255];
unsigned char ANSWER[5];
SET[0] = FLAG;
SET[1] = A;
SET[2] = C;
SET[3] = SET[1]^SET[2];
SET[4] = FLAG;
ANSWER[0] = FLAG;
ANSWER[1] = AR;
ANSWER[2] = C;
ANSWER[3] = ANSWER[1]^ANSWER[2];
ANSWER[4] = FLAG;
int fd,c, res, numChar;
struct termios oldtio,newtio;
int i, sum = 0, speed = 0;
if ( (argc < 2) ||
((strcmp("/dev/ttyS0", argv[1])!=0) &&
(strcmp("/dev/ttyS4", argv[1])!=0) )) {
printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS4\n");
exit(1);
}
/*
Open serial port device for reading and writing and not as controlling tty
because we don't want to get killed if linenoise sends CTRL-C.
*/
fd = open(argv[1], O_RDWR | O_NOCTTY );
if (fd <0) {perror(argv[1]); exit(-1); }
if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
perror("tcgetattr");
exit(-1);
}
bzero(&newtio, sizeof(newtio));
newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
newtio.c_iflag = IGNPAR;
newtio.c_oflag = 0;
/* set input mode (non-canonical, no echo,...) */
newtio.c_lflag = 0;
newtio.c_cc[VTIME] = 3; /* inter-character timer unused */
newtio.c_cc[VMIN] = 0; /* blocking read until 5 chars received */
/*
VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
leitura do(s) próximo(s) caracter(es)
*/
tcflush(fd, TCIOFLUSH);
if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
perror("tcsetattr");
exit(-1);
}
printf("New termios structure set\n");
while(conta < 4 && !STOP)
{
	if(flag)
	{
		res = write(fd,SET,5);
		printf("%d bytes written\n", res);
		alarm(3); // activa alarme de 3s
		flag=0;
	}
	while(flag==0 && !STOP)
	{
		read(fd, UA, 255);
		if(stateMachine(UA) == 1)
		{
			STOP = TRUE;
			printf(":%s:%d\n", UA, res);
		}
	}
}

sleep (1);
printf("Vou terminar.\n");
if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
perror("tcsetattr");
exit(-1);
}
sleep (1);
close(fd);
return 0;
}
