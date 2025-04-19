#include "Key.h"

volatile uint8_t KeyNum = 0;

void Key_Init(void)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);// 使能 GPIOA 时钟
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN; // 不使用上拉或下拉电阻
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}


uint8_t GetKeyNum(void)
{
	uint8_t temp;
	if(KeyNum)
	{
		temp = KeyNum;
		KeyNum = 0;
		return temp;
	}
	return 0;
}

uint8_t Key_GetState(void)
{
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_2) == 1) 
		return 1;
	else if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_3) == 1) 
		return 2;
	else 
		return 0;
}

void Key_Tick(void)
{
	static uint8_t Count = 0;
	static uint8_t CurrState, PrevState;
	
	Count++;
	if(Count >= 20)
	{
		Count = 0;
		PrevState = CurrState;
		CurrState = Key_GetState();
		
		if(CurrState == 0 && PrevState != 0)
		{
			KeyNum = PrevState;
		}
	}
}





