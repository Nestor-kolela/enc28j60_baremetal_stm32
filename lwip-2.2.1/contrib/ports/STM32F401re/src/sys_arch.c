#include "arch/sys_arch.h"

static uint32_t millisec;
u32_t sys_now(void)
{
	return millisec;
}

void sys_now_increment(void) {
	millisec += 500;
}
