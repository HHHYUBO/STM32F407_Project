#include "spi.h"

/**
 * @brief  初始化SPI1
 * @param  无
 * @retval 无
 */
void SPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;
    
    /* 使能SPI1和GPIO时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    
    /* 配置SPI1引脚: SCK(PB3), MISO(PB4) and MOSI(PB5) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    /* 连接SPI1引脚到AF5 */
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI1);
    
    /* SPI1配置 */
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  // 设置SPI单向或者双向的数据模式
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                       // 设置SPI工作模式:设置为主SPI
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;                   // 设置SPI的数据大小:SPI发送接收8位帧结构
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;                         // 串行同步时钟的空闲状态为高电平
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;                        // 串行同步时钟的第二个跳变沿（上升或下降）数据被采样
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;                           // NSS信号由软件控制
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;// 定义波特率预分频的值
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;                  // 指定数据传输从MSB位开始
    SPI_InitStructure.SPI_CRCPolynomial = 7;                            // CRC值计算的多项式
    SPI_Init(SPI1, &SPI_InitStructure);
    
    /* 使能SPI1 */
    SPI_Cmd(SPI1, ENABLE);
}

/**
 * @brief  设置SPI1速度
 * @param  SPI_BaudRatePrescaler: SPI_BaudRatePrescaler_2~SPI_BaudRatePrescaler_256
 * @retval 无
 */
void SPI1_SetSpeed(uint16_t SPI_BaudRatePrescaler)
{
    assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));
    
    /* 清除SPI1的波特率位 */
    SPI1->CR1 &= 0XFFC7;
    
    /* 设置SPI1的波特率 */
    SPI1->CR1 |= SPI_BaudRatePrescaler;
    
    /* 使能SPI1 */
    SPI_Cmd(SPI1, ENABLE);
}

/**
 * @brief  SPI1读写一个字节
 * @param  TxData: 要写入的字节
 * @retval 读取到的字节
 */
uint8_t SPI1_ReadWriteByte(uint8_t TxData)
{
    uint8_t retry = 0;
    
    /* 等待发送缓冲区为空 */
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
    {
        retry++;
        if(retry > 200) return 0;
    }
    
    /* 发送一个字节 */
    SPI_I2S_SendData(SPI1, TxData);
    retry = 0;
    
    /* 等待接收缓冲区非空 */
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET)
    {
        retry++;
        if(retry > 200) return 0;
    }
    
    /* 返回接收到的数据 */
    return SPI_I2S_ReceiveData(SPI1);
}
