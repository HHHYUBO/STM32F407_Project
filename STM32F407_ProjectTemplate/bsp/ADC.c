#include "ADC.h"

#define BUFFER_SIZE 1
uint16_t adcBuffer[BUFFER_SIZE]; // 用于DMA存储ADC数据

void ADC_DMA_TIM_Init(void)
{
    // 使能ADC1时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    // 使能DMA2时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    // 使能定时器2时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    // 使能 GPIOA 时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;

    // 配置 PA5 为模拟模式（用于 ADC 输入）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; // PA5
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN; // 模拟模式
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; // 不使用上拉或下拉电阻
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // GPIO 速度设置
    GPIO_Init(GPIOA, &GPIO_InitStructure); // 初始化 GPIOA

    // 配置DMA
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_Channel = DMA_Channel_0; // ADC通道0
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR; // ADC数据寄存器地址
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&adcBuffer; // 数据存储位置
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory; // 从外设到内存
    DMA_InitStructure.DMA_BufferSize = 1; // 单次传输数据长度
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // 外设地址不递增
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; // 内存地址递增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // 外设数据大小为16位
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord; // 内存数据大小为16位
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; // 环形模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_High; // DMA传输优先级
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable; // 不使用FIFO
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single; // 内存突发传输方式
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single; // 外设突发传输方式
    DMA_Init(DMA2_Stream0, &DMA_InitStructure);

    // 使能DMA传输
    DMA_Cmd(DMA2_Stream0, ENABLE);
    
    // 设置 DMA2 Stream0 中断优先级
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  // 设置抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;  // 设置响应优先级
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;  // 使能中断
    NVIC_Init(&NVIC_InitStructure);

    // 使能 DMA 中断
    DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, ENABLE);

    // 配置ADC
    ADC_InitTypeDef ADC_InitStructure;
    ADC_StructInit(&ADC_InitStructure);
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b; // 12位分辨率
    ADC_InitStructure.ADC_ScanConvMode = DISABLE; // 单通道
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE; // 连续转换模式
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConv_T3_TRGO; // 不使用外部触发
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; // 右对齐
    ADC_InitStructure.ADC_NbrOfConversion = 1; // 只有一个通道
    ADC_Init(ADC1, &ADC_InitStructure);

    // 配置ADC通道5（PA5）
    ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 1, ADC_SampleTime_3Cycles); // 采样时间3个时钟周期

    // 使能ADC
    ADC_Cmd(ADC1, ENABLE);

    // 使能DMA请求
    ADC_DMACmd(ADC1, ENABLE);

    // 配置定时器2
    TIM_TimeBaseInitTypeDef TIM_InitStructure;
    TIM_InitStructure.TIM_Period = 10499; // 设置周期
    TIM_InitStructure.TIM_Prescaler = 7;  // 设置预分频
    TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_InitStructure);

    // 使能定时器
    TIM_Cmd(TIM2, ENABLE);

    // 使能定时器2更新中断
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    // 启动ADC转换
    ADC_SoftwareStartConv(ADC1);
}

// DMA 中断处理函数
void DMA2_Stream0_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_TCIF0))  // 检查 DMA 传输完成标志
    {
        // 数据传输完成，输出数据
        printf("ADC Value: %d\n", adcBuffer[0]);  // 通过串口输出采集的数据

        // 清除 DMA 中断标志
        DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);

        // 重新启动 ADC 转换，确保连续采样
        ADC_SoftwareStartConv(ADC1);
    }
}
