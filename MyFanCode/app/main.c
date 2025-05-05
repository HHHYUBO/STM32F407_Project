#include "board.h"

int main(void)
{
	board_init();
	
	LED_init();
	Key_Init();
	Timer2_Init();
	Serial1_Init(115200); 
	TemAndVol_ADC_Config();
	Motor_Init();
	ADC_DMA_Init();
	W25QXX_Init();
	ASR_Init();
	
	while(1){		
		Motor_ModeSwitch();
		Motor_Control();
		Bluetooth_SendParameters();
		delay_ms(100);
	}
}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{	
		sys_millis++;  
		Key_Tick();
		
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
