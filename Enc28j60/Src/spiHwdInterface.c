/*
 * spi2Stm32Interface.c
 *
 *  Created on: Feb 8, 2025
 *      Author: NK KALAMBAY
 */

#include "spiHwdInterface.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

#define MAX_TIME_OUT			1000
#define SPI_HANDLE				&hspi1

#define Ethernet_CS_GPIO_Port	ENC28J60_CS_GPIO_Port
#define Ethernet_CS_Pin			ENC28J60_CS_Pin

void spi1ChipSelect(void)
{
	HAL_GPIO_WritePin(Ethernet_CS_GPIO_Port, Ethernet_CS_Pin, GPIO_PIN_RESET);
}

void spi1ChipDeSelect(void)
{
	HAL_GPIO_WritePin(Ethernet_CS_GPIO_Port, Ethernet_CS_Pin, GPIO_PIN_SET);
}

uint16_t spi1Read(uint8_t * ptrData, uint16_t u16length)
{
	return HAL_SPI_Receive(SPI_HANDLE, ptrData, u16length, MAX_TIME_OUT) == HAL_OK ? 1 : 0;
}

void spi1Write(uint8_t * ptrData, uint16_t u16length)
{
	HAL_SPI_Transmit(SPI_HANDLE, ptrData, u16length, MAX_TIME_OUT);
}

void delayMsFunction(uint32_t ms)
{
	HAL_Delay(ms);
}
