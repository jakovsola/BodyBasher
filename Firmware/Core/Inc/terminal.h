/*
 * terminal.h
 *
 *  Created on: Mar 31, 2024
 *      Author: jakov
 */

#ifndef INC_TERMINAL_H_
#define INC_TERMINAL_H_

#define TERMINAL_ERROR 1
#define TERMINAL_OK 0

#define MESSAGE_BUFFER_LENGTH 32
#define MESSAGE_HISTORY_DEPTH 5

#include "terminal.h"
#include "usbd_cdc_if.h"
//#include "main.h"

//anlysis functions
void initMessageBuffer();
void checkTerminal();    //should somehow be inline
void analyseMessage();
uint8_t findParameter(char * option, uint16_t * value, uint8_t findValue);


uint8_t TermHelp();
uint8_t printPrompt();
uint8_t TermArm();
uint8_t TermDisarm();
uint8_t TermStatus();
uint8_t TermError();
uint8_t TermBB();

uint8_t initTerminal();

uint8_t printInvalid();

void clearOutputGenerate(uint8_t * clearOutput, uint8_t * output);

typedef enum
{
    ARM = 0U,
    DISARM,
    ERROR_COM,
    HELP,
    STATUS,
    UNKNOWN = 0XFF
} TERMINAL_COMMAND;


#endif /* INC_TERMINAL_H_ */
