/*
 * terminal.c
 *
 *  Created on: Mar 31, 2024
 *      Author: jakov
 */
#include "terminal.h"
#include "fsm.h"

extern EVENT event;
extern STATE state;
extern uint8_t errorCode;

extern ADC_HandleTypeDef hadc;
extern SPI_HandleTypeDef hspi1;
extern I2C_HandleTypeDef hi2c1;

extern uint16_t v_set;
extern uint16_t v_pol;
extern uint16_t v_tol;

uint8_t newLine = 0;
uint8_t overflow = 0;

uint8_t message[MESSAGE_HISTORY_DEPTH][MESSAGE_BUFFER_LENGTH];
uint8_t historyPosition = 0;
uint8_t *messagePointer[MESSAGE_HISTORY_DEPTH];
//uint8_t *cursor = message[0];

uint8_t helpMessage[] =
        "\r\nThis is the list of functions:\r\n"
                "   help:   you are here\r\n"
                "   arm:    charges the output capacitor according to settings\r\n"
                "       -v: voltage setpoint in volts\r\n"
                "       -p: voltage polarity; 1 positive, 0 negative\r\n"
                "       -t: percentage tolerance (default 2%, max 20%)\r\n"
                "       FORMAT: arm -v <voltage> -p <0/1> -t <value>\r\n"
                "   disarm: discharges the capacitor to a safe voltage\r\n"
                "   status: prints current device information\r\n"
                "   error:  error status and control\r\n"
                "       -p: print error code (default)\r\n"
                "       -c: clears the errors\r\n"
                "       -h: expanation of error codes\r\n"
                "   bb:     TPS55289 register write and read (writes before reading if both)\r\n"
                "       -rr: read register\r\n"
                "       -wr: write register\r\n"
                "       -val: value to write to register\r\n"
                "       FORMAT: bb -rr <register> -wr <register> -val <value>\r\n"
                "   reset: resets the STM32 MCU";

uint8_t logo[] =
        "    ____            __         ____             __\r\n"
        "   / __ )____  ____/ /_  __   / __ )____ ______/ /_  ___  _____\r\n"
        "  / __  / __ |/ __  / / / /  / __ / __  / ___/  _  |/ _ |/ ___/\r\n"
        " / /_/ / /_/ / /_/ / /_/ /  / /_/ / /_/ (__  / / / /  __/ /\r\n"
        "/_____/|____/|__ _/ __  /  /_____/|__ _/____/_/ /_/|___/_/\r\n"
        "                  /____/\r\n"
        "FW Version 1.1";

uint8_t overflowMessage[] = "\r\nOverflow"; //here because the function is called all the time

/**
  * @brief  Checks if enter key was pressed. If yes, calls analysis function and sets prepares
  *         the buffers for new inputs.
  */

void checkTerminal()
{
    if (newLine)
    {

        analyseMessage();
        newLine = 0;

        memcpy(message[0], message[historyPosition], MESSAGE_BUFFER_LENGTH);
        messagePointer[0] = message[0]
                + (messagePointer[historyPosition] - message[historyPosition]);

        for (uint8_t i = MESSAGE_HISTORY_DEPTH - 1; i > 0; i--)
        {
            memcpy(message[i], message[i - 1], MESSAGE_BUFFER_LENGTH);
            messagePointer[i] = message[i]
                    + (messagePointer[i - 1] - message[i - 1]);
        }
//        memcpy(message[2], message[1], MESSAGE_BUFFER_LENGTH);
//        memcpy(message[1], message[historyPosition], MESSAGE_BUFFER_LENGTH);

//        messagePointer[2] = message[2] + (messagePointer[1] - message[1]);
//        messagePointer[1] = message[1] + (messagePointer[historyPosition] - message[historyPosition]);

        historyPosition = 0;
        messagePointer[historyPosition] = message[historyPosition];
        memset(message[historyPosition], ' ', MESSAGE_BUFFER_LENGTH); //clear buffer on every promptm (needed for findParameter)
    } else if (overflow)
    {
        CDC_Transmit_FS(overflowMessage, sizeof(overflowMessage));
        printPrompt();
        overflow = 0;
    }
    return;
}

/**
  * @brief Decodes which command was called and decodes their perameters if needed.
  *        Calls relevant functions depending on the user input.
  *
  */
