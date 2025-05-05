#include "ADC.h"

uint16_t adc_buffer[ADC_BUFFER_SIZE];
volatile uint8_t adc_conversion_complete = 0;

// ===================== 第一部分：DMA传输ADC配置 =====================
void ADC_DMA_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
    
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);/* 使能GPIOC时钟 */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);/* 使能DMA2时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);/* 使能ADC1时钟 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);/* 使能TIM4时钟 */
	
	/* 配置PC5为模拟输入模式 (ADC通道15) */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* 配置DMA2 Stream0 Channel0 用于ADC1 */
	DMA_InitStructure.DMA_Channel = DMA_Channel_0;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&adc_buffer;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = ADC_BUFFER_SIZE;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	
	DMA_Init(DMA2_Stream0, &DMA_InitStructure);
	
	/* 配置DMA中断 */
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	/* 清除DMA标志 */
	DMA_ClearFlag(DMA2_Stream0, DMA_FLAG_TCIF0 | DMA_FLAG_HTIF0 | DMA_FLAG_TEIF0 | DMA_FLAG_DMEIF0 | DMA_FLAG_FEIF0);
	
	/* 初始化时不启用DMA中断，等到ADC_Start()函数调用时再启用 */
	DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, DISABLE);
	
	/* 使能DMA2 Stream0 */
	DMA_Cmd(DMA2_Stream0, ENABLE);
	
	/* ADC通用配置 */
	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4; // ADCCLK = PCLK2/4 = 84/4 = 21MHz
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);
	
	/* ADC1配置 */
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE; // 单通道模式
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; // 禁用连续转换
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T4_CC4; // 使用TIM4的CC4作为触发源
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 1;
	ADC_Init(ADC1, &ADC_InitStructure);
	
	/* 配置ADC通道 */
	ADC_RegularChannelConfig(ADC1, ADC_Channel_15, 1, ADC_SampleTime_28Cycles);
	
	/* 使能ADC1 DMA */
	ADC_DMACmd(ADC1, ENABLE);
	
	/* 使能ADC1 */
	ADC_Cmd(ADC1, ENABLE);
	
	/* 
	 * 配置定时器4
	 * 系统时钟为168MHz，APB1时钟为42MHz
	 * 当APB1预分频不为1时，定时器的时钟频率为APB1时钟的2倍，即84MHz
	 * 要获得8kHz的频率，我们需要设置：
	 * 分频系数 = 84MHz / 8kHz = 10500
	 * 因此，我们设置预分频为10499（需要减1），自动重装载值为0
	 */
	TIM_TimeBaseStructure.TIM_Period = 9;
	TIM_TimeBaseStructure.TIM_Prescaler = 1049;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	/* 配置TIM4的CC4通道 */
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 5;  // 设置占空比为50%
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC4Init(TIM4, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);

	/* 设置TIM4的输出触发源为CC4事件 */
	TIM_SelectOutputTrigger(TIM4, TIM_TRGOSource_OC4Ref);

	/* 使能TIM4的预装载寄存器 */
	TIM_ARRPreloadConfig(TIM4, ENABLE);

	/* 初始化时不使能TIM4，等到ADC_Start()函数调用时再使能 */
	TIM_Cmd(TIM4, DISABLE);
}

// ===================== 第二部分：温度传感器和电池电压ADC配置 =====================
void TemAndVol_ADC_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	
	// 使能ADC2时钟和GPIOA时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	
	// 配置PA7为模拟输入（温度通道）
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	// 配置PA6为模拟输入（电压通道）
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	// ADC公共参数配置
	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);
	
	// ADC2参数配置
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE; // 非扫描模式
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; // 单次转换模式
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None; // 软件触发
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 1;
	ADC_Init(ADC2, &ADC_InitStructure);
	
	// 使能ADC2
	ADC_Cmd(ADC2, ENABLE);
}

/**
 * @brief 获取温度通道的ADC值
 * @return 温度通道的ADC转换结果
 */
