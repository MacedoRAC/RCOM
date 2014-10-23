/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define F 0x7e
#define A 0x01
#define AR 0x03
#define C 0x03

unsigned char UA[5];

volatile int STOP=FALSE;

// MODIFIED STATE_MACHINE
int stateMachine(char array[]){
	
    int state, i;
    i=0; state=0;
       	
    unsigned char content;
    
    for (i; i < sizeof(array)/sizeof(unsigned char); i++){
		content=array[i]; 
		switch(state){
			case 0:
               if (content == F)
               state=1;
               else state=0;
               break;

               case 1:
               if (content == AR) state=2;
               else if (content == F) state=1;
               else state=0;
               break;

               case 2:
               if (content == C) state=3;
               else if (content == F) state=1;
               else state=0;
               break;

               case 3:
               if (content == AR^C) state=4;
               else 
					return 2; // erro de Bcc2 -> enviar REJ
               break;

               case 4:
               if (content == F) return 1;
               else state = 0;
               break;
               }
       }        
       return 0;
}


int llread(int fd, char* buffer){
	// read I -> o que nos e enviado
	
	
	int counter=0;
	int res=0;
	
    int stateCheck =0;
    while(stateCheck != 1){ //verificar condiçao de paragem
      res = read(fd, buffer, 255);
      prevState=stateCheck;//save previous return of state machine
      
      stateCheck = stateMachine(buffer);
      
      if((stateCheck==2 && prevCheck++!=2) 
    }
	
	//write -> RR / REJ
	return counter;
}


int main(int argc, char** argv)
{
  int fd,c, res, numChar;
    struct termios oldtio,newtio;
    char buf[255];
    char buf2[255];
    char string[255];
    int i, sum = 0, speed = 0;
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS4", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS4\n");
      exit(1);
    }

    UA[0]=F;
    UA[1]=A;
    UA[2]=C;
    UA[3]=UA[1]^UA[2];
    UA[4]=F;


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

    newtio.c_cc[VTIME]    = 3;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
   
  
   /* int stateCheck =0;
    while(stateCheck != 1){
      res = read(fd, buf, 255);
      stateCheck = stateMachine(buf);
    }*/
    
    int counter;
    counter = llread(fd, buf);
    
    if(counter!=-1)
		printf(":%s:%d\n", buf, counter);
	else
		printf("reading error\n");
    	
  /* 
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar 
    o indicado no guião 
  */


    write(fd, UA, strlen(UA));
    sleep(5);

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }




    close(fd);
    return 0;
}



