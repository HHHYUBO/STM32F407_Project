#ifndef __MOTOR_H
#define __MOTOR_H

#include "board.h"

// 控制模式枚举
typedef enum {
    MOTOR_MODE_KEY = 0,    // 按键控制模式
    MOTOR_MODE_TEMP = 1,   // 温度控制模式
    MOTOR_MODE_VOICE = 2   // 语音控制模式
} Motor_ModeTypeDef;

// 电机控制函数声明
void Motor_Init(void);
void Motor_SetSpeed(uint16_t speed);
void Motor_KeyControl(void);
void Motor_TempControl(void);
void Motor_VoiceControl(void);
void Motor_ModeSwitch(void);
void Motor_Control(void);

uint8_t Motor_GetSpeedLevel(void);
uint8_t Motor_GetControlMode(void);

#endif /* __MOTOR_H */
