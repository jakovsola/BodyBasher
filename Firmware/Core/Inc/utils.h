/*
 * utils.h
 *
 *  Created on: Apr 14, 2024
 *      Author: jakov
 */

#ifndef INC_UTILS_H_
#define INC_UTILS_H_

#include "stm32f0xx_hal.h"
#include "main.h"

#define BB_OutputEnable 1
#define BB_OutputDisable 0

#define OFF_50 59 //50.08
#define OFF_100 61 //99.95
#define OFF_150 66 //149.94
#define OFF_200 76 //200.15
#define OFF_250 80 //249.9
#define OFF_300 83 //399.88
#define OFF_350 80 //349.98
#define OFF_400 79 //399.96
#define OFF_450 77 //449.88
#define OFF_500 68 //499.86

#define GRAD_50_100 (OFF_100 - OFF_50)
#define GRAD_100_150 (OFF_150 - OFF_100)
#define GRAD_150_200 (OFF_200 - OFF_150)
#define GRAD_200_250 (OFF_250 - OFF_200)
#define GRAD_250_300 (OFF_300 - OFF_250)
#define GRAD_300_350 (OFF_350 - OFF_300)
#define GRAD_350_400 (OFF_400 - OFF_350)
#define GRAD_400_450 (OFF_450 - OFF_400)
#define GRAD_450_500 (OFF_500 - OFF_450)



void setupBB(uint8_t outputEnable, uint16_t setpoint_HV);


#endif /* INC_UTILS_H_ */

