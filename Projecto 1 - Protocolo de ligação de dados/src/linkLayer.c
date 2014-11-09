#include "linkLayer.h"

void config(int fd)
{
	struct termios newtio;
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
	leitura do(s) proximo(s) caracter(es)
	 */
	tcflush(fd, TCIOFLUSH);
	if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}
	printf("New termios structure set\n");
}

int stuffing(unsigned char* data, unsigned char* stuffed, int n)
{
	int i, length=1;
	stuffed[0] = data[0];
	for(i=1; i<n-1; i++)
	{
		if(data[i]==FLAG)
		{
			stuffed[length]=ESC_BYTE;
			length++;
			stuffed[length]=0x5e;
			length++;
		}
		else if(data[i]==ESC_BYTE)
		{
			stuffed[length]=ESC_BYTE;
			length++;
			stuffed[length]=0x5d;
			length++;
		}
		else
		{
			stuffed[length]=data[i];
			length++;
		}
	}
	stuffed[length]=data[n-1];
	length++;
	return length;
}

int destuffing(unsigned char recv, unsigned char* lastChar, unsigned char* destuffedChar)
{
	if(recv == ESC_BYTE)
	{
		lastChar[0] = ESC_BYTE;
		return 0;
	}
	else if(lastChar[0] == ESC_BYTE)
	{
		if(recv == 0x5e)
		{
			lastChar[0] = 0x5e;
			destuffedChar[0] = FLAG;
		}
		else if(recv == 0x5d)
		{
			lastChar[0] = 0x5d;
			destuffedChar[0] = ESC_BYTE;
		}
		return 1;
	}
	else
	{
		destuffedChar[0] = recv;
		return 1;
	}
}

int llopen(int porta, int mode)
{
	int fd, c, res, numChar, state = 0, ret = 0;
	(void) signal(SIGALRM, atende); // instala rotina que atende interrupcao
	unsigned char SET[5];
	unsigned char UA[5];
	unsigned char buf[255];
	unsigned char recv;
	SET[0] = FLAG;
	SET[1] = A;
	SET[2] = C_SET;
	SET[3] = SET[1]^SET[2];
	SET[4] = FLAG;
	UA[0] = FLAG;
	UA[1] = A;
	UA[2] = C_UA;
	UA[3] = UA[1]^UA[2];
	UA[4] = FLAG;
	char* port;
	port = (char*) malloc(sizeof(char)*10);
	port[0] = '/';port[1] = 'd';port[2] = 'e';port[3] = 'v';port[4] = '/';
	port[5] = 't';port[6] = 't';port[7] = 'y';port[8] = 'S';port[9] = '0'+porta;
	fd = open(port, O_RDWR | O_NOCTTY);
	if (fd <0) {perror(port); exit(-1); }
	free(port);
	config(fd);

	if(mode == TRANSMITTER)
	{
		while(conta < 4 && ret != 1)
		{
			if(flag)
			{
				res = write(fd,SET,5);
				printf("%d bytes written\n", res);
				alarm(3); // activa alarme de 3s
				flag=0;
			}

			while(flag==0 && ret != 1)
			{
				res = read(fd, buf, 1);
				if(res == 1)
				{
					recv = buf[0];
					ret = validateUA(recv, &state);
				}
			}
		}
		if(conta == 4)
			return -1;
	}
	else
	{
		while(ret != 1)
		{
			res = read(fd, buf, 1);
			if(res == 1)
			{
				recv = buf[0];
				ret = validateSET(recv, &state);
			}
		}
		write(fd, UA, 5);
	}
	return fd;
}

int llwrite(int fd, unsigned char* buffer, int length)
{
	conta = 0;
	flag = 1;
	int i=0, stuffed_size = 0, state = 0, ret = 0, stateRet = 0, res=0, numCharsRead = 0;
	unsigned char bcc2 = 0x00;
	unsigned char trama[MAX_PACKET_SIZE+10]; // macro tamanho
	unsigned char stuffed[(MAX_PACKET_SIZE+10)*2];
	unsigned char answer[255];
	trama[0] = FLAG;
	trama[1] = A;
	if ( sequenceNumber == 0)
		trama[2] = C0;
	else
		trama[2] = C1;
	trama[3] = trama[1]^trama[2];
	printf("pacote\n");
	for(i;i<length;i++)
	{
		printf("%02x  ", buffer[i]);
		trama[i+4] = buffer[i];
		bcc2 = bcc2^trama[i+4];
	}
	printf("\n");
	trama[length+4] = bcc2;
	trama[length+5] = FLAG;
	//colmatar se erro no stuffing
	stuffed_size = stuffing(trama,stuffed,length+6);
	while(conta < 4)
	{
		if(flag)
		{
			res=write(fd,stuffed,stuffed_size);
			alarm(3);
			flag = 0;
		}
		while(flag==0)
		{
			numCharsRead = read(fd,answer,1);
			if(numCharsRead == 1)
			{
				stateRet = stateMachinellwrite(answer[0], &state, &ret);
			}
			if(stateRet == 1)
				switch(ret)
				{
				case 2:
					if(sequenceNumber == 1)
					{
						printf("recebeu RR0\n\n");
						sequenceNumber = 0;
						alarm(0);
						return res;
					}
					break;
				case 3:
					if(sequenceNumber == 0)
					{
						printf("recebeu REJ0\n\n");
						res=write(fd,stuffed,stuffed_size);
						flag = 0;
						state = 0;
						stateRet = 0;
						ret = 0;
						numCharsRead = 0;
						alarm(3);
					}
					break;
				case 4:
					if(sequenceNumber == 0)
					{
						printf("recebeu RR1\n\n");
						sequenceNumber = 1;
						alarm(0);
						return res;
					}
					break;
				case 5:
					if(sequenceNumber == 1)
					{
						printf("recebeu REJ1\n\n");
						res=write(fd,stuffed,stuffed_size);
						flag = 0;
						state = 0;
						stateRet = 0;
						ret = 0;
						numCharsRead = 0;
						alarm(3);
					}
					break;
				}
		}
	}
	if(conta == 4)
		return -1;
}

