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

static char buffer[2048];
static char final_buffer[2048];

// Add these color definitions at the top of your file
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"

// Debug level definitions

void dMesgPrint(uint8_t debugLevel, const char *format, ...)
{
	const char * color_code = COLOR_WHITE;

	switch(debugLevel) {
		case 0: color_code = COLOR_RED; break;     // Error
		case 1: color_code = COLOR_YELLOW; break;  // Warning
		case 2: color_code = COLOR_GREEN; break;   // Info
		case 3: color_code = COLOR_CYAN; break;    // Debug
		default: color_code = COLOR_WHITE; break;
	}

	va_list args;
	va_start(args, format);
	int len = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (len > 0 && len < sizeof(buffer)) {
		int final_len = snprintf(final_buffer, sizeof(final_buffer), "%s%s%s", color_code, buffer, COLOR_RESET);
		if (final_len > 0 && final_len < sizeof(final_buffer)) {
			HAL_UART_Transmit(&huart2, (uint8_t *) final_buffer, final_len, 1000);
		}
	}

}
void dMesgPrintLwIp(const char *__restrict format, ...)
{
	const char * color_code = COLOR_CYAN;

	va_list args;
	va_start(args, format);
	int len = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (len > 0 && len < sizeof(buffer)) {
		int final_len = snprintf(final_buffer, sizeof(final_buffer), "%s%s%s", color_code, buffer, COLOR_RESET);
		if (final_len > 0 && final_len < sizeof(final_buffer)) {
			HAL_UART_Transmit(&huart2, (uint8_t *) final_buffer, final_len, 1000);
		}
	}
}
