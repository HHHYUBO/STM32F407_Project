#ifndef __ADC_H
#define __ADC_H

#include "board.h"
extern volatile uint8_t adc_conversion_complete;

void ADC_DMA_Init(void);
void ADC_Start(void);
void Check_And_Print_ADC(void);

#endif
