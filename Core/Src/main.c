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
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include "arch/sys_arch.h"
#include "lwip/timeouts.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum Connection_State {
	START_UP,
	DHCP_IP_RECEIVED,
	DNS_IP_OBTAINED,
	MQTT_CONNECTING,
	MQTT_CONNECTED,
	MQTT_DISCONNECTED
}Connection_State;

struct mqttBrokerDetails {
	const char * name;
	ip_addr_t ip;
	const char * user;
	const char * password;
};

struct mqttBrokerDetails mqttBroker = {
		//.name = "test.mosquitto.org",
		//.name = "broker.hivemq.com",
		.name = "broker.emqx.io",
		.ip = {0},
		.user = "",
		.password = ""
};
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

static volatile uint32_t enc28j60intCounter;
static volatile uint32_t u32FinerTimer;
static volatile uint32_t u32CoarseTimer;
static volatile uint8_t timeForDns;
static volatile uint8_t timerForEtharp;
static volatile uint32_t u32MqttCounter;
static volatile bool dnsRequest;
static struct netif my_netif;
static struct dhcp myDhcpClient;
static enum Connection_State myConn = START_UP;
static mqtt_client_t * myMqtt = NULL;
static bool bSubscribed = false;
static uint32_t rx_bytes = 0;
static uint32_t lastReconnectAttempt = 0;
static bool bTxBusy = false;

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

