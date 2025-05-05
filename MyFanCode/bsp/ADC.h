#ifndef __ADC_H
#define __ADC_H

#include "board.h"
extern volatile uint8_t adc_conversion_complete;
extern uint16_t adc_buffer[];

#define ADC_BUFFER_SIZE 8000

void ADC_DMA_Init(void);
void ADC_Start(void);
void Check_And_Print_ADC(void);

void TemAndVol_ADC_Config(void);
uint16_t ADC_GetTemperature(void);
uint16_t ADC_GetVoltage(void);

#endif
