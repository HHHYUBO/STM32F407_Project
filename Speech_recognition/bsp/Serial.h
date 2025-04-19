#ifndef __SERIAL_H
#define __SERIAL_H

#include "board.h"

#define BSP_USART_RCC       RCC_APB2Periph_USART1
#define BSP_USART_TX_RCC    RCC_AHB1Periph_GPIOA
#define BSP_USART_RX_RCC    RCC_AHB1Periph_GPIOB

#define BSP_USART               USART1
#define BSP_USART_TX_PORT       GPIOA
#define BSP_USART_TX_PIN        GPIO_Pin_9
#define BSP_USART_RX_PORT       GPIOB
#define BSP_USART_RX_PIN        GPIO_Pin_7
#define BSP_USART_AF            GPIO_AF_USART1
#define BSP_USART_TX_AF_PIN     GPIO_PinSource9
#define BSP_USART_RX_AF_PIN     GPIO_PinSource7

extern char Serial_RxPacket[];
extern uint8_t Serial_RxFlag;

void Serial1_Init(uint32_t __Baud);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(char *String);
void Serial_SendNumber(uint32_t Number, uint8_t Length);
void Serial_Printf(char *format, ...);

#endif