static void ipObtained(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
static void myMqttClientCallBack(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void publishIncoming(void *arg, const char *topic, u32_t tot_len);
static void mqtt_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
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
  netif_add_noaddr(&my_netif, NULL, ethernet_init, ethernet_input);
  netif_set_addr(&my_netif, IP4_ADDR_ANY, IP4_ADDR_ANY, IP4_ADDR_ANY);

  // Set the interface as the default
  netif_set_default(&my_netif);

  // Bring up the interface
  netif_set_up(&my_netif);

  uint32_t u32PacketCounter = 0;

  dhcp_set_struct(&my_netif, &myDhcpClient);
  dhcp_start(&my_netif);

  bool enableDns = false;
  myMqtt = (mqtt_client_t *) mqtt_client_new();

  if(!myMqtt) while(1);

  static const struct mqtt_connect_client_info_t myInfo =
  {
      .client_id = "mqtt-nestor-kalambay",
      .client_user = NULL,
      .client_pass = NULL,

      .keep_alive = 60,

      .will_topic = NULL,
      .will_msg = "offline",
      .will_msg_len = 7,
      .will_qos = 0,
      .will_retain = 0
  };

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(u32FinerTimer >= 1) {
		  u32FinerTimer = 0;
		  dhcp_fine_tmr();
		  HAL_GPIO_TogglePin(GreenLED1_GPIO_Port, GreenLED1_Pin);
	  }

	  if(u32CoarseTimer >= 120) {
		  u32CoarseTimer = 0;
		  dhcp_coarse_tmr();
	  }

	  if(timeForDns >= 2 && enableDns == true) {
		  timeForDns = 0;
		  dns_tmr();
	  }

	  if(timerForEtharp >= 2) {
		  timerForEtharp = 0;
		  etharp_tmr();
	  }


	  sys_check_timeouts();

	  switch(myConn) {
	  case START_UP:
		  if(dhcp_supplied_address(&my_netif)) {

			  dns_init();
			  ip_addr_t dnsServer;
			  IP4_ADDR(&dnsServer, 8, 8, 8, 8);
			  dns_setserver(0, &dnsServer);

			  ip_addr_t mosquitoIp;
			  dns_gethostbyname(mqttBroker.name, &mosquitoIp, ipObtained, NULL);

			  enableDns = true;
			  myConn = DHCP_IP_RECEIVED;
		  }
		  break;
	  case DHCP_IP_RECEIVED:

		  break;
	  case MQTT_DISCONNECTED:
	  case DNS_IP_OBTAINED:
		    if(sys_now() - lastReconnectAttempt > 3000) {
		        lastReconnectAttempt = sys_now();
		        if(ERR_OK == mqtt_client_connect(myMqtt, &mqttBroker.ip, 1883,
		                                         myMqttClientCallBack,
		                                         (void *) &myConn,
		                                         &myInfo))
		        {
		            myConn = MQTT_CONNECTING;
		            bSubscribed = false;
		        }
		    }
		  break;
	  case MQTT_CONNECTING:
		  if(mqtt_client_is_connected(myMqtt) == 1) {

		  }
		  break;

	  case MQTT_CONNECTED:
		  if(u32MqttCounter >= 10) {
			  u32MqttCounter = 0;
			  const char* message = "This message is coming from an alien who just landed in south Africa";
			  (void) mqtt_publish(myMqtt, "franzkafka", message, strlen(message), 0, 0, NULL, NULL);
		  }

		  if(bSubscribed == false) {
			  bSubscribed = true;
			  mqtt_set_inpub_callback(myMqtt, publishIncoming, mqtt_data_cb, NULL);
			  //mqtt_subscribe(myMqtt, "$SYS/broker/uptime", 0, NULL, NULL);
			  mqtt_subscribe(myMqtt, "franzkafka", 0, NULL, NULL);
		  }
		  break;
	  }

	  if(enc28j60intCounter > 0) {
		  //Process the packet then decrement by one.
		  enc28j60intCounter--;

		  uint8_t u8Value = 0;
		  u8Value = enc28j60_readEtherReg(&dev, dev.bank0.commonRegs.EIR);
		  dMesgPrint(DEBUG_INFO, "EIR REG --> %u\r\n", u8Value);

		  for (uint8_t cnt = 0; cnt < 8; cnt++) {
			if (u8Value & (1 << cnt)) {
				//Allow for further interrupts to happen by making the pin go back high
				enc28j60_global_int_Clear(&dev);

				switch (cnt) {
				default:
					break;
				case 0:
					dMesgPrint(DEBUG_ERROR, "1) Receive Error Interrupt Flag bit\r\n");
					enc28j60_clear_interrupt(&dev, RXERIF);
					break;

				case 1:
					dMesgPrint(DEBUG_ERROR, "2) Transmit Error Interrupt Flag bit\r\n");
					enc28j60_clear_interrupt(&dev, TXERIF);
					break;

				case 2:
					dMesgPrint(DEBUG_INFO, "3) WOL Interrupt Flag bit\r\n");
					//enc28j60_clear_interrupt(&dev, WOLIF);
					break;

				case 3:
					dMesgPrint(DEBUG_INFO, "4) Transmit Interrupt Flag bit\r\n");
					enc28j60_clear_interrupt(&dev, TXIF);
					bTxBusy = false;
					break;

				case 4:
					dMesgPrint(DEBUG_INFO, "5) Link Change Interrupt Flag bit\r\n");
					(void) enc28j60_readPhyReg(&dev, dev.phyReg.PHIR);
					enc28j60_link_change_int_clear(&dev);
					break;

				case 5:
					dMesgPrint(DEBUG_INFO, "5) DMA Interrupt Flag bit\r\n");
					enc28j60_clear_interrupt(&dev, DMAIF);
					break;

				case 6:
					dMesgPrint(DEBUG_INFO, "6) Receive Packet Pending Interrupt Flag bit\r\n");

					bool err = enc28j60_etherReceive(&dev);

					enc28j60_clear_interrupt(&dev, PKTIF);

					if (err == true) {
						//Packet number
						dMesgPrint(DEBUG_INFO, "PKT number %d\r\n", u32PacketCounter++);

						//Packet length
						dMesgPrint(DEBUG_INFO, "PKT length %d\r\n", dev.rxPkt.rxPktLen.u16PktLen);

						//Let do the translation from array to pbuf
						uint16_t u18length = dev.rxPkt.rxPktLen.u16PktLen;
						struct pbuf * ethBuffer = pbuf_alloc(PBUF_RAW, u18length, PBUF_POOL);
						if(ethBuffer != NULL) {
							ethernet_do_translation_to_pbub(&dev, ethBuffer);
							if (netif_input(ethBuffer, &my_netif) != ERR_OK) {
								pbuf_free(ethBuffer);
							}
						}
					}

					break;
				}

				//Clear the global interrupt bit.
				enc28j60_global_Int_Set(&dev);

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
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
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
	  timeForDns++;
	  u32MqttCounter++;
	  timerForEtharp++;
	  sys_now_increment();
  }

  /* USER CODE END Callback 1 */
}
err_t ethernet_init(struct netif *netif) {
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

err_t enc28j60_translate(struct netif *netif, struct pbuf *p) {
	if(bTxBusy != true) {
		dMesgPrint(DEBUG_INFO, "TX started!\r\n");
		struct pbuf *q;
	    uint16_t total_len = p->tot_len;
	    uint16_t offset = 0;

	    /* Copy the full chained pbuf into linear ENC buffer */
	    for (q = p; q != NULL; q = q->next)
	    {
	        memcpy(&dev.txPkt.data[offset], q->payload, q->len);
	        offset += q->len;
	    }

	    /* Safety: offset must equal total length */
	    if (offset != total_len) {
	        return ERR_BUF;
	    }

	    (void) enc28j60_etherTransmit(&dev, dev.txPkt.data, total_len);

	    bTxBusy = true;
	    return ERR_OK;
	}else {
		dMesgPrint(DEBUG_ERROR, "TX busy!\r\n");
		return ERR_INPROGRESS;
	}
}

void ethernet_do_translation_to_pbub(enc28j60Drv * dev, struct pbuf *p) {
	uint16_t len = dev->rxPkt.rxPktLen.u16PktLen;
    if (len > p->tot_len) len = p->tot_len;
	pbuf_take(p, dev->rxPkt.data, len);
}

static void ipObtained(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
	if(strcmp(mqttBroker.name, name) == 0) {
		mqttBroker.ip = *ipaddr;
		myConn = DNS_IP_OBTAINED;
	}
}

static void myMqttClientCallBack(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
	enum Connection_State * ptr = (enum Connection_State * ) arg;
	if(client == myMqtt) {
		switch(status) {
		case MQTT_CONNECT_ACCEPTED:
 			*ptr = (enum Connection_State) MQTT_CONNECTED;
			break;
		case MQTT_CONNECT_DISCONNECTED:
			*ptr = (enum Connection_State) MQTT_DISCONNECTED;
			break;
		default:
			while(1);
			break;
		}
	}
}

static void mqtt_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    rx_bytes += len;

    if(flags & MQTT_DATA_FLAG_LAST)
    {
        dMesgPrint(DEBUG_INFO, "MQTT message complete: %lu bytes\r\n", rx_bytes);
        rx_bytes = 0;
    }
}

static void publishIncoming(void *arg, const char *topic, u32_t tot_len) {
	dMesgPrint(DEBUG_INFO, "Data arrived on %s\r\n", topic);
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
