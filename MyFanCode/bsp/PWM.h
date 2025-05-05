#ifndef __PWM_H
#define __PWM_H

#include "board.h"

void PWM_TIM3_Init(void);
void PWM_TIM3_SetDuty(uint8_t duty);

#endif /* __PWM_H */
