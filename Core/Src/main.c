/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "enc28j60_hwd.h"
#include "spiHwdInterface.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "netif/ethernet.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/etharp.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
extern enc28j60Drv dev;

#if 0
static ip4_addr_t ip = {.addr = 1};
static ip4_addr_t gw = {.addr = 1};
static ip4_addr_t msk = {.addr = 4294967040};
#endif

static volatile uint32_t enc28j60intCounter = 0;
static volatile uint32_t u32FinerTimer = 0;
static volatile uint32_t u32CoarseTimer = 0;
static struct netif my_netif;
static struct dhcp myDhcpClient;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
err_t ethernet_init(struct netif *netif);
err_t enc28j60_translate(struct netif *netif, struct pbuf *p);
void ethernet_do_translation_to_pbub(enc28j60Drv * dev, struct pbuf *p);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start_IT(&htim2);

  enc28j60_initDr(&dev, spi1ChipSelect, spi1ChipDeSelect, spi1Read, spi1Write, NULL, delayMsFunction);
  enc28j60_strtDr(&dev);

  lwip_init();

  // Add network interface
  //netif_add(&my_netif, &ip, &msk, &gw, NULL, ethernet_init, ethernet_input);
  netif_add_noaddr(&my_netif, NULL, ethernet_init, ethernet_input);
  netif_set_addr(&my_netif, IP4_ADDR_ANY, IP4_ADDR_ANY, IP4_ADDR_ANY);

  // Set the interface as the default
  netif_set_default(&my_netif);

  // Bring up the interface
  netif_set_up(&my_netif);

  uint32_t u32PacketCounter = 0;
  uint8_t u8PktCount = 0;
  uint8_t u8Value = 0;
  uint8_t * u8TempValue = NULL;

  dhcp_set_struct(&my_netif, &myDhcpClient);
  dhcp_start(&my_netif);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(u32FinerTimer >= 1)
	  {
		  dhcp_fine_tmr();
		  HAL_GPIO_TogglePin(GreenLED1_GPIO_Port, GreenLED1_Pin);
		  u32FinerTimer = 0;
	  }

	  if(u32CoarseTimer >= 120)
	  {
		  u32CoarseTimer = 0;
		  dhcp_coarse_tmr();
	  }
	  if(enc28j60intCounter > 0)
	  {
		  //Process the packet then decrement by one.
		  enc28j60intCounter--;

			u8PktCount = enc28j60_readEtherReg(&dev, dev.bank1.EPKTCNT);
			dMesgPrint(DEBUG_INFO, "EPKTCNT count --> %d\r\n", u8PktCount);

			volatile uint8_t u8DeviceId = enc28j60_readEtherReg(&dev, dev.bank3.EREVID);

			dMesgPrint(DEBUG_INFO, "Device ID: 0x%02X\r\n", u8DeviceId);

			u8Value = enc28j60_readEtherReg(&dev, dev.bank0.commonRegs.EIR);
			dMesgPrint(DEBUG_INFO, "EIR REG --> %u\r\n", u8Value);

			//u16FullDuplex = enc28j60_readPhyReg(&dev, dev.phyReg.PHSTAT2);
			//dMesgPrint(DEBUG_INFO, "PHY Duplex Status: %d\r\n", u16FullDuplex);

			for (uint8_t cnt = 0; cnt < 8; cnt++) {

				if (u8Value & (1 << cnt)) {
					//Allow for further interrupts to happen by making the pin go back high
					enc28j60_BitFieldClear(&dev, dev.bank0.commonRegs.EIE, 1 << 7);

					switch (cnt) {
					case 0:
						dMesgPrint(DEBUG_ERROR, "1) Receive Error Interrupt Flag bit\r\n");
						enc28j60_BitFieldClear(&dev, dev.bank0.commonRegs.EIR, 1 << cnt);
						break;

					case 1:
						dMesgPrint(DEBUG_ERROR, "2) Transmit Error Interrupt Flag bit\r\n");
						enc28j60_BitFieldClear(&dev, dev.bank0.commonRegs.EIR, 1 << cnt);
						break;

					case 2:
						dMesgPrint(DEBUG_INFO, "3) WOL Interrupt Flag bit\r\n");
						break;

					case 3:
						dMesgPrint(DEBUG_INFO, "4) Transmit Interrupt Flag bit\r\n");
						enc28j60_BitFieldClear(&dev, dev.bank0.commonRegs.EIR, 1 << cnt);
						break;

					case 4:
						dMesgPrint(DEBUG_INFO, "5) Link Change Interrupt Flag bit\r\n");
						(void) enc28j60_readPhyReg(&dev, dev.phyReg.PHIR);
						enc28j60_BitFieldSet(&dev, dev.bank0.commonRegs.EIE, 1 << cnt);

						break;

					case 5:
						dMesgPrint(DEBUG_INFO, "5) DMA Interrupt Flag bit\r\n");
						enc28j60_BitFieldClear(&dev, dev.bank0.commonRegs.EIR, 1 << cnt);
						break;

					case 6:
						dMesgPrint(DEBUG_INFO, "6) Receive Packet Pending Interrupt Flag bit\r\n");
						if(u8PktCount > 0)
						{
							bool err;
							err = enc28j60_etherReceive(&dev, u8TempValue, 0);
							if (err == true) {
								//Packet number
								dMesgPrint(DEBUG_INFO, "PKT number %d\r\n", u32PacketCounter++);

								//Packet length
								dMesgPrint(DEBUG_INFO, "PKT length %d\r\n", dev.rxPkt.rxPktLen.u16PktLen);

								//Let do the translation from array to pbuf
								uint16_t u18length = dev.rxPkt.rxPktLen.u16PktLen;
								struct pbuf * ethBuffer = pbuf_alloc(PBUF_LINK, u18length, PBUF_REF);
								if(ethBuffer != NULL)
								{
									ethernet_do_translation_to_pbub(&dev, ethBuffer);
									netif_input(ethBuffer, &my_netif);
								}
							}
						}

						break;
					}

					//1) Here
					//Clear the interrupt bit.
					enc28j60_BitFieldSet(&dev, dev.bank0.commonRegs.EIE, 1 << 7);
				}
			}
	  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 9999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 3999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GreenLED1_GPIO_Port, GreenLED1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ENC28J60_CS_GPIO_Port, ENC28J60_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : B1_Pin ENC28J60_INT_Pin */
  GPIO_InitStruct.Pin = B1_Pin|ENC28J60_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : GreenLED1_Pin */
  GPIO_InitStruct.Pin = GreenLED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GreenLED1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ENC28J60_CS_Pin */
  GPIO_InitStruct.Pin = ENC28J60_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(ENC28J60_CS_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == GPIO_PIN_7)
	{
		//send a notification to the task
		enc28j60intCounter++;
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {

  }
  /* USER CODE BEGIN Callback 1 */
  else if (htim->Instance == TIM2)
  {
	  u32FinerTimer++;
	  u32CoarseTimer++;
  }

  /* USER CODE END Callback 1 */
}
err_t ethernet_init(struct netif *netif)
{
	netif->name[0] = 'e';
	netif->name[1] = 'n';
	//
	netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP \
			| NETIF_FLAG_IGMP \
			| NETIF_FLAG_LINK_UP;
	netif->mtu = 1500;
	netif->output = etharp_output;
	netif->linkoutput = enc28j60_translate;
	netif->hwaddr_len = ETH_HWADDR_LEN;
	netif_set_hostname(netif, "enc28j60_feytech");
	for(uint8_t i = 0; i < 6; i++)
	{
		netif->hwaddr[i] =  dev.mac.macSrcAddr[5-i];
	}
	return ERR_OK;
}

static uint8_t enc28j60_buffer[1500];
err_t enc28j60_translate(struct netif *netif, struct pbuf *p)
{
	//This function should transmit
	uint16_t length = p->len;
	memcpy(enc28j60_buffer, (uint8_t *) p->payload, length);
	enc28j60_etherTransmit(&dev, enc28j60_buffer, length);
	return ERR_OK;
}


void ethernet_do_translation_to_pbub(enc28j60Drv * dev, struct pbuf *p)
{
	p->next = NULL;
	p->len = dev->rxPkt.rxPktLen.u16PktLen;
	p->payload = dev->rxPkt.data;
	memcpy((uint8_t *) p->payload, dev->rxPkt.data, dev->rxPkt.rxPktLen.u16PktLen);
	p->ref = 1;
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
