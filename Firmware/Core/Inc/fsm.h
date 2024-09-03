/*
 * fsm.h
 *
 *  Created on: Apr 1, 2024
 *      Author: jakov
 */

#ifndef INC_FSM_H_
#define INC_FSM_H_

#include "stm32f0xx_hal.h"
#include "main.h"

//#define BB_ADDR 0x74 << 1

typedef enum //states
{
    ST_IDLE = 0U,
    ST_ARMING,
    ST_ARMED,
//    ST_INJECTING,
    ST_ERROR = 0xFF
} STATE;

typedef enum //state transition events
{
    EV_NULL = 0U,
    EV_ARM,
    EV_DISARM,
    EV_VOLTAGE_READY,
    EV_TRIGGER,
    EV_CLEAR_ERRROR,
    EV_ERROR = 0XFF
} EVENT;

//typedef enum //buck-boost converter register map
//{
//    BB_REF_LSB = 0x00,
//    BB_REF_MSB,
//    BB_ILIM,
//    BB_SR,
//    BB_FS,
//    BB_CDC,
//    BB_MODE,
//    BB_STATUS
//} BB_REG;


//STATE stateMachine(STATE currentState);

//transition actions
void toIdle();
void toArming();
void toArmed();
void toError();
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
//state actions
void Arming();

#endif /* INC_FSM_H_ */
