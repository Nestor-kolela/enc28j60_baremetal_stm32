/*
 * log.c
 *
 *  Created on: Oct 25, 2025
 *      Author: NK KALAMBAY
 */

#include "log.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "main.h"

extern UART_HandleTypeDef huart2;

/* ================== CONFIG ================== */

#define LOG_TMP_BUF_SIZE   1024
#define UART_TX_BUF_SIZE   1024

/* ================== BUFFERS ================== */

static uint8_t uart_tx_buf[UART_TX_BUF_SIZE];

static volatile uint16_t tx_head;
static volatile uint16_t tx_tail;
static volatile uint8_t dma_busy;
static volatile uint16_t dma_len;

/* Debug / diagnostics */
volatile uint32_t uart_dropped;

/* temp buffers (static = DMA safe, no stack pressure) */
static char buffer[LOG_TMP_BUF_SIZE];
static char final_buffer[LOG_TMP_BUF_SIZE];

/* ================== COLORS ================== */

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"

/* ================== INTERNAL ================== */

static void uart_start_dma(void)
{
    if (dma_busy) return;
    if (tx_head == tx_tail) return;

    if (huart2.gState != HAL_UART_STATE_READY)
        return;

    uint16_t len;

    if (tx_head > tx_tail)
        len = tx_head - tx_tail;
    else
        len = UART_TX_BUF_SIZE - tx_tail;

    if (len == 0 || len > UART_TX_BUF_SIZE)
        return;

    dma_busy = 1;
    dma_len = len;

    if (HAL_UART_Transmit_DMA(&huart2, &uart_tx_buf[tx_tail], len) != HAL_OK)
    {
        dma_busy = 0;
    }
}

static void uart_write_dma(uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        __disable_irq();

        uint16_t next = (tx_head + 1) % UART_TX_BUF_SIZE;

        if (next == tx_tail)
        {
            uart_dropped++;
            __enable_irq();
            break;
        }

        uart_tx_buf[tx_head] = data[i];
        tx_head = next;

        __enable_irq();
    }

    __disable_irq();
    uart_start_dma();
    __enable_irq();
}

/* ================== CALLBACK ================== */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        tx_tail = (tx_tail + dma_len) % UART_TX_BUF_SIZE;
        dma_busy = 0;

        uart_start_dma();
    }
}

/* ================== LOG FUNCTIONS ================== */

void dMesgPrint_impl(uint8_t debugLevel,
                     const char *file,
                     int line,
                     const char *func,
                     const char *format, ...)
{
    const char * color_code = COLOR_WHITE;

    switch(debugLevel) {
        case 0: color_code = COLOR_RED; break;
        case 1: color_code = COLOR_YELLOW; break;
        case 2: color_code = COLOR_GREEN; break;
        case 3: color_code = COLOR_CYAN; break;
        default: color_code = COLOR_WHITE; break;
    }

    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len < 0) return;

    if (len >= sizeof(buffer))
        len = sizeof(buffer) - 1;

    int final_len = snprintf(final_buffer, sizeof(final_buffer),
            "%s[%s:%d:%s] %s%s",
            color_code, file, line, func, buffer, COLOR_RESET);

    if (final_len < 0) return;

    if (final_len >= sizeof(final_buffer))
        final_len = sizeof(final_buffer) - 1;

    uart_write_dma((uint8_t *)final_buffer, (uint16_t)final_len);
}

void dMesgPrintLwIp_impl(const char *file,
                         int line,
                         const char *func,
                         const char *format, ...)
{
    const char * color_code = COLOR_MAGENTA;

    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len < 0) return;

    if (len >= sizeof(buffer))
        len = sizeof(buffer) - 1;

    int final_len = snprintf(final_buffer, sizeof(final_buffer),
            "%s[LWIP %s:%d:%s] %s%s",
            color_code, file, line, func, buffer, COLOR_RESET);

    if (final_len < 0) return;

    if (final_len >= sizeof(final_buffer))
        final_len = sizeof(final_buffer) - 1;

    uart_write_dma((uint8_t *)final_buffer, (uint16_t)final_len);
}