int llread(int fd, unsigned char* buffer)
{
	int length=0, c = 0, i = 0, stateMachine = 0, ind = 4, destuffedRet =0, state = 0, ret = 0, bcc2OK = 0;
	unsigned char bcc2test = 0x00;
	unsigned char lastChar[1];
	unsigned char destuffedChar[1];
	unsigned char readbuf[255];
	unsigned char bufdestuffed[MAX_PACKET_SIZE+10];
	lastChar[0] = 0x00;
	do
	{
		length = read(fd,readbuf,1);
		if (length==1)
		{
			destuffedRet = destuffing(readbuf[0], lastChar, destuffedChar);
			if(destuffedRet == 1)
			{
				if(state == 5 || state == 6)
				{
					buffer[i] = destuffedChar[0];
					i++;
					if(destuffedChar[0] == bcc2test)
						bcc2OK = 1;
					else
						bcc2OK = 0;
					stateMachine = stateMachinellread(readbuf[0], bcc2OK, &state, &ret);
					bcc2test = bcc2test^destuffedChar[0];
				}
				else
					stateMachine = stateMachinellread(readbuf[0], bcc2OK, &state, &ret);
				if(stateMachine == 1)
					break;
				if(ret == 3)
				{
					if(bufdestuffed[2] == C0)
					{
						unsigned char send[5];
						send[0] = FLAG;
						send[1] = A;
						send[2] = C_REJ0;
						send[3] = send[1]^send[2];
						send[4] = FLAG;

						write(fd, send, 5);
						state = 0; ret = 0; i = 1; bcc2test = 0x00; lastChar[0] = 0x00; bcc2OK = 0;
					}
					else
					{
						unsigned char send[5];
						send[0] = FLAG;
						send[1] = A;
						send[2] = C_REJ1;
						send[3] = send[1]^send[2];
						send[4] = FLAG;

						write(fd, send, 5);
						state = 0; ret = 0; i = 0; bcc2test = 0x00; lastChar[0] = 0x00; bcc2OK = 0;
					}
				}
			}
		}
	}while(stateMachine == 0);
	if(ret == 1)
	{
		{
			unsigned char send[5];
			send[0] = FLAG;
			send[1] = A;
			send[2] = C_RR1;
			send[3] = send[1]^send[2];
			send[4] = FLAG;

			write(fd, send, 5);
		}
	}
	else if(ret == 2)
	{
		{
			unsigned char send[5];
			send[0] = FLAG;
			send[1] = A;
			send[2] = C_RR0;
			send[3] = send[1]^send[2];
			send[4] = FLAG;

			write(fd, send, 5);
		}
	}
	return i-2;
}

int llclose(int fd){
	unsigned char UA[5];
	unsigned char DISC[5];
	UA[0] = FLAG;
	UA[1] = A;
	UA[2] = C_UA;
	UA[3] = UA[1]^UA[2];
	UA[4] = FLAG;
	DISC[0] = FLAG;
	DISC[1] = A;
	DISC[2] = C_DISC;
	DISC[3] = DISC[1]^DISC[2];
	DISC[4] = FLAG;
	STOP = FALSE;
	conta = 0;
	flag = 1;
	unsigned char buf[255];
	unsigned char recv;
	int state = 0, ret = 0, stateRet = 0;
	if(status == TRANSMITTER) {
		int written = 0;
		while(conta < 4 && written != 2)
		{
			if(flag && written == 0)
			{
				int res = write(fd,DISC,5);
				printf("%d bytes written DISC\n", res);
				alarm(3); // activa alarme de 3s
				flag=0;
			}
			else if(flag && written)
			{
				int res = write(fd,UA,5);
				printf("%d bytes written UA\n", res);
				sleep(3);
				written = 2;
			}
			while(flag==0 && !STOP)
			{
				int res = read(fd, buf, 1);
				if(res == 1)
				{
					recv = buf[0];
					stateRet = stateMachinellwrite(recv, &state, & ret);
				}
				if(stateRet == 1 && ret == 6)
				{
					STOP = TRUE;
					conta = 0;
					if(written == 0)
						written = 1;
					state = 0;
					ret = 0;
					stateRet = 0;
				}
			}
		}
		if(conta == 4)
		{	
			if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
				perror("tcsetattr");
				exit(-1);
			}
			printf("Old termios structure set\n");
			close(fd);
			return -1;
		}
	}
	else if(status == RECEIVER) {
		int i = 0;
		while(!STOP)
		{
			int res = read(fd, buf, 1);
			if(res == 1)
			{
				recv = buf[0];
				stateRet = stateMachinellwrite(recv, &state, & ret);
			}
			if(stateRet == 1 && ret == 6) // recebe DISC
			{
				STOP = TRUE;
				state = 0;
				ret = 0;
				stateRet = 0;
			}
		}
		int res = write(fd,DISC,5);
		printf("%d bytes written DISC\n", res);
		STOP = FALSE;
		while(!STOP)
		{
			int res = read(fd, buf, 1);
			if(res == 1)
			{
				recv = buf[0];
				stateRet = stateMachinellwrite(recv, &state, & ret);
			}
			if(stateRet == 1 && ret == 1) // recebe UA
			{
				STOP = TRUE;
				state = 0;
				ret = 0;
				stateRet = 0;
			}
		}
	}
	if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}
	printf("Old termios structure set\n");
	close(fd);
	return 0;
}
