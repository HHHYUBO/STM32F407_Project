/**
 * �Զ����ɵ�����ʶ��ģ��Ȩ�� (����汾)
 * ������STM32F407
 */

#ifndef SPEECH_MODEL_H
#define SPEECH_MODEL_H

#include "arm_math.h"

// ģ�ͽṹ��Ϣ
#define MODEL_INPUT_HEIGHT 30
#define MODEL_INPUT_WIDTH 39
#define MODEL_INPUT_CHANNELS 1
#define MODEL_NUM_CLASSES 3

#define MODEL_FLATTENED_SIZE 1170

// ȫ���Ӳ� dense
#define FC0_INPUT_SIZE 1170
#define FC0_OUTPUT_SIZE 32

// BatchNormalization�� batch_normalization
#define BN0_SIZE 32

// ȫ���Ӳ� dense_1
#define FC1_INPUT_SIZE 32
#define FC1_OUTPUT_SIZE 16

// BatchNormalization�� batch_normalization_1
#define BN1_SIZE 16

// ȫ���Ӳ� dense_2
#define FC2_INPUT_SIZE 16
#define FC2_OUTPUT_SIZE 3

// ��ǩӳ��
extern const char* LABELS[];

// ȫ���Ӳ� dense - Ȩ�غ�ƫ��
extern const float32_t fc0_weights[];
extern const float32_t fc0_bias[];

// BatchNormalization�� batch_normalization - ����
extern const float32_t bn0_gamma[];
extern const float32_t bn0_beta[];
extern const float32_t bn0_moving_mean[];
extern const float32_t bn0_moving_variance[];

// ȫ���Ӳ� dense_1 - Ȩ�غ�ƫ��
extern const float32_t fc1_weights[];
extern const float32_t fc1_bias[];

// BatchNormalization�� batch_normalization_1 - ����
extern const float32_t bn1_gamma[];
extern const float32_t bn1_beta[];
extern const float32_t bn1_moving_mean[];
extern const float32_t bn1_moving_variance[];

// ȫ���Ӳ� dense_2 - Ȩ�غ�ƫ��
extern const float32_t fc2_weights[];
extern const float32_t fc2_bias[];

// ģ������������ - ʵ����ASR_CNN.c��
uint8_t ASR_CNN_Recognize(float32_t input[MODEL_INPUT_HEIGHT][MODEL_INPUT_WIDTH], uint8_t *result, float32_t *confidence);
const char* ASR_CNN_GetLabel(uint8_t result);
void ASR_CNN_Init(void);

#endif // SPEECH_MODEL_H
