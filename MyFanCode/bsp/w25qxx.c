#include "w25qxx.h"

/* 片选信号控制 */
#define W25QXX_CS_LOW()     GPIO_ResetBits(GPIOA, GPIO_Pin_15)
#define W25QXX_CS_HIGH()    GPIO_SetBits(GPIOA, GPIO_Pin_15)

// 数据缓冲区
uint8_t WriteBuffer[256];
uint8_t ReadBuffer[256];

uint16_t W25QXX_TYPE = W25Q128;  // 默认是W25Q128

/**
 * @brief  初始化W25QXX
 * @param  无
 * @retval 无
 */
void W25QXX_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* 使能GPIOA时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    
    /* 配置PA15为输出，用于片选 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* 初始化SPI1 */
    SPI1_Init();
    SPI1_SetSpeed(SPI_BaudRatePrescaler_4); // 设置为21M时钟,高速模式
    
    /* 片选信号拉高，取消片选 */
    W25QXX_CS_HIGH();
    
    /* 读取Flash ID */
    W25QXX_TYPE = W25QXX_ReadID();
    
    /* 打印Flash ID信息 */
    printf("W25QXX ID: 0x%X\r\n", W25QXX_TYPE);
    
    /* 检查是否为W25Q128 */
    if(W25QXX_TYPE == 0xEF17)
    {
        printf("W25Q128 detected!\r\n");
    }
    else
    {
        printf("Unknown Flash type or Flash not found!\r\n");
    }
}

/**
 * @brief  读取W25QXX的状态寄存器
 * @param  regno: 状态寄存器号，范围:1~3
 * @retval 状态寄存器值
 */
uint8_t W25QXX_ReadSR(uint8_t regno)
{
    uint8_t byte = 0, command = 0;
    
    switch(regno)
    {
        case 1:
            command = W25X_ReadStatusReg1;  // 读状态寄存器1指令
            break;
        case 2:
            command = W25X_ReadStatusReg2;  // 读状态寄存器2指令
            break;
        case 3:
            command = W25X_ReadStatusReg3;  // 读状态寄存器3指令
            break;
        default:
            command = W25X_ReadStatusReg1;
            break;
    }
    
    W25QXX_CS_LOW();                        // 使能器件
    SPI1_ReadWriteByte(command);            // 发送读取状态寄存器命令
    byte = SPI1_ReadWriteByte(0xFF);        // 读取一个字节
    W25QXX_CS_HIGH();                       // 取消片选
    
    return byte;
}

/**
 * @brief  写W25QXX状态寄存器
 * @param  regno: 状态寄存器号，范围:1~3
 * @param  sr: 要写入的值
 * @retval 无
 */
void W25QXX_Write_SR(uint8_t regno, uint8_t sr)
{
    uint8_t command = 0;
    
    switch(regno)
    {
        case 1:
            command = W25X_WriteStatusReg1;  // 写状态寄存器1指令
            break;
        case 2:
            command = W25X_WriteStatusReg2;  // 写状态寄存器2指令
            break;
        case 3:
            command = W25X_WriteStatusReg3;  // 写状态寄存器3指令
            break;
        default:
            command = W25X_WriteStatusReg1;
            break;
    }
    
    W25QXX_CS_LOW();                        // 使能器件
    SPI1_ReadWriteByte(command);            // 发送写状态寄存器命令
    SPI1_ReadWriteByte(sr);                 // 写入一个字节
    W25QXX_CS_HIGH();                       // 取消片选
}

/**
 * @brief  W25QXX写使能
 * @param  无
 * @retval 无
 */
void W25QXX_Write_Enable(void)
{
    W25QXX_CS_LOW();                        // 使能器件
    SPI1_ReadWriteByte(W25X_WriteEnable);   // 发送写使能
    W25QXX_CS_HIGH();                       // 取消片选
}

/**
 * @brief  W25QXX写禁止
 * @param  无
 * @retval 无
 */
void W25QXX_Write_Disable(void)
{
    W25QXX_CS_LOW();                        // 使能器件
    SPI1_ReadWriteByte(W25X_WriteDisable);  // 发送写禁止指令
    W25QXX_CS_HIGH();                       // 取消片选
}

