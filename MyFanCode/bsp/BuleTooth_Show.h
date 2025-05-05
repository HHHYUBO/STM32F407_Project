#ifndef __BLUETOOTH_SHOW_H
#define __BLUETOOTH_SHOW_H

#include "board.h"

// 控制模式定义
#define MODE_BUTTON    0  // 按键控制模式
#define MODE_TEMP      1  // 温度控制模式
#define MODE_VOICE     2  // 语音控制模式

// 电池相关函数声明
float Battery_GetVoltage(uint16_t adc_value);
uint8_t Battery_GetPercentage(uint16_t adc_value);

// 温度传感器相关函数声明
float NTC_GetResistance(uint16_t adc_value);
float NTC_GetTemperature(uint16_t adc_value);

// 蓝牙发送参数函数声明
void Bluetooth_SendParameters(void);

#endif /* __BLUETOOTH_SHOW_H */
