#ifndef __SPI_H
#define __SPI_H

#include "board.h"

/* 函数声明 */
void SPI1_Init(void);
void SPI1_SetSpeed(uint16_t SPI_BaudRatePrescaler);
uint8_t SPI1_ReadWriteByte(uint8_t TxData);

#endif /* __SPI_H */
