/*
 * fsm.c
 *
 *  Created on: Apr 1, 2024
 *      Author: jakov
 */
#include "fsm.h"
#include "terminal.h"
#include "stm32f0xx_hal.h"
#include "utils.h"

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern I2C_HandleTypeDef hi2c1;
extern ADC_HandleTypeDef hadc;
extern SPI_HandleTypeDef hspi1;
//extern WWDG_HandleTypeDef hwwdg;

extern EVENT event;
extern uint8_t errorCode;

uint32_t v_ctrl;
uint16_t v_set;
uint16_t v_pol;
uint16_t v_tol;

//uint8_t bb_mode_en_buf = 0xb0;
//uint8_t bb_mode_dis_buf = 0x30;
//uint8_t bb_fs_buf = 0x80;

//uint8_t bb_mode_buf = 0xa0;

// Override the weak call back function for TIM3 - used for the timeout
//TODO fix the ocasional double time issue
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM3) {
        errorCode = errorCode | ER_TIMEOUT;
        event = EV_ERROR;
//        toError();

//        event = EV_VOLTAGE_READY;
    }
    if (htim->Instance == TIM2 )
    {

        // output voltage measurement
        uint16_t v_out_raw;
        float v_out;

        uint16_t v_tol_val = v_set * v_tol/100;


        HAL_SPI_Receive(&hspi1, &v_out_raw, 1, 100);


        //TODO test this
        v_out_raw = (v_out_raw & 0x1ff8) >> 2;
		v_out = v_out_raw * 0.4963; // (5V/1024) * (101MOhm/1MOhm) = 0.493V

        if (v_out_raw > 1017) //overvoltage above 505V (505/0.4963 = 1017.5 = 1017)
        {
            errorCode = errorCode | ER_OVERVOLTAGE;
            event = EV_ERROR;
//            toError();
        }

        else if ((v_out > v_set - v_tol_val) && (v_out < v_set + v_tol_val)) //TODO fix here also
		{
//            event = EV_ERROR;
            event = EV_VOLTAGE_READY;
		}
    }
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == SW_nFLT_Pin) {
      errorCode = errorCode | ER_LMG_FAULT;
      event = EV_ERROR;
//      toError();
  } else {
      __NOP();
  }
}

void HAL_WWDG_EarlyWakeupCallback(WWDG_HandleTypeDef *hwwdg)
{
	uint8_t errorMessage[32] = "";
	sprintf((char *)errorMessage, "\r\nError! Code: 0x%02X", errorCode);
	CDC_Transmit_FS(errorMessage, sizeof(errorMessage));
}


//transition actions

/**
  * @brief
  *
  */
void toIdle()
{
    //LEDs
    HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

    //disable V_CTRL
    setupBB(BB_OutputDisable, 0);

    //GPIO peripherals
    HAL_GPIO_WritePin(SET_GPIO_Port, SET_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SW_EN_GPIO_Port, SW_EN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(V_READY_GPIO_Port, V_READY_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(COM_P_GPIO_Port, COM_P_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(COM_N_GPIO_Port, COM_N_Pin, GPIO_PIN_RESET);

}

/**
  * @brief
  *
  */

void toArming()
{
    //LEDs
    HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);

    //enable V_CTRL + set voltage
    setupBB(BB_OutputEnable, v_set);

    //GPIO peripherals
    HAL_GPIO_WritePin(SW_EN_GPIO_Port, SW_EN_Pin, GPIO_PIN_RESET);
//    HAL_GPIO_WritePin(SW_EN_GPIO_Port, SW_EN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(V_READY_GPIO_Port, V_READY_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(COM_P_GPIO_Port, COM_P_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(COM_N_GPIO_Port, COM_N_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SET_GPIO_Port, SET_Pin, GPIO_PIN_SET);

    HAL_TIM_Base_Start_IT(&htim3);
    HAL_TIM_Base_Start_IT(&htim2);
}

/**
  * @brief
  *
  */

void toArmed()
{
    //GPIO peripherals
    HAL_GPIO_WritePin(SET_GPIO_Port, SET_Pin, GPIO_PIN_RESET);          //first disconnect from PSU

    if (v_pol == NEGATIVE){
    	HAL_GPIO_WritePin(COM_N_GPIO_Port, COM_N_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(COM_P_GPIO_Port, COM_P_Pin, GPIO_PIN_SET);    //common node to positive
    } else if (v_pol == POSITIVE){
        HAL_GPIO_WritePin(COM_P_GPIO_Port, COM_P_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(COM_N_GPIO_Port, COM_N_Pin, GPIO_PIN_SET);    //common node to negative
    }
    HAL_GPIO_WritePin(SW_EN_GPIO_Port, SW_EN_Pin, GPIO_PIN_SET);

    HAL_Delay(5);
    HAL_GPIO_WritePin(V_READY_GPIO_Port, V_READY_Pin, GPIO_PIN_SET);    //set ready

    if (v_set > 200) setupBB(BB_OutputDisable, v_set); //to prevent bleeder overheating (P = 1.6W)

    uint8_t armedMessage[] = "\r\nArmed!";
	//LEDs
	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

	CDC_Transmit_FS(armedMessage, sizeof(armedMessage));
	printPrompt();
}

/**
  * @brief
  *
  */

void toError()
{
    //disable V_CTRL
    HAL_GPIO_WritePin(EN_BB_GPIO_Port, EN_BB_Pin, GPIO_PIN_RESET);

    //GPIO peripherals
    HAL_GPIO_WritePin(SET_GPIO_Port, SET_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SW_EN_GPIO_Port, SW_EN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(V_READY_GPIO_Port, V_READY_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(COM_P_GPIO_Port, COM_P_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(COM_N_GPIO_Port, COM_N_Pin, GPIO_PIN_RESET);

    uint8_t errorMessage[32] = "";
	sprintf((char *)errorMessage, "\r\nError! Code: 0x%02X", errorCode);
	//LEDs
	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);

	CDC_Transmit_FS(errorMessage, sizeof(errorMessage));
	printPrompt();
}
