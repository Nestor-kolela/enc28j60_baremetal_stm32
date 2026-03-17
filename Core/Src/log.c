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

#include "main.h"

extern UART_HandleTypeDef huart2;

/* ================== CONFIG ================== */

#define UART_TX_BUF_SIZE 4096

/* ================== BUFFERS ================== */

static uint8_t uart_tx_buf[UART_TX_BUF_SIZE];
static volatile uint16_t tx_head = 0;
static volatile uint16_t tx_tail = 0;
static volatile uint8_t dma_busy = 0;
static uint16_t dma_len = 0;

/* temp format buffers (same as yours) */
static char buffer[2048];
static char final_buffer[2048];

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

    dma_busy = 1;

    if (tx_head > tx_tail)
    {
        dma_len = tx_head - tx_tail;
    }
    else
    {
        dma_len = UART_TX_BUF_SIZE - tx_tail;
    }

    HAL_UART_Transmit_DMA(&huart2, &uart_tx_buf[tx_tail], dma_len);
}

static void uart_write_dma(uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        uint16_t next = (tx_head + 1) % UART_TX_BUF_SIZE;

        // Buffer full → drop data (non-blocking)
        if (next == tx_tail)
            break;

        uart_tx_buf[tx_head] = data[i];
        tx_head = next;
    }

    uart_start_dma();
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

void dMesgPrint_impl(uint8_t debugLevel, const char *file, int line, const char *func, const char *format, ...)
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

    if (len > 0 && len < sizeof(buffer)) {

        int final_len = snprintf(final_buffer, sizeof(final_buffer),
                "%s[%s:%d:%s] %s%s",
                color_code, file, line, func, buffer, COLOR_RESET);

        if (final_len > 0 && final_len < sizeof(final_buffer)) {
            uart_write_dma((uint8_t *)final_buffer, final_len);
        }
    }
}

void dMesgPrintLwIp_impl(const char *file, int line, const char *func, const char *format, ...)
{
    const char * color_code = COLOR_MAGENTA;

    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len > 0 && len < sizeof(buffer)) {

        int final_len = snprintf(final_buffer, sizeof(final_buffer),
                "%s[LWIP %s:%d:%s] %s%s",
                color_code, file, line, func, buffer, COLOR_RESET);

        if (final_len > 0 && final_len < sizeof(final_buffer)) {
            uart_write_dma((uint8_t *)final_buffer, final_len);
        }
    }
}