uint16_t ADC_GetTemperature(void)
{
	ADC_RegularChannelConfig(ADC2, ADC_Channel_7, 1, ADC_SampleTime_144Cycles);
	ADC_SoftwareStartConv(ADC2);
	while(ADC_GetFlagStatus(ADC2, ADC_FLAG_EOC) == RESET);
	return ADC_GetConversionValue(ADC2);
}

/**
 * @brief 获取电压通道的ADC值
 * @return 电压通道的ADC转换结果
 */
uint16_t ADC_GetVoltage(void)
{
	ADC_RegularChannelConfig(ADC2, ADC_Channel_6, 1, ADC_SampleTime_144Cycles);
	ADC_SoftwareStartConv(ADC2);
	while(ADC_GetFlagStatus(ADC2, ADC_FLAG_EOC) == RESET);
	return ADC_GetConversionValue(ADC2);
}

// 修改ADC_Start函数以确保开始新的采样序列
void ADC_Start(void)
{
    adc_conversion_complete = 0;
    
    // 停止定时器和DMA
    TIM_Cmd(TIM4, DISABLE);
    DMA_Cmd(DMA2_Stream0, DISABLE);
    while(DMA_GetCmdStatus(DMA2_Stream0) != DISABLE);
    
    // 使用memset清空缓冲区，比循环更高效
    memset(adc_buffer, 0, sizeof(adc_buffer));
    
    // 重新配置DMA
    DMA_DeInit(DMA2_Stream0);
    
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_Channel = DMA_Channel_0;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&adc_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = ADC_BUFFER_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // 改为Normal模式而非Circular
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(DMA2_Stream0, &DMA_InitStructure);
    
    DMA_ClearFlag(DMA2_Stream0, DMA_FLAG_TCIF0 | DMA_FLAG_HTIF0 | DMA_FLAG_TEIF0 | DMA_FLAG_DMEIF0 | DMA_FLAG_FEIF0);
    DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, ENABLE);
    DMA_Cmd(DMA2_Stream0, ENABLE);
    
    // 重新配置ADC
    // 不需要完全重新配置ADC，只需要重新启用即可
    ADC_Cmd(ADC1, DISABLE);
    delay_ms(1);
    
    // 重新启用ADC DMA请求
    ADC_DMACmd(ADC1, DISABLE);
    ADC_DMACmd(ADC1, ENABLE);
    
    ADC_Cmd(ADC1, ENABLE);
    delay_ms(1);
    
    // 重新配置定时器
    TIM_Cmd(TIM4, DISABLE);
    TIM_SetCounter(TIM4, 0);
    TIM_Cmd(TIM4, ENABLE);
}

// 修改中断处理函数
void DMA2_Stream0_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA2_Stream0, DMA_IT_TCIF0))
    {
        DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);
        TIM_Cmd(TIM4, DISABLE); // 停止定时器
        
        // 不需要禁用DMA，因为在Normal模式下，传输完成后DMA会自动禁用
        
        adc_conversion_complete = 1; // 设置标志
    }
}

// 在主循环中调用此函数来检查并打印结果
void Check_And_Print_ADC(void)
{
    static uint16_t print_index = 0;
    const uint16_t BATCH_SIZE = 100; // 每次打印100个数据点
    
    if(adc_conversion_complete)
    {
        if(print_index == 0)
        {
            printf("ADC conversion results:\r\n");
        }
        
        // 发送所有数据，而不是只发送一批
        while(print_index < ADC_BUFFER_SIZE)
        {
            // 分批打印数据
            uint16_t end_index = print_index + BATCH_SIZE;
            if(end_index > ADC_BUFFER_SIZE)
            {
                end_index = ADC_BUFFER_SIZE;
            }
            
            for(uint16_t i = print_index; i < end_index; i++)
            {
                printf("Sample %d: Raw=%d\r\n", i, adc_buffer[i]);
            }
            
            print_index = end_index;
            
            // 添加短暂延时，让串口有时间发送数据
            delay_ms(50);
        }
        
        // 打印完成后重置索引和标志
        print_index = 0;
        adc_conversion_complete = 0;
    }
}
