#include "board.h"

#define Train 1
#define Recognize 0
static uint8_t ASR_Mode = Train;

int main(void)
{
	board_init();
	
	LED_init();
	Key_Init();
	Timer2_Init();
	Serial1_Init(115200); 
	ADC_DMA_Init();
	W25QXX_Init();
	ASR_Init();
	ASR_CNN_Init();
	
	while(1){	
		if(GetKeyNum()==1)
		{				
			// 启动ADC采集
			ADC_Start();
			
			// 等待ADC采集完成
			while(!adc_conversion_complete)
			{
				delay_ms(10);
			}  
				
			if(ASR_Mode == Recognize)
			{
				// 执行语音识别
				uint8_t result;
				float32_t confidence;
				// 使用封装好的语音识别函数
				uint8_t success = ASR_RecognizeSpeech(adc_buffer, ADC_BUFFER_SIZE, &result, &confidence);
				// 处理识别结果
				ASR_HandleRecognitionResult(result, confidence);
			}
			else if(ASR_Mode == Train)
			{
				ASR_ExtractFeaturesOnly(adc_buffer,ADC_BUFFER_SIZE);
				// 发送采集结果
				ASR_SendCNNTrainingData();
			}
			
			// 重置ADC完成标志
			adc_conversion_complete = 0;
		}
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
