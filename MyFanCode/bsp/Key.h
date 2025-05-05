#ifndef __KEY_H
#define __KEY_H

#include "board.h"

volatile extern uint8_t KeyNum;

void Key_Init(void);
uint8_t GetKeyNum(void);
uint8_t Key_GetState(void);
void Key_Tick(void);

// ¼ì²â°´¼ü1
uint8_t GetKey1(void);
uint8_t GetKey2(void);

#endif
