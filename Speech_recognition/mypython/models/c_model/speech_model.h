/**
 * 自动生成的语音识别模型权重 (极简版本)
 * 适用于STM32F407
 */

#ifndef SPEECH_MODEL_H
#define SPEECH_MODEL_H

#include "arm_math.h"

// 模型结构信息
#define MODEL_INPUT_HEIGHT 30
#define MODEL_INPUT_WIDTH 39
#define MODEL_INPUT_CHANNELS 1
#define MODEL_NUM_CLASSES 3

#define MODEL_FLATTENED_SIZE 1170

// 全连接层 dense
#define FC0_INPUT_SIZE 1170
#define FC0_OUTPUT_SIZE 32

// BatchNormalization层 batch_normalization
#define BN0_SIZE 32

// 全连接层 dense_1
#define FC1_INPUT_SIZE 32
#define FC1_OUTPUT_SIZE 16

// BatchNormalization层 batch_normalization_1
#define BN1_SIZE 16

// 全连接层 dense_2
#define FC2_INPUT_SIZE 16
#define FC2_OUTPUT_SIZE 3

// 标签映射
extern const char* LABELS[];

// 全连接层 dense - 权重和偏置
extern const float32_t fc0_weights[];
extern const float32_t fc0_bias[];

// BatchNormalization层 batch_normalization - 参数
extern const float32_t bn0_gamma[];
extern const float32_t bn0_beta[];
extern const float32_t bn0_moving_mean[];
extern const float32_t bn0_moving_variance[];

// 全连接层 dense_1 - 权重和偏置
extern const float32_t fc1_weights[];
extern const float32_t fc1_bias[];

// BatchNormalization层 batch_normalization_1 - 参数
extern const float32_t bn1_gamma[];
extern const float32_t bn1_beta[];
extern const float32_t bn1_moving_mean[];
extern const float32_t bn1_moving_variance[];

// 全连接层 dense_2 - 权重和偏置
extern const float32_t fc2_weights[];
extern const float32_t fc2_bias[];

// 模型推理函数声明 - 实现在ASR_CNN.c中
uint8_t ASR_CNN_Recognize(float32_t input[MODEL_INPUT_HEIGHT][MODEL_INPUT_WIDTH], uint8_t *result, float32_t *confidence);
const char* ASR_CNN_GetLabel(uint8_t result);
void ASR_CNN_Init(void);

#endif // SPEECH_MODEL_H