void analyseMessage()
{
    uint8_t helpInput[] = "help";
    uint8_t armInput[] = "arm";
    uint8_t disarmInput[] = "disarm";
    uint8_t statusInput[] = "status";
    uint8_t errorInput[] = "error";
    uint8_t resetInput[] = "reset";
    uint8_t bbInput[] = "bb";

    uint8_t unknown[] = "\r\nUnknown message, see help\r\nBB:>";

    if (messagePointer[historyPosition] == message[historyPosition])
    {
        printPrompt();
        return;
    }

//    if (memcmp(helpInput, message, messagePointer) == 0)

    if (memcmp(helpInput, message[historyPosition], sizeof(helpInput) - 1) == 0)
    {
        TermHelp();
    } else if (memcmp(armInput, message[historyPosition], sizeof(armInput) - 1)
            == 0)
    {
        TermArm();
    } else if (memcmp(disarmInput, message[historyPosition],
            sizeof(disarmInput) - 1) == 0)
    {
        TermDisarm();
    } else if (memcmp(statusInput, message[historyPosition],
            sizeof(statusInput) - 1) == 0)
    {
        TermStatus();
    } else if (memcmp(errorInput, message[historyPosition],
            sizeof(errorInput) - 1) == 0)
    {
        TermError();
    } else if (memcmp(bbInput, message[historyPosition], sizeof(bbInput) - 1)
            == 0)
    {
        TermBB();
    } else if (memcmp(resetInput, message[historyPosition],
            sizeof(resetInput) - 1) == 0)
    {
        HAL_NVIC_SystemReset();
    } else
    {
        CDC_Transmit_FS(unknown, sizeof(unknown));
    }
    return;
}

/**
  * @brief
  *
  * @param
  * @retval
  */
uint8_t TermHelp()
{
    CDC_Transmit_FS(helpMessage, sizeof(helpMessage));
    return printPrompt();
//    return CDC_Transmit_FS(helpMessage2, sizeof(helpMessage2));
}

/**
  * @brief
  *
  */
uint8_t TermArm()
{
    uint8_t armMessage[32] = "";

    uint16_t voltage = 0;
    uint8_t voltageFound = 0;

    uint16_t polarity = 1;
    uint8_t polarityFound = 0;

    uint16_t tolerance = 2;
//    uint8_t toleranceFound = 0; //don't care if found since there is a default

    voltageFound = findParameter("-v", &voltage, TRUE);
    polarityFound = findParameter("-p", &polarity, TRUE);
    findParameter("-t", &tolerance, TRUE);

    if (voltageFound == FALSE || polarityFound == FALSE)
    {
        sprintf((char*) armMessage, "\r\nVoltage/polarity bad input.");
    } else if (voltage > 500)
    {
        sprintf((char*) armMessage, "\r\nVoltage > 500V");
    } else if (polarity != POSITIVE && polarity != NEGATIVE)
    {
        sprintf((char*) armMessage, "\r\nPolarity must be 1(+) or 0(-)");
    } else if (tolerance > 20)
    {
        sprintf((char*) armMessage, "\r\nTolerance must be <20%%");
    }

    else
    {
        if (polarity == POSITIVE)
        {
            sprintf((char*) armMessage, "\r\nArming to +%uV +-%u%%", voltage,
                    tolerance);
        } else if (polarity == NEGATIVE)
        {
            sprintf((char*) armMessage, "\r\nArming to -%uV +-%u%%", voltage,
                    tolerance);
        }
        v_set = voltage;
        v_pol = polarity;
        v_tol = tolerance;
        event = EV_ARM;
    }
    CDC_Transmit_FS(armMessage, sizeof(armMessage));
    return printPrompt();

}

/**
  * @brief
  *
  */

uint8_t TermDisarm()
{
    uint8_t disarmMessage[] = "\r\nDisarming...";
    event = EV_DISARM;
    CDC_Transmit_FS(disarmMessage, sizeof(disarmMessage));
    return printPrompt();
}

/**
  * @brief
  *
  */

