/**
 * 语音识别CNN模型推理头文件
 * 适用于STM32F407
 */
#ifndef ASR_CNN_H
#define ASR_CNN_H

#include "arm_math.h"
#include <stdint.h>
#include "speech_model.h"  // 添加这行以包含模型定义

/**
 * 初始化CNN模型
 */
void ASR_CNN_Init(void);

/**
 * 执行语音识别模型推理
 * @param features 输入特征，形状为 [MODEL_INPUT_HEIGHT][MODEL_INPUT_WIDTH]
 * @param result 识别结果类别索引
 * @param confidence 识别置信度
 * @return 1表示成功，0表示失败
 */
uint8_t ASR_CNN_Recognize(float32_t features[MODEL_INPUT_HEIGHT][MODEL_INPUT_WIDTH], 
                          uint8_t *result, float32_t *confidence);

/**
 * 获取识别结果对应的标签
 * @param result 识别结果类别索引
 * @return 标签字符串
 */
const char* ASR_CNN_GetLabel(uint8_t result);

#endif /* ASR_CNN_H */
