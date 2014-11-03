/*
 * uartecho.c
 *
 *  Created on: Oct 25, 2014
 *      Author: jerome
 */
#include "main.h"

typedef struct {
   char command[2];
   int length;
   char* data;
   int checksum;
} uartFrame;

typedef enum  {
	TILDA_SEARCH,
	COMMAND1, COMMAND2,
	LENGTH1, LENGTH2,
	DATA,
	CHECKSUM1, CHECKSUM2
} packetState;

typedef struct {
	char command[2];
	void (*executeCommandFunc)(int, char*);
} commandExecuteCommandMap;

UART_Handle uart;
UART_Params uartParams;

static void printFrame(uartFrame frame);
static void UART_write_integer(UART_Handle handle, int number);

void parseFrame(uartFrame frame);

void executeLF(int dataLength, char* args);
void executeLR(int dataLength, char* args);
void executeRF(int dataLength, char* args);
void executeRR(int dataLength, char* args);

#define NUM_CMDS (10)
commandExecuteCommandMap commandExecuteCommandMapTable[NUM_CMDS] = {
		{"lf", &executeLF},
		{"lr", &executeLR},
		{"rf", &executeRF},
		{"rr", &executeRR}
};

/*
 *  ======== echoFxn ========
 *  Task for this function is created statically. See the project's .cfg file.
 */
void echoFxn(UArg arg0, UArg arg1)
{
	char inputChar;
	char* prompt = "\r\n>> ";
    packetState currentState = TILDA_SEARCH;
    int frameDataCounter=0;
    uartFrame currentFrame;

    /* Create a UART with data processing off. */
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.readEcho = UART_ECHO_ON;
    uartParams.baudRate = 9600;
    uart = UART_open(Board_UART0, &uartParams);

    if (uart == NULL) {
        System_abort("Error opening the UART");
    }

    UART_write(uart, prompt, sizeof(prompt));

    /* Loop forever echoing */
    while (1) {
    	UART_read(uart, &inputChar, 1);
    	if(inputChar == '~') {
    		currentState = COMMAND1;
    		continue;
    	}
    	else if(inputChar == '\r') {
    		UART_write(uart, prompt, sizeof(prompt));
    		UART_write(uart, "hello\b", sizeof("hello\b"));

    		currentState = TILDA_SEARCH;
    		continue;
    	}
    	switch(currentState) {
    	case COMMAND1:
    		currentFrame.command[0] = inputChar;
    		currentState = COMMAND2;
    		break;

    	case COMMAND2:
    		currentFrame.command[1] = inputChar;
    		currentState = LENGTH1;
    		break;

    	case LENGTH1:
    		currentFrame.length = atoi(&inputChar) << 4;
    		currentState = LENGTH2;
    		break;

    	case LENGTH2:
    		currentFrame.length += atoi(&inputChar);
    		currentFrame.data = malloc(currentFrame.length * sizeof(char));
    		frameDataCounter=0;
    		currentState = DATA;
    		break;

    	case DATA:
    		currentFrame.data[frameDataCounter] = inputChar;
    		frameDataCounter++;
    		if(frameDataCounter == currentFrame.length) {
    			currentState = CHECKSUM1;
    		}

    		break;

    	case CHECKSUM1:
    		currentFrame.checksum = atoi(&inputChar) << 8;
    		currentState = 	CHECKSUM2;
    		break;

    	case CHECKSUM2:
    		currentFrame.checksum += atoi(&inputChar);
    		currentState = TILDA_SEARCH;
    		printFrame(currentFrame);
    		parseFrame(currentFrame);
    		break;
    	default:
    		break;
    	}

    }
}

static void printFrame(uartFrame frame) {

	UART_write(uart, "\r\nFrame Found!\r\nPrinting Frame...", sizeof("\r\nFrame Found!\r\nPrinting Frame..."));
	//Print Command
	UART_write(uart, "\r\nCommand = ", sizeof("\r\nCommand = "));
	UART_write(uart, frame.command, 2);
	//Print Length
	UART_write(uart, "\r\nLength = ", sizeof("\r\nLength = "));
	UART_write_integer(uart, frame.length);
	//Print Data
	UART_write(uart, "\r\nData = ", sizeof("\r\nData = "));
	UART_write(uart, frame.data, frame.length);
	//Print Checksum
	UART_write(uart, "\r\nChecksum = ", sizeof("\r\nChecksum = "));
	UART_write_integer(uart, frame.checksum);
	UART_write(uart, "\r\n", sizeof("\r\n"));
}

void parseFrame(uartFrame frame) {
	int i;
	if(!frame.checksum) {
		for(i = 0; i<NUM_CMDS; i++) {
			if (
					(frame.command[0] == commandExecuteCommandMapTable[i].command[0]) &&
					(frame.command[1] == commandExecuteCommandMapTable[i].command[1])
				 ) {
				commandExecuteCommandMapTable[i].executeCommandFunc(frame.length, frame.data);
			}
		}
	}
	else {
		UART_write(uart, "Checksum Failed!", sizeof("Checksum Failed!"));
	}
}



/* Conversion Functions */

// Used to print integers as ASCII characters
static void UART_write_integer(UART_Handle handle, int n) {
  if(n >= 10){
	  UART_write_integer(uart, n/10);
      n = n%10;
  }
  n = n+'0';
  UART_write(uart, &n, sizeof(char));
}




/* Function Table */
void executeLF(int dataLength, char* args) {
	UART_write(uart, "\r\nExecuting LF", sizeof("\r\nExecuting LF"));
	return;
}
void executeLR(int dataLength, char* args) {
	UART_write(uart, "\r\nExecuting LR", sizeof("\r\nExecuting LR"));
	return;
}
void executeRF(int dataLength, char* args) {
	UART_write(uart, "\r\nExecuting RF", sizeof("\r\nExecuting RF"));
	return;
}
void executeRR(int dataLength, char* args) {
	UART_write(uart, "\r\nExecuting RR", sizeof("\r\nExecuting RR"));
	return;
}
