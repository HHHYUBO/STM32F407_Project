#include "LED.h"

void LED_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	GPIO_SetBits(GPIOD,GPIO_Pin_9);
	GPIO_SetBits(GPIOD,GPIO_Pin_10);
}

void LED1_ON(void)
{
	GPIO_ResetBits(GPIOD,GPIO_Pin_9);
}

void LED1_OFF(void)
{
	GPIO_SetBits(GPIOD,GPIO_Pin_9);
}

void LED1_Turn(void)
{
	if(GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_9) == 1)
		GPIO_ResetBits(GPIOD,GPIO_Pin_9);
	else
		GPIO_SetBits(GPIOD,GPIO_Pin_9);
}

void LED2_ON(void)
{
	GPIO_ResetBits(GPIOD,GPIO_Pin_10);
}

void LED2_OFF(void)
{
	GPIO_SetBits(GPIOD,GPIO_Pin_10);
}

void LED2_Turn(void)
{
	if(GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_10) == 1)
		GPIO_ResetBits(GPIOD,GPIO_Pin_10);
	else
		GPIO_SetBits(GPIOD,GPIO_Pin_10);
}
