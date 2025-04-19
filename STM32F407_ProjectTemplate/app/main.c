#include "board.h"
#include <stdio.h>


int main(void)
{
	board_init();
	
	LED_init();
	Serial1_Init(115200); 
	printf("hello");
	// 调用初始化函数
	ADC_DMA_TIM_Init();
	// 启动ADC转换（如果尚未启动）
	ADC_SoftwareStartConv(ADC1);
	
	while(1)
	{
		LED1_Turn();
		delay_ms(500);

	}
	

}