uint8_t TermStatus()
{
    //control voltage measurement
    uint32_t v_ctrl_raw;
    float v_ctrl;
    uint8_t controlVoltageMessage[32] = "";

    HAL_ADC_Start(&hadc);
    HAL_ADC_PollForConversion(&hadc, 1);
    HAL_Delay(1);

    v_ctrl_raw = HAL_ADC_GetValue(&hadc);
    v_ctrl = v_ctrl_raw * 2.906; // (3300mV/4096) * (18.4kOhm/5.1kOhm) = 2.906mV
    sprintf((char*) controlVoltageMessage, "\r\nControl voltage ADC: %.2fmV",
            v_ctrl);
    CDC_Transmit_FS(controlVoltageMessage, sizeof(controlVoltageMessage));

    //output voltage measurement
    uint16_t v_out_raw;
    float v_out;
    uint8_t outputVoltageMessage[32] = "";

//	HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Receive(&hspi1, &v_out_raw, 1, 100);
//	HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_SET);

    v_out_raw = (v_out_raw & 0x1ff8) >> 2;
    v_out = v_out_raw * 0.4963; // (5V/1024) * (101MOhm/1MOhm) = 0.493V
    sprintf((char*) outputVoltageMessage, "\r\nControl voltage ADC: %.2fV",
            v_out);
    CDC_Transmit_FS(outputVoltageMessage, sizeof(outputVoltageMessage));

    //TPS55289 status register
    uint8_t TPS_status = 0;
    uint8_t TPSStatusMessage[32] = "";
    HAL_I2C_Mem_Read(&hi2c1, BB_ADDR, BB_STATUS, 1, &TPS_status,
            I2C_MEMADD_SIZE_8BIT, 100);

    uint8_t scp;
    scp = (uint8_t) ((TPS_status & 0x80) != 0);
    uint8_t ocp;
    ocp = (uint8_t) ((TPS_status & 0x40) != 0);
    uint8_t ovp;
    ovp = (uint8_t) ((TPS_status & 0x20) != 0);
    sprintf((char*) TPSStatusMessage, "\r\nTPS: SCP: %u; OCP: %u; OVP: %u", scp,
            ocp, ovp);
    CDC_Transmit_FS(TPSStatusMessage, sizeof(TPSStatusMessage));

    //state
    uint8_t stateMessage[32] = "";
    switch (state) {
        case ST_IDLE:
            sprintf((char*) stateMessage, "\r\nCurrent state: IDLE");
            break;
        case ST_ARMING:
            sprintf((char*) stateMessage, "\r\nCurrent state: ARMING");
            break;
        case ST_ARMED:
            sprintf((char*) stateMessage, "\r\nCurrent state: ARMED");
            break;
            //    ST_INJECTING,
        case ST_ERROR:
            sprintf((char*) stateMessage, "\r\nCurrent state: ERROR");
            break;
    }
    CDC_Transmit_FS(stateMessage, sizeof(stateMessage));
    return printPrompt();
}

/**
  * @brief
  *
  */

uint8_t TermError()
{
    uint8_t errorMessage[32] = "";
    uint8_t errorClearMessage[] = "\r\nErrors cleared.";
    uint8_t errorHelpMessage[] =
            "\r\nThe error code is an XOR of all the error events.\r\n"
                    "   CODE | MEANING\r\n"
                    "   0x01 | Arming timeout\r\n"
                    "   0x02 | Arming over-voltage (v_out > 505V\r\n"
                    "   0x04 | LMG2610 over-temperature fault"
                    "	0x08 | Watchdog timeout (reported before reset)";

//    uint8_t printFound = 0;
    uint8_t clearFound = 0;
    uint8_t helpFound = 0;

//    printFound = findParameter("-p", NULL, FALSE); //no need to look for it, always print
    clearFound = findParameter("-c", NULL, FALSE);
    helpFound = findParameter("-h", NULL, FALSE);

    if (errorCode == 0x00) sprintf((char*) errorMessage, "\r\nNo errors!");
    else
        sprintf((char*) errorMessage, "\r\nError code: 0x%02X", errorCode);
    CDC_Transmit_FS(errorMessage, sizeof(errorMessage));

    if (clearFound == TRUE && errorCode != 0x00)
    {
        errorCode = 0x00;
        event = EV_CLEAR_ERRROR;
        CDC_Transmit_FS(errorClearMessage, sizeof(errorClearMessage));
    }

    if (helpFound == TRUE)
    {
        CDC_Transmit_FS(errorHelpMessage, sizeof(errorHelpMessage));
    }

    return printPrompt();
}

/**
  * @brief
  *
  */

