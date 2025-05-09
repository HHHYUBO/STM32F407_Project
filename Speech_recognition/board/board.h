/*
 * 立创开发板软硬件资料与相关扩展板软硬件资料官网全部开源
 * 开发板官网：www.lckfb.com
 * 技术支持常驻论坛，任何技术问题欢迎随时交流学习
 * 立创论坛：https://oshwhub.com/forum
 * 关注bilibili账号：【立创开发板】，掌握我们的最新动态！
 * 不靠卖板赚钱，以培养中国工程师为己任
 * 

 Change Logs:
 * Date           Author       Notes
 * 2024-03-07     LCKFB-LP    first version
 */
#ifndef __BOARD_H__
#define __BOARD_H__

#include "stm32f4xx.h"

#include <ctype.h>
#include <stdbool.h>
#include "stdio.h"	
#include <stdarg.h>
#include "float.h"
#include "string.h"	 
#include "math.h"
#include "stdlib.h" 
#include "arm_math.h"

#include "LED.h"
#include "Serial.h"
#include "ADC.h"
#include "Key.h"
#include "Timer.h"
#include "ASR.h"
#include "w25qxx.h"
#include "spi.h"
#include "ASR_CNN.h"


void board_init(void);
void delay_us(uint32_t _us);
void delay_ms(uint32_t _ms);
void delay_1ms(uint32_t ms);
void delay_1us(uint32_t us);

#endif