/**
 * @brief  读取芯片ID
 * @param  无
 * @retval 芯片ID
 */
uint16_t W25QXX_ReadID(void)
{
    uint16_t Temp = 0;
    
    W25QXX_CS_LOW();
    SPI1_ReadWriteByte(0x90);               // 发送读取ID命令
    SPI1_ReadWriteByte(0x00);
    SPI1_ReadWriteByte(0x00);
    SPI1_ReadWriteByte(0x00);
    Temp |= SPI1_ReadWriteByte(0xFF) << 8;  // 读取制造商ID
    Temp |= SPI1_ReadWriteByte(0xFF);       // 读取设备ID
    W25QXX_CS_HIGH();
    
    return Temp;
}

/**
 * @brief  读取SPI FLASH
 * @param  pBuffer: 数据存储区
 * @param  ReadAddr: 开始读取的地址(24bit)
 * @param  NumByteToRead: 要读取的字节数(最大65535)
 * @retval 无
 */
void W25QXX_Read(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)
{
    uint16_t i;
    
    W25QXX_CS_LOW();                        // 使能器件
    SPI1_ReadWriteByte(W25X_ReadData);      // 发送读取命令
    SPI1_ReadWriteByte((uint8_t)((ReadAddr) >> 16));  // 发送24bit地址
    SPI1_ReadWriteByte((uint8_t)((ReadAddr) >> 8));
    SPI1_ReadWriteByte((uint8_t)ReadAddr);
    
    for(i = 0; i < NumByteToRead; i++)
    {
        pBuffer[i] = SPI1_ReadWriteByte(0xFF);  // 循环读数
    }
    
    W25QXX_CS_HIGH();                       // 取消片选
}

/**
 * @brief  在指定地址开始写入最大256字节的数据
 * @param  pBuffer: 数据存储区
 * @param  WriteAddr: 开始写入的地址(24bit)
 * @param  NumByteToWrite: 要写入的字节数(最大256),该数不应该超过该页的剩余字节数
 * @retval 无
 */
void W25QXX_Write_Page(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
    uint16_t i;
    
    W25QXX_Write_Enable();                  // 写使能
    
    W25QXX_CS_LOW();                        // 使能器件
    SPI1_ReadWriteByte(W25X_PageProgram);   // 发送写页命令
    SPI1_ReadWriteByte((uint8_t)((WriteAddr) >> 16)); // 发送24bit地址
    SPI1_ReadWriteByte((uint8_t)((WriteAddr) >> 8));
    SPI1_ReadWriteByte((uint8_t)WriteAddr);
    
    for(i = 0; i < NumByteToWrite; i++)
    {
        SPI1_ReadWriteByte(pBuffer[i]);     // 循环写数
    }
    
    W25QXX_CS_HIGH();                       // 取消片选
    W25QXX_Wait_Busy();                     // 等待写入结束
}

/**
 * @brief  无检验写SPI FLASH
 * @note   必须确保所写的地址范围内的数据全部为0XFF,否则在非0XFF处写入的数据将失败!
 * @param  pBuffer: 数据存储区
 * @param  WriteAddr: 开始写入的地址(24bit)
 * @param  NumByteToWrite: 要写入的字节数(最大65535)
 * @retval 无
 */
void W25QXX_Write_NoCheck(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
    uint16_t pageremain;
    
    pageremain = 256 - WriteAddr % 256;     // 单页剩余的字节数
    
    if(NumByteToWrite <= pageremain)
        pageremain = NumByteToWrite;        // 不大于256个字节
    
    while(1)
    {
        W25QXX_Write_Page(pBuffer, WriteAddr, pageremain);
        
        if(NumByteToWrite == pageremain)
            break;                          // 写入结束了
        else                                // NumByteToWrite > pageremain
        {
            pBuffer += pageremain;
            WriteAddr += pageremain;
            NumByteToWrite -= pageremain;
            
            if(NumByteToWrite > 256)
                pageremain = 256;           // 一次可以写入256个字节
            else
                pageremain = NumByteToWrite;// 不够256个字节了
        }
    }
}

/**
 * @brief  等待空闲
 * @param  无
 * @retval 无
 */