uint8_t TermBB()
{

    uint8_t bbMessageRead[32] = ""; //also for message together
    uint8_t bbMessageWrite[32] = "";

    uint16_t readRegister = 0xffff;
    uint8_t readRegisterFound = 0;
    uint8_t readValue = 0;

    uint16_t writeRegister = 0xffff;
    uint8_t writeRegisterFound = 0;

    uint16_t writeValue = 0xffff;
    uint8_t writeValueFound = 0;

    readRegisterFound = findParameter("-rr", &readRegister, TRUE);
    writeRegisterFound = findParameter("-wr", &writeRegister, TRUE);

    if (readRegisterFound == FALSE && writeRegisterFound == FALSE)
    {
        sprintf((char*) bbMessageRead, "\r\nMust use -rr and/or -wr");
    } else
    {
        if (writeRegisterFound == TRUE)
        {
            writeValueFound = findParameter("-val", &writeValue, TRUE);
            if (writeRegister > 0x0007)
            {
                sprintf((char*) bbMessageWrite,
                        "\r\nWrite register out of range");
            } else if (writeValueFound == FALSE)
            {
                sprintf((char*) bbMessageWrite,
                        "\r\nUse -val to sepecify value");
            } else if (writeValue > 0xff)
            {
                sprintf((char*) bbMessageWrite, "\r\nValue out of range");
            } else
            {
                HAL_I2C_Mem_Write(&hi2c1, BB_ADDR, (uint8_t) writeRegister, 1,
                        (uint8_t*) &writeValue, I2C_MEMADD_SIZE_8BIT, 100); //set msb vref
            }
        }
        if (readRegisterFound == TRUE)
        {
            if (readRegister > 0x0007)
            {
                sprintf((char*) bbMessageRead,
                        "\r\nRead register out of range");
            } else
            {
                HAL_I2C_Mem_Read(&hi2c1, BB_ADDR, (uint8_t) readRegister, 1,
                        &readValue, I2C_MEMADD_SIZE_8BIT, 100);   //set msb vref
                sprintf((char*) bbMessageRead, "\r\nRegister %u = 0x%02X (%u)",
                        readRegister, readValue, readValue);
            }
        }
    }
    CDC_Transmit_FS(bbMessageWrite, sizeof(bbMessageWrite));
    CDC_Transmit_FS(bbMessageRead, sizeof(bbMessageRead));
    return printPrompt();
}

/**
  * @brief
  *
  */

uint8_t printPrompt()
{
    uint8_t prompt[] = "\r\nBB:>";
//    memset(message[historyPosition], ' ', MESSAGE_BUFFER_LENGTH); //clear buffer on every promptm (needed for findParameter)
    return CDC_Transmit_FS(prompt, sizeof(prompt));
}

/**
  * @brief
  *
  */

uint8_t initTerminal()
{
    memset(message[historyPosition], ' ', MESSAGE_BUFFER_LENGTH); //clear buffer on every promptm (needed for findParameter)
    for (uint8_t i = 0; i < MESSAGE_HISTORY_DEPTH; i++)
    {
        memset(message[i], ' ', MESSAGE_BUFFER_LENGTH);
        messagePointer[i] = message[i];
    }
    return CDC_Transmit_FS(logo, sizeof(logo));
}

/**
  * @brief
  *
  */

uint8_t printInvalid()
{
    uint8_t invalid[] = "\r\nCommand not valid in current state";
    CDC_Transmit_FS(invalid, sizeof(invalid));
    return printPrompt();
}

/**
  * @brief
  *
  */

uint8_t printInvalidParameterValue()
{
    uint8_t invalid[] = "\r\nParameter has to be an integer";
    return CDC_Transmit_FS(invalid, sizeof(invalid));
}

/**
  * @brief
  *
  * @param
  * @retval
  */

uint8_t findParameter(char *option, uint16_t *value, uint8_t findValue)
{
    uint8_t found = FALSE;
    char *index = NULL;
    char *valueSearch = NULL;
    char *valueStart = NULL;

    index = strstr((char*) message[historyPosition], option); // find option in message

    if (index == NULL) return found; //return 0 if not found
    // search option string from input line
    if (findValue == TRUE)
    {
        valueSearch = index + strlen(option); // move to after the parameter flag
        while (*valueSearch == ' '
                && valueSearch
                        < (char*) (message[historyPosition]
                                + MESSAGE_BUFFER_LENGTH))
            valueSearch++;      // find first character that is not a [SPACE]
        for (valueStart = valueSearch;
                *valueSearch != ' '
                        && valueSearch
                                < (char*) (message[historyPosition]
                                        + MESSAGE_BUFFER_LENGTH); valueSearch++)
        {
            if (*valueSearch < '0' || *valueSearch > '9')
            {
                printInvalidParameterValue();
                return found; //return 0 if input NaN
            }
        }
    }
    *value = (uint16_t) strtoul(valueStart, &valueSearch, 10);
    found = TRUE;
    return found;
}

/**
  * @brief
  *
  * @param
  * @retval
  */

void clearOutputGenerate(uint8_t *clearOutput, uint8_t *output)
{
    uint8_t returnPrompt[] = "\rBB:>";
    memcpy(clearOutput, returnPrompt, sizeof(returnPrompt));
    memset(&clearOutput[sizeof(returnPrompt)], ' ', MESSAGE_BUFFER_LENGTH);
    memcpy(&clearOutput[MESSAGE_BUFFER_LENGTH + sizeof(returnPrompt)],
            returnPrompt, sizeof(returnPrompt));
    memcpy(&clearOutput[MESSAGE_BUFFER_LENGTH + 2 * sizeof(returnPrompt)],
            output, MESSAGE_BUFFER_LENGTH);
}
//
//
//
//
//
//
