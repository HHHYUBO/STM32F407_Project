#ifndef __ASR_H
#define __ASR_H

#include "board.h"

// 定义缓冲区大小
#define FRAME_SIZE 256
#define FRAME_OVERLAP 128
#define NUM_FRAMES ((ADC_BUFFER_SIZE - FRAME_SIZE) / (FRAME_SIZE - FRAME_OVERLAP) + 1)

// 定义MFCC相关常量
#define NFFT 512          // FFT点数
#define NUM_CEPS 13       // MFCC系数数量
#define NUM_FILTERS 26    // 梅尔滤波器数量
#define FEATURE_DIM 39    // 特征维度 (13 MFCC + 13 Delta + 13 Delta-Delta)

#define Train 1
#define Recognize 0

// 预处理后的数据缓冲区
extern float32_t processed_buffer[];
extern float32_t frame_buffer[];
extern float32_t window_buffer[];

// 特征缓冲区
extern float32_t energy_features[];
extern float32_t zcr_features[];
extern float32_t mfcc_features[][NUM_CEPS];
extern float32_t delta_features[][NUM_CEPS];
extern float32_t delta_delta_features[][NUM_CEPS];
extern float32_t combined_features[][FEATURE_DIM];

// 端点检测结果
extern uint16_t speech_start;
extern uint16_t speech_end;
extern uint8_t is_speech[];

// 定义模式
extern uint8_t ASR_Mode;

/******************************************************************************
 * Initialization Functions
 ******************************************************************************/
// 初始化ASR模块
void ASR_Init(void);


/******************************************************************************
 * Voice Signal Processing Functions
 ******************************************************************************/
// 信号预处理函数
void ASR_Preprocess(uint16_t* adc_data, uint16_t data_len);
// 端点检测（VAD）
void ASR_DetectEndpoints(void);


/******************************************************************************
 * MFCC Feature Extraction Functions
 ******************************************************************************/
// 提取MFCC特征（包含delta和delta-delta）
void ASR_ExtractMFCCWithDeltas(void);
// 计算delta特征
void ASR_ComputeDeltaFeatures(float32_t input[][NUM_CEPS], float32_t output[][NUM_CEPS], uint16_t num_frames);
// 对MFCC特征进行均值归一化
void ASR_NormalizeMFCC(float32_t mfcc[][NUM_CEPS], uint16_t num_frames);
// 合并MFCC、Delta和Delta-Delta特征
void ASR_CombineFeatures(void);
void ASR_ExtractFeaturesOnly(uint16_t* adc_data, uint16_t data_len);

/******************************************************************************
 * High-Level Speech Recognition Functions
 ******************************************************************************/
// 完整的语音识别流程函数 - 合并了特征提取和识别过程
uint8_t ASR_RecognizeSpeech(uint16_t* adc_data, uint16_t data_len, uint8_t* result, float32_t* confidence);
// 处理识别结果的函数
void ASR_HandleRecognitionResult(uint8_t result, float32_t confidence);
// 外部调用的包含ADC采集、预处理与识别的完整函数
uint8_t Recognition(void);
	
/******************************************************************************
 * Data Transmission Functions
 ******************************************************************************/
// 通过串口发送处理后的数据
void ASR_SendProcessedData(void);
// 发送CNN训练数据到PC
void ASR_SendCNNTrainingData(void);

#endif /* __ASR_H */