void W25QXX_Wait_Busy(void)
{
    while((W25QXX_ReadSR(1) & 0x01) == 0x01);  // 等待BUSY位清空
}

/**
 * @brief  擦除整个芯片
 * @param  无
 * @retval 无
 */
void W25QXX_Erase_Chip(void)
{
    W25QXX_Write_Enable();                  // 写使能
    W25QXX_Wait_Busy();                     // 等待空闲
    
    W25QXX_CS_LOW();                        // 使能器件
    SPI1_ReadWriteByte(W25X_ChipErase);     // 发送片擦除命令
    W25QXX_CS_HIGH();                       // 取消片选
    
    W25QXX_Wait_Busy();                     // 等待芯片擦除结束
}

/**
 * @brief  擦除一个扇区
 * @param  Dst_Addr: 扇区地址 根据实际容量设置
 * @retval 无
 */
void W25QXX_Erase_Sector(uint32_t Dst_Addr)
{
    Dst_Addr *= 4096;                       // 扇区地址
    W25QXX_Write_Enable();                  // 写使能
    W25QXX_Wait_Busy();                     // 等待空闲
    
    W25QXX_CS_LOW();                        // 使能器件
    SPI1_ReadWriteByte(W25X_SectorErase);   // 发送扇区擦除指令
    SPI1_ReadWriteByte((uint8_t)((Dst_Addr) >> 16)); // 发送24bit地址
    SPI1_ReadWriteByte((uint8_t)((Dst_Addr) >> 8));
    SPI1_ReadWriteByte((uint8_t)Dst_Addr);
    W25QXX_CS_HIGH();                       // 取消片选
    
    W25QXX_Wait_Busy();                     // 等待擦除完成
}

/**
 * @brief  擦除一个块
 * @param  Dst_Addr: 块地址 根据实际容量设置
 * @retval 无
 */
void W25QXX_Erase_Block(uint32_t Dst_Addr)
{
    Dst_Addr *= 65536;                      // 块地址
    W25QXX_Write_Enable();                  // 写使能
    W25QXX_Wait_Busy();                     // 等待空闲
    
    W25QXX_CS_LOW();                        // 使能器件
    SPI1_ReadWriteByte(W25X_BlockErase);    // 发送块擦除指令
    SPI1_ReadWriteByte((uint8_t)((Dst_Addr) >> 16)); // 发送24bit地址
    SPI1_ReadWriteByte((uint8_t)((Dst_Addr) >> 8));
    SPI1_ReadWriteByte((uint8_t)Dst_Addr);
    W25QXX_CS_HIGH();                       // 取消片选
    
    W25QXX_Wait_Busy();                     // 等待擦除完成
}

/**
 * @brief  写SPI FLASH
 * @note   在指定地址开始写入指定长度的数据
 * @param  pBuffer: 数据存储区
 * @param  WriteAddr: 开始写入的地址(24bit)
 * @param  NumByteToWrite: 要写入的字节数(最大65535)
 * @retval 0:成功, 1:失败
 */
uint8_t W25QXX_Write(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
    uint32_t secpos;
    uint16_t secoff;
    uint16_t secremain;
    uint16_t i;
    static uint8_t W25QXX_BUF[4096];  // 使用静态分配而不是动态分配
    uint8_t result = 0;
    
    secpos = WriteAddr / 4096;              // 扇区地址
    secoff = WriteAddr % 4096;              // 在扇区内的偏移
    secremain = 4096 - secoff;              // 扇区剩余空间大小
    
    if(NumByteToWrite <= secremain)
        secremain = NumByteToWrite;         // 不大于4096个字节
    
    while(1)
    {
        W25QXX_Read(W25QXX_BUF, secpos * 4096, 4096); // 读出整个扇区的内容
        
        for(i = 0; i < secremain; i++)      // 校验数据
        {
            if(W25QXX_BUF[secoff + i] != 0xFF)
                break;                      // 需要擦除
        }
        
        if(i < secremain)                   // 需要擦除
        {
            W25QXX_Erase_Sector(secpos);    // 擦除这个扇区
            
            for(i = 0; i < secremain; i++)  // 复制
            {
                W25QXX_BUF[secoff + i] = pBuffer[i];
            }
            
            W25QXX_Write_NoCheck(W25QXX_BUF, secpos * 4096, 4096); // 写入整个扇区
        }
        else
        {
            W25QXX_Write_NoCheck(pBuffer, WriteAddr, secremain); // 写已经擦除了的,直接写入扇区剩余区间
        }
        
        if(NumByteToWrite == secremain)
        {
            break;                          // 写入结束了
        }
        else                                // 写入未结束
        {
            secpos++;                       // 扇区地址增1
            secoff = 0;                     // 偏移位置为0
            
            pBuffer += secremain;           // 指针偏移
            WriteAddr += secremain;         // 写地址偏移
            NumByteToWrite -= secremain;    // 字节数递减
            
            if(NumByteToWrite > 4096)
                secremain = 4096;           // 下一个扇区还是写不完
            else
                secremain = NumByteToWrite; // 下一个扇区可以写完了
        }
    }
    
    return result;  // 返回操作结果
}

