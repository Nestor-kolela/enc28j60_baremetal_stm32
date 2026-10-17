#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "lwip/arch.h"   /* pulls in arch/cc.h and the u32_t typedefs */

u32_t sys_now(void);
void sys_now_increment(void);
