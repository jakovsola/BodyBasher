/*
 * utils.c
 *
 *  Created on: Apr 14, 2024
 *      Author: jakov
 */
#include "utils.h"
#include "math.h"

extern I2C_HandleTypeDef hi2c1;

/**
  * @brief
  *
  * @param
  * @retval
  */

void setupBB(uint8_t outputEnable, uint16_t setpoint_HV){


    //setpoint
	uint8_t test[32] = "";
	uint16_t offset = 0;
	uint16_t offsetsArray[] = {0, OFF_50, OFF_100, OFF_150, OFF_200, OFF_250, OFF_300, OFF_350, OFF_400, OFF_450, OFF_500};
	float gradientsArray[] = {0, GRAD_50_100, GRAD_100_150, GRAD_150_200, GRAD_200_250, GRAD_250_300, GRAD_300_350, GRAD_350_400, GRAD_400_450, GRAD_450_500};
//
//
	uint16_t v;
	for (v = 2; v <= 10; v++)
	{
		if(setpoint_HV < v * 50)
		{
			offset = round(((float) offsetsArray[v-1] + (float) (setpoint_HV - (v - 1) * 50) * gradientsArray[v-1]/50));
			break;
		}
	}
	if(setpoint_HV == 500) offset = OFF_500;
	else if(setpoint_HV < 50) offset = OFF_50;

	uint16_t setpoint = (setpoint_HV << 2) - offset; //approximate conversion from 500V range to 2^11 - offset (0.4V)
    setpoint = 0x07ff & setpoint; //mask out the reserved bits
    uint8_t bb_reflsb = (uint8_t) (setpoint & 0x00ff);
    uint8_t bb_refmsb = (uint8_t) ((setpoint & 0xff00) >> 8);

    uint8_t bb_fs_buf = 0x80;
    uint8_t bb_mode_buf = 0x30 | outputEnable << 7;

    HAL_GPIO_WritePin(EN_BB_GPIO_Port, EN_BB_Pin, GPIO_PIN_SET); //exit shutdown
    HAL_I2C_Mem_Write(&hi2c1, BB_ADDR, BB_FS, 1, &bb_fs_buf, I2C_MEMADD_SIZE_8BIT, 100);        //set feedback external

    HAL_I2C_Mem_Write(&hi2c1, BB_ADDR, BB_REF_LSB, 1, &bb_reflsb, I2C_MEMADD_SIZE_8BIT, 100);   //set lsb vref
    HAL_I2C_Mem_Write(&hi2c1, BB_ADDR, BB_REF_MSB, 1, &bb_refmsb, I2C_MEMADD_SIZE_8BIT, 100);   //set msb vref

    HAL_I2C_Mem_Write(&hi2c1, BB_ADDR, BB_MODE, 1, &bb_mode_buf, I2C_MEMADD_SIZE_8BIT, 100);    //output enable

}
