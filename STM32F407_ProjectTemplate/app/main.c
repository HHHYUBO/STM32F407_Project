#include "board.h"
#include <stdio.h>


int main(void)
{
	board_init();
	
	LED_init();
	Serial1_Init(115200); 
	printf("hello");
	// ���ó�ʼ������
	ADC_DMA_TIM_Init();
	// ����ADCת���������δ������
	ADC_SoftwareStartConv(ADC1);
	
	while(1)
	{
		LED1_Turn();
		delay_ms(500);

	}
	

}
