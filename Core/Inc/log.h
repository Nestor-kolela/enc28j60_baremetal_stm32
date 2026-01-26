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


void dMesgPrint(uint8_t debugLevel, const char *format, ...);
void dMesgPrintLwIp(const char *__restrict x, ...);

#endif /* INC_LOG_H_ */
