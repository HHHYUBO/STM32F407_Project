#ifndef __TIMER_H
#define __TIMER_H

#include "board.h"

extern volatile uint32_t sys_millis;

void Timer2_Init(void);
uint32_t millis(void);

#endif
