#ifndef VALIDATION_H_
#define VALIDATION_H_

#include "general.h"

int validateSET(unsigned char set, int* stateSet);
int validateUA(unsigned char data,int* stateUa);
int stateMachinellwrite(unsigned char c, int* state, int* ret);
int stateMachineReceiver(unsigned char* array);

#endif /* VALIDATION_H_ */
