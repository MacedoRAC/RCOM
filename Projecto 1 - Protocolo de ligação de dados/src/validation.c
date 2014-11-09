#include "validation.h"

int validateSET(unsigned char set, int* stateSet){

	switch(*stateSet){
	case 0:
		if(set == FLAG){
			*stateSet=1;
		}
		return 0;
	case 1:
		if(set == FLAG){
			*stateSet=1;
		}else if(set == A){
			*stateSet=2;
		}else{
			*stateSet=0;
		}
		return 0;
	case 2:
		if(set == FLAG){
			*stateSet=1;
		}else if(set == C_SET){
			*stateSet=3;
		}else{
			*stateSet=0;
		}
		return 0;
	case 3:
		if(set == FLAG){
			*stateSet=1;
		}else if(set == (A ^ C_SET)){
			*stateSet=4;
		}else{
			*stateSet=0;
		}
		return 0;
	case 4:
		if(set == FLAG){
			*stateSet=5;
			return 1;
		}

		else{
			*stateSet=0;
		}
		return 0;

	}
}

int validateUA(unsigned char data,int* stateUa){
	switch(*stateUa){
	case 0:
		if(data==FLAG){
			*stateUa=1;
		}
		return 0;
	case 1:
		if(data == FLAG){
			*stateUa=1;
		}else if(data == A){
			*stateUa=2;
		}else{
			*stateUa=0;
		}
		return 0;

	case 2:
		if(data == FLAG){
			*stateUa=1;
		}else if(data == C_UA){
			*stateUa=3;
		}else{
			*stateUa=0;
		}
		return 0;

	case 3:
		if(data == FLAG){
			*stateUa=1;
		}else if(data == (A ^ C_UA)){
			*stateUa=4;
		}	else{
			*stateUa=0;
		}
		return 0;

	case 4:
		if(data == FLAG){
			*stateUa=5;
			return 1;
		}else{
			*stateUa=0;
			return 0;
		}
	case 5:
		return -1;
	}
}

int stateMachinellwrite(unsigned char c, int* state, int* ret)
{
	switch(*state)
	{
	case 0:
		if (c == FLAG) *state=1;
		else *state=0;
		break;
	case 1:
		if (c == A) *state=2;
		else if (c == FLAG) *state=1;
		else *state=0;
		break;
	case 2:
		if (c == C_UA) *state=3;
		else if (c == C_RR0) *state=4;
		else if (c == C_REJ0) *state=5;
		else if (c == C_RR1) *state=6;
		else if (c == C_REJ1) *state=7;
		else if (c == C_DISC) *state=8;
		else if (c == FLAG) *state=1;
		else *state=0;
		break;
	case 3:
		if (c == A^C_UA) {*state=9; *ret=1;}
		else if (c == FLAG) *state=1;
		else *state=0;
		break;
	case 4:
		if (c == A^C_RR0) {*state=9; *ret=2;}
		else if (c == FLAG) *state=1;
		else *state=0;
		break;
	case 5:
		if (c == A^C_REJ0) {*state=9; *ret=3;}
		else if (c == FLAG) *state=1;
		else *state=0;
		break;
	case 6:
		if (c == A^C_RR1) {*state=9; *ret=4;}
		else if (c == FLAG) *state=1;
		else *state=0;
		break;
	case 7:
		if (c == A^C_REJ1) {*state=9; *ret=5;}
		else if (c == FLAG) *state=1;
		else *state=0;
		break;
	case 8:
		if(c == A^C_DISC) {*state=9; *ret=6;}
		else if (c == FLAG) *state=1;
		else *state=0;
		break;
	case 9:
		if (c == FLAG) return 1; // acabou
		else {*state = 0; *ret=0;}
		break;
	}
	return 0;//continua
}

int stateMachinellread(unsigned char c, int bcc2, int* state, int* ret)
{
	switch(*state)
	{
	case 0:
		if (c == FLAG) *state=1;
		else *state=0;
		break;
	case 1:
		if (c == A) *state=2;
		else if (c == FLAG) *state=1;
		else *state=0;
		break;
	case 2:
		if (c == C0) *state=3;
		else if (c == C1) *state=4;
		else if (c == FLAG) *state=1;
		else *state=0;
		break;
	case 3:
		if (c == A^C0) {*state=5; *ret=1;}
		else if (c == FLAG) *state=1;
		else *state=0;
		break;
	case 4:
		if (c == A^C1) {*state=5; *ret=2;}
		else if (c == FLAG) *state=1;
		else *state=0;
		break;
	case 5:
		if (1== bcc2) {*state = 6;}
		else if (c==FLAG) {*state=1; *ret=3;}
		else *state = 5;
		break;
	case 6:
		if (c == FLAG) return 1; // acabou
		else if(1 == bcc2) *state = 6;
		else {*state = 5;}
		break;
	}
	return 0;//continua
}