/**
 * @brief  进入掉电模式
 * @param  无
 * @retval 无
 */
void W25QXX_PowerDown(void)
{
    W25QXX_CS_LOW();                        // 使能器件
    SPI1_ReadWriteByte(W25X_PowerDown);     // 发送掉电命令
    W25QXX_CS_HIGH();                       // 取消片选
    delay_us(3);                            // 等待TPD
}

/**
 * @brief  唤醒
 * @param  无
 * @retval 无
 */
void W25QXX_WAKEUP(void)
{
    W25QXX_CS_LOW();                        // 使能器件
    SPI1_ReadWriteByte(W25X_ReleasePowerDown); // 发送唤醒命令
    W25QXX_CS_HIGH();                       // 取消片选
    delay_us(3);                            // 等待TRES1
}



void W25QXX_Test(void)
{
    uint16_t i;
    uint16_t id;
    uint8_t result = 0;
    
    printf("W25QXX test start...\r\n");
    
    // 初始化W25QXX
    W25QXX_Init();
    
    // 读取芯片ID
    id = W25QXX_ReadID();
    printf("W25QXX ID: 0x%X\r\n", id);
    
    if(id == W25Q128) // 检查是否为W25Q128
    {
        printf("successfully detected!\r\n");
        
        // 准备测试数据
        for(i = 0; i < 256; i++)
        {
            WriteBuffer[i] = i;
        }
        
        printf("Erasing...\r\n");
        // 擦除第0个扇区
        W25QXX_Erase_Sector(0);
        
        printf("writing...\r\n");
        // 写入数据到第0个扇区
        result = W25QXX_Write(WriteBuffer, 0, 256);
        if(result == 0)
        {
            printf("write finish!\r\n");
        }
        else
        {
            printf("write failed!\r\n");
            return;
        }
        
        // 清空读缓冲区
        for(i = 0; i < 256; i++)
        {
            ReadBuffer[i] = 0;
        }
        
        printf("reading...\r\n");
        // 读取数据
        W25QXX_Read(ReadBuffer, 0, 256);
        
        // 验证数据
        for(i = 0; i < 256; i++)
        {
            if(ReadBuffer[i] != WriteBuffer[i])
            {
                printf("Data validation failed! loc:%d, write:%d, read:%d\r\n", i, WriteBuffer[i], ReadBuffer[i]);
                return;
            }
        }
        
        printf("Data validation successfully!\r\n");
        
        // 测试掉电和唤醒功能
        printf("Test the power-down mode...\r\n");
        W25QXX_PowerDown();
        delay_ms(100);
        
        printf("wakeup Flash...\r\n");
        W25QXX_WAKEUP();
        delay_ms(10);
        
        // 再次读取数据验证
        for(i = 0; i < 256; i++)
        {
            ReadBuffer[i] = 0;
        }
        
        W25QXX_Read(ReadBuffer, 0, 256);
        
        // 验证数据
        for(i = 0; i < 256; i++)
        {
            if(ReadBuffer[i] != WriteBuffer[i])
            {
                printf("wakeup validation failed!\r\n");
                return;
            }
        }
        
        printf("wakeup validation finish!\r\n");
        printf("W25QXX test finish!\r\n");
    }
    else
    {
        printf("detected failed! ID:0x%X\r\n", id);
    }
}
