#include "board.h"
#include <stdio.h>


int main(void)
{
	board_init();
	
	LED_init();
	Key_Init();
	Timer2_Init();
	Serial1_Init(115200); 
	ADC_DMA_Init();
	
	while(1){		
		if(GetKeyNum()==1)
		{
			delay_ms(500); // 等待ADC采集开始
			ADC_Start();
			delay_ms(1000); // 等待ADC采集完成
			
			// 循环调用Check_And_Print_ADC直到所有数据都被打印完毕
			while(1)
			{
				Check_And_Print_ADC();
				
				// 如果所有数据都已打印完毕，退出循环
				if(adc_conversion_complete == 0)
				{
					break;
				}
				
				delay_ms(5); // 短暂延时，避免串口缓冲区溢出
			}
		}
	}
}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{	
		sys_millis++;  // ÿ���ж�����1ms
		Key_Tick();
		
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
