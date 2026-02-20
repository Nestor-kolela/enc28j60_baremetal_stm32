/*
 * log.h
 *
 *  Created on: Oct 25, 2025
 *      Author: NK KALAMBAY
 */

#ifndef INC_LOG_H_
#define INC_LOG_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define DEBUG_ERROR   0
#define DEBUG_WARN    1
#define DEBUG_INFO    2
#define DEBUG_DEBUG   3
#define DEBUG_VERBOSE 4


void dMesgPrint_impl(uint8_t level, const char *file, int line, const char *func, const char *format, ...);
void dMesgPrintLwIp_impl(const char *file, int line, const char *func, const char *format, ...);

#define dMesgPrint(level, fmt, ...) \
    dMesgPrint_impl(level, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define dMesgPrintLwIp(fmt, ...) \
    dMesgPrintLwIp_impl(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#endif /* INC_LOG_H_ */
