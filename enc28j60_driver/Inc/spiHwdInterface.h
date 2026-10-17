/*
 * spi2Stm32Interface.h
 *
 *  Created on: Feb 8, 2025
 *      Author: NK KALAMBAY
 */

#ifndef SPIHWDINTERFACE_H_
#define SPIHWDINTERFACE_H_

#include "main.h"



void spi1ChipSelect(void);
void spi1ChipDeSelect(void);
uint16_t spi1Read(uint8_t * ptrData, uint16_t u16length);
void spi1Write(uint8_t * ptrData, uint16_t u16length);
void delayMsFunction(uint32_t ms);
#endif /* SPIHWDINTERFACE_H_ */
