#include "board.h"
#include <stdio.h>
#include "stm32f4xx.h"   

/**
  * @brief  初始化TIM3 PWM模式，PC8输出
  * @param  None
  * @retval None
  * @note   频率1kHz，分辨率1%，默认占空比0%
  */
void PWM_TIM3_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    /* 使能TIM3和GPIOC时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    
    /* 配置PC8为复用功能 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    /* 将PC8连接到TIM3_CH3 */
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource8, GPIO_AF_TIM3);
    
    /* 配置时基单元 
     * 假设APB1时钟为84MHz (STM32F407默认配置)
     * 要实现1kHz频率，分辨率为1%:
     * 周期 = 84000000 / (PSC+1) / (ARR+1) = 1000Hz
     * 选择PSC=83, ARR=999, 则:
     * 84000000 / 84 / 1000 = 1000Hz
     * 分辨率为1% = 100个等级，所以ARR=999正好满足要求
     */
    TIM_TimeBaseStructure.TIM_Period = 999;
    TIM_TimeBaseStructure.TIM_Prescaler = 83;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    /* 配置输出比较单元 */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0; // 占空比0%
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    
    /* PC8对应TIM3的通道3 */
    TIM_OC3Init(TIM3, &TIM_OCInitStructure);
    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
    
    /* 使能TIM3 */
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}

/**
  * @brief  设置PWM占空比
  * @param  duty: 占空比值，范围0-100
  * @retval None
  */
void PWM_TIM3_SetDuty(uint8_t duty)
{
    uint16_t pulse;
    
    if(duty > 100)
        duty = 100;
    
    pulse = (uint16_t)((999 * duty) / 100);
    TIM_SetCompare3(TIM3, pulse);
}

