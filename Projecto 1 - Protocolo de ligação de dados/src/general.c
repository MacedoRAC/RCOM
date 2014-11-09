#include "general.h"

int flag = 1;
int conta = 0;
unsigned int sequenceNumber = 0;
volatile int STOP=FALSE;

void atende() // handler atende alarme
{
	printf("alarme #%d\n", conta);
	flag=1;
	conta++;
}
