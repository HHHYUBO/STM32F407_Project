#include "ASR.h"
#include "mel_filters_sparse.h"  // 使用稀疏表示的梅尔滤波器系数


/******************************************************************************
 * Global Variables
 ******************************************************************************/
// 预处理后的数据缓冲区
float32_t processed_buffer[ADC_BUFFER_SIZE];
float32_t frame_buffer[FRAME_SIZE];
float32_t window_buffer[FRAME_SIZE];

// 特征缓冲区
float32_t energy_features[NUM_FRAMES];
float32_t zcr_features[NUM_FRAMES];

// MFCC特征缓冲区
float32_t mfcc_features[NUM_FRAMES][NUM_CEPS];
float32_t delta_features[NUM_FRAMES][NUM_CEPS];
float32_t delta_delta_features[NUM_FRAMES][NUM_CEPS];

// FFT及MFCC计算用的全局缓冲区
float32_t fft_input[NFFT];
float32_t fft_output[NFFT];
float32_t power_spectrum[NFFT/2 + 1];
float32_t mel_energies[NUM_FILTERS];

// FFT实例
arm_rfft_fast_instance_f32 fft_instance;

// 端点检测结果
uint16_t speech_start = 0;
uint16_t speech_end = 0;
uint8_t is_speech[NUM_FRAMES] = {0};

// 合并特征用的缓冲区
float32_t combined_features[NUM_FRAMES][39];  // 13 MFCC + 13 Delta + 13 Delta-Delta

/******************************************************************************
 * Initialization Functions
 ******************************************************************************/
// 初始化ASR模块
void ASR_Init(void)
{
    // 初始化汉明窗
    for (uint16_t i = 0; i < FRAME_SIZE; i++) {
        // 汉明窗公式: 0.54 - 0.46 * cos(2*pi*i/(N-1))
        window_buffer[i] = 0.54f - 0.46f * arm_cos_f32((2.0f * PI * i) / (FRAME_SIZE - 1));
    }
    
    // 初始化FFT实例
    arm_rfft_fast_init_f32(&fft_instance, NFFT);
    
    printf("ASR module initialized\r\n");
}

/******************************************************************************
 * Voice Signal Processing Functions
 ******************************************************************************/
// 信号预处理函数
void ASR_Preprocess(uint16_t* adc_data, uint16_t data_len)
{
    // 1. 转换为浮点数
    for (uint16_t i = 0; i < data_len; i++) {
        processed_buffer[i] = (float32_t)adc_data[i];
    }
    
    // 2. 预加重处理
    const float32_t alpha = 0.95f; // 预加重系数，通常取0.9~0.97
    float32_t prev_sample = processed_buffer[0];
    float32_t temp;
    
    for (uint16_t i = 1; i < data_len; i++) {
        temp = processed_buffer[i];
        processed_buffer[i] = processed_buffer[i] - alpha * prev_sample;
        prev_sample = temp;
    }
    
    printf("Pre-emphasis completed\r\n");
    
    // 3. 去直流偏置
    float32_t mean;
    arm_mean_f32(processed_buffer, data_len, &mean);
    printf("Signal mean value: %.4f\r\n", mean);
    
    for (uint16_t i = 0; i < data_len; i++) {
        processed_buffer[i] -= mean;
    }
    
    printf("Signal preprocessing completed\r\n");
    
    // 4. 分帧处理并提取基本特征
    printf("Starting frame processing, total frames: %d\r\n", NUM_FRAMES);
    
    for (uint16_t frame = 0; frame < NUM_FRAMES; frame++) {
        uint16_t frame_start = frame * (FRAME_SIZE - FRAME_OVERLAP);
        
        // 检查帧起始位置是否有效
        if (frame_start + FRAME_SIZE > data_len) {
            printf("Warning: Frame %d exceeds data range, skipping\r\n", frame);
            continue;
        }
        
        // 复制一帧数据
        arm_copy_f32(&processed_buffer[frame_start], frame_buffer, FRAME_SIZE);
        
        // 应用窗函数
        arm_mult_f32(frame_buffer, window_buffer, frame_buffer, FRAME_SIZE);
        
        // 计算能量特征
        float32_t energy = 0.0f;
        for (uint16_t i = 0; i < FRAME_SIZE; i++) {
            energy += frame_buffer[i] * frame_buffer[i];
        }
        energy_features[frame] = energy / FRAME_SIZE;
        
        // 计算过零率
        uint16_t zero_crossings = 0;
        for (uint16_t i = 1; i < FRAME_SIZE; i++) {
            if ((frame_buffer[i] * frame_buffer[i-1]) < 0) {
                zero_crossings++;
            }
        }
        zcr_features[frame] = (float32_t)zero_crossings / (FRAME_SIZE - 1);
    }
    
    printf("Frame processing and basic feature extraction completed\r\n");
}

// 端点检测（VAD）
void ASR_DetectEndpoints(void)
{
    printf("Starting endpoint detection...\r\n");
    
    // 计算能量的平均值和标准差，用于自适应阈值
    float32_t energy_sum = 0.0f;
    float32_t energy_max = 0.0f;
    
    for (uint16_t frame = 0; frame < NUM_FRAMES; frame++) {
        energy_sum += energy_features[frame];
        if (energy_features[frame] > energy_max) {
            energy_max = energy_features[frame];
        }
    }
    
    float32_t energy_avg = energy_sum / NUM_FRAMES;
    
    // 自适应阈值设置
    float32_t energy_threshold = energy_avg * 1.5f;  // 或者使用能量最大值的一定比例
    if (energy_threshold < 0.01f) energy_threshold = 0.01f;  // 设置最小阈值
    
    const float32_t zcr_threshold = 0.2f;      // 过零率阈值
    const uint16_t min_speech_frames = 5;      // 最小语音帧数
    
    // 清零端点检测结果数组
    memset(is_speech, 0, sizeof(is_speech));
    
    // 基于能量和过零率进行初步判断
    for (uint16_t frame = 0; frame < NUM_FRAMES; frame++) {
        if (energy_features[frame] > energy_threshold || 
            zcr_features[frame] > zcr_threshold) {
            is_speech[frame] = 1;
        }
    }
    
    // 平滑处理：去除孤立的语音/非语音帧
    for (uint16_t frame = 1; frame < NUM_FRAMES - 1; frame++) {
        if (is_speech[frame-1] == is_speech[frame+1] && 
            is_speech[frame] != is_speech[frame-1]) {
            is_speech[frame] = is_speech[frame-1];
        }
    }
    
    // 查找语音起始点
    uint16_t count = 0;
    for (uint16_t frame = 0; frame < NUM_FRAMES; frame++) {
        if (is_speech[frame]) {
            count++;
            if (count >= min_speech_frames) {
                speech_start = frame - count + 1;
                break;
            }
        } else {
            count = 0;
        }
    }
    
    // 查找语音结束点
    count = 0;
    for (uint16_t frame = NUM_FRAMES - 1; frame > speech_start; frame--) {
        if (is_speech[frame]) {
            count++;
            if (count >= min_speech_frames) {
                speech_end = frame + count - 1;
                if (speech_end >= NUM_FRAMES) speech_end = NUM_FRAMES - 1;
                break;
            }
        } else {
            count = 0;
        }
    }
    
    printf("Speech detected from frame %d to %d\r\n", speech_start, speech_end);
}

/******************************************************************************
 * MFCC Feature Extraction Functions
 ******************************************************************************/
// 计算delta特征
void ASR_ComputeDeltaFeatures(float32_t input[][NUM_CEPS], float32_t output[][NUM_CEPS], uint16_t num_frames)
{
    const uint16_t window = 2;  // 差分窗口大小
    
    // 计算差分
    for (uint16_t frame = 0; frame < num_frames; frame++) {
        for (uint16_t i = 0; i < NUM_CEPS; i++) {
            float32_t numerator = 0.0f;
            float32_t denominator = 0.0f;
            
            for (int16_t n = -window; n <= window; n++) {
                int16_t pos = frame + n;
                if (pos >= 0 && pos < num_frames) {
                    numerator += n * input[pos][i];
                    denominator += n * n;
                }
            }
            
            output[frame][i] = numerator / denominator;
        }
    }
}

// 对MFCC特征进行均值归一化
void ASR_NormalizeMFCC(float32_t mfcc[][NUM_CEPS], uint16_t num_frames)
{
    float32_t means[NUM_CEPS] = {0.0f};
    
    // 计算每个维度的均值
    for (uint16_t i = 0; i < NUM_CEPS; i++) {
        for (uint16_t frame = 0; frame < num_frames; frame++) {
            means[i] += mfcc[frame][i];
        }
        means[i] /= num_frames;
    }
    
    // 减去均值
    for (uint16_t frame = 0; frame < num_frames; frame++) {
        for (uint16_t i = 0; i < NUM_CEPS; i++) {
            mfcc[frame][i] -= means[i];
        }
    }
}

// 合并MFCC、Delta和Delta-Delta特征
// 添加特征加权和归一化
void ASR_CombineFeatures(void)
{
    // 权重系数
    const float32_t w_mfcc = 1.0f;
    const float32_t w_delta = 0.75f;
    const float32_t w_delta_delta = 0.5f;
    
    // 先对Delta和Delta-Delta特征进行归一化
    ASR_NormalizeMFCC(delta_features, NUM_FRAMES);
    ASR_NormalizeMFCC(delta_delta_features, NUM_FRAMES);
    
    for (uint16_t frame = 0; frame < NUM_FRAMES; frame++) {
        // 复制加权特征
        for (uint16_t i = 0; i < NUM_CEPS; i++) {
            combined_features[frame][i] = mfcc_features[frame][i] * w_mfcc;
            combined_features[frame][i + NUM_CEPS] = delta_features[frame][i] * w_delta;
            combined_features[frame][i + 2*NUM_CEPS] = delta_delta_features[frame][i] * w_delta_delta;
        }
    }
}

// 提取MFCC特征（包含delta和delta-delta）
void ASR_ExtractMFCCWithDeltas(void)
{
    printf("Starting MFCC feature extraction with deltas...\r\n");
    
    // 为每一帧计算MFCC
    for (uint16_t frame = 0; frame < NUM_FRAMES; frame++) {
        uint16_t frame_start = frame * (FRAME_SIZE - FRAME_OVERLAP);
        
        // 检查帧起始位置是否有效
        if (frame_start + FRAME_SIZE > ADC_BUFFER_SIZE) {
            printf("Warning: Frame %d exceeds data range, skipping MFCC\r\n", frame);
            continue;
        }
        
        // 1. 复制一帧数据并应用窗函数
        arm_copy_f32(&processed_buffer[frame_start], frame_buffer, FRAME_SIZE);
        arm_mult_f32(frame_buffer, window_buffer, frame_buffer, FRAME_SIZE);
        
        // 2. 准备FFT输入（补零至nfft点）
        for (uint16_t i = 0; i < FRAME_SIZE; i++) {
            fft_input[i] = frame_buffer[i];
        }
        for (uint16_t i = FRAME_SIZE; i < NFFT; i++) {
            fft_input[i] = 0.0f;
        }
        
        // 3. 执行FFT
        arm_rfft_fast_f32(&fft_instance, fft_input, fft_output, 0);
        
        // 4. 计算功率谱
        for (uint16_t i = 0; i <= NFFT/2; i++) {
            if (i == 0 || i == NFFT/2) {
                // DC和Nyquist分量是实数
                power_spectrum[i] = fft_output[2*i] * fft_output[2*i];
            } else {
                // 其他分量需要计算复数的模平方
                power_spectrum[i] = fft_output[2*i] * fft_output[2*i] + 
                                  fft_output[2*i+1] * fft_output[2*i+1];
            }
        }
        
        // 5. 应用预计算的梅尔滤波器组（使用稀疏表示）
        for (uint16_t m = 0; m < NUM_FILTERS; m++) {
            mel_energies[m] = 0.0f;
            
            // 使用稀疏表示的滤波器系数，只计算非零系数
            for (uint16_t i = 0; i < mel_nonzero_count[m]; i++) {
                uint16_t k = mel_indices[m][i];
                mel_energies[m] += mel_values[m][i] * power_spectrum[k];
            }
            
            // 6. 取对数
            if (mel_energies[m] < 1e-10f) mel_energies[m] = 1e-10f;  // 防止取对数时出现负无穷
            mel_energies[m] = log10f(mel_energies[m]);
        }
        
        // 7. 应用DCT（离散余弦变换）
        for (uint16_t i = 0; i < NUM_CEPS; i++) {
            mfcc_features[frame][i] = 0.0f;
            for (uint16_t j = 0; j < NUM_FILTERS; j++) {
                mfcc_features[frame][i] += mel_energies[j] * 
                                        arm_cos_f32(PI * i * (j + 0.5f) / NUM_FILTERS);
            }
            // 应用缩放因子
            mfcc_features[frame][i] *= sqrtf(2.0f / NUM_FILTERS);
        }
    }
    
    // 8. 对MFCC特征进行均值归一化 (CMN)
    ASR_NormalizeMFCC(mfcc_features, NUM_FRAMES);
    
    // 9. 计算delta特征
    ASR_ComputeDeltaFeatures(mfcc_features, delta_features, NUM_FRAMES);
    
    // 10. 计算delta-delta特征
    ASR_ComputeDeltaFeatures(delta_features, delta_delta_features, NUM_FRAMES);
    
    printf("MFCC feature extraction with deltas completed\r\n");
}

// 仅进行特征提取，不进行CNN识别
void ASR_ExtractFeaturesOnly(uint16_t* adc_data, uint16_t data_len)
{   
    // 1. 预处理
    ASR_Preprocess(adc_data, data_len);
    
    // 2. 端点检测
    ASR_DetectEndpoints();
    
    // 3. 提取MFCC特征
    ASR_ExtractMFCCWithDeltas();
    
    // 4. 合并特征
    ASR_CombineFeatures();   
}

/******************************************************************************
 * High-Level Speech Recognition Functions
 ******************************************************************************/
// 全局特征缓冲区，用于存储提取的特征
static float32_t features[MODEL_INPUT_HEIGHT][MODEL_INPUT_WIDTH];

// 完整的语音识别流程函数 - 合并了特征提取和识别过程
uint8_t ASR_RecognizeSpeech(uint16_t* adc_data, uint16_t data_len, uint8_t* result, float32_t* confidence)
{
    printf("Starting speech recognition process...\r\n");
    
    // 1. 预处理
    ASR_Preprocess(adc_data, data_len);
    
    // 2. 端点检测
    ASR_DetectEndpoints();
    
    // 3. 提取MFCC特征
    ASR_ExtractMFCCWithDeltas();
    
    // 4. 合并特征
    ASR_CombineFeatures();
    
    // 5. 准备特征数据 - 使用提取的MFCC特征
    // 将提取的特征复制到输入数组
    for (uint16_t h = 0; h < MODEL_INPUT_HEIGHT; h++) {
        for (uint16_t w = 0; w < MODEL_INPUT_WIDTH; w++) {
            if (h < NUM_FRAMES && w < FEATURE_DIM) {
                features[h][w] = combined_features[h][w];
            } else {
                features[h][w] = 0.0f;  // 填充未使用的部分
            }
        }
    }
    
    // 6. 执行CNN识别
    uint8_t success = ASR_CNN_Recognize(features, result, confidence);
    
    printf("Speech recognition process completed\r\n");
    return success;
}

// 处理识别结果的函数
void ASR_HandleRecognitionResult(uint8_t result, float32_t confidence)
{
    const char* label = ASR_CNN_GetLabel(result);
    printf("Result: %s, Confidence: %.2f%%\r\n", label, confidence * 100.0f);
    
    // 根据识别结果执行相应操作
    if (strcmp(label, "add") == 0) {
        printf("Executing addition operation\r\n");
				LED1_Turn();
    } else if (strcmp(label, "sub") == 0) {
        printf("Executing subtraction operation\r\n");
				LED2_Turn();
    }
    // 可以根据需要添加更多的命令处理
}


/******************************************************************************
 * Data Transmission Functions
 ******************************************************************************/
// 通过串口发送处理后的数据
void ASR_SendProcessedData(void)
{
    // 发送数据头标识
    printf("DATA_BEGIN\r\n");
    printf("TOTAL_SAMPLES:%d\r\n", ADC_BUFFER_SIZE);
    
    // 发送预处理后的语音数据 - 只发送部分样本以减少数据量
    printf("PROCESSED_SAMPLES_BEGIN\r\n");
    for (uint16_t i = 0; i < ADC_BUFFER_SIZE; i += 10) { // 每10个样本发送一个
        printf("SAMPLE %d %.4f\r\n", i, processed_buffer[i]);
    }
    printf("PROCESSED_SAMPLES_END\r\n");
    
    // 发送汉明窗数据
    printf("WINDOW_DATA_BEGIN\r\n");
    for (uint16_t i = 0; i < FRAME_SIZE; i += 4) { // 减少数据量
        printf("WINDOW %d %.6f\r\n", i, window_buffer[i]);
    }
    printf("WINDOW_DATA_END\r\n");
    
    // 选择关键帧发送数据
    uint16_t frames_to_send[] = {0, NUM_FRAMES/2, NUM_FRAMES-1};
    
    // 发送帧能量和过零率特征
    printf("BASIC_FEATURES_BEGIN\r\n");
    for (uint16_t f = 0; f < 3; f++) {
        uint16_t frame_idx = frames_to_send[f];
        if (frame_idx < NUM_FRAMES) {
            printf("FRAME_FEATURE:%d\r\n", frame_idx);
            printf("ENERGY %.6f\r\n", energy_features[frame_idx]);
            printf("ZCR %.6f\r\n", zcr_features[frame_idx]);
        }
    }
    printf("BASIC_FEATURES_END\r\n");
    
    // 发送MFCC特征
    printf("MFCC_FEATURES_BEGIN\r\n");
    for (uint16_t f = 0; f < 3; f++) {
        uint16_t frame_idx = frames_to_send[f];
        if (frame_idx < NUM_FRAMES) {
            printf("FRAME_MFCC:%d\r\n", frame_idx);
            
            // 静态MFCC
            printf("STATIC_MFCC_BEGIN\r\n");
            for (uint16_t i = 0; i < NUM_CEPS; i++) {
                printf("MFCC_%d %.6f\r\n", i, mfcc_features[frame_idx][i]);
            }
            printf("STATIC_MFCC_END\r\n");
            
            // Delta MFCC
            printf("DELTA_MFCC_BEGIN\r\n");
            for (uint16_t i = 0; i < NUM_CEPS; i++) {
                printf("DELTA_%d %.6f\r\n", i, delta_features[frame_idx][i]);
            }
            printf("DELTA_MFCC_END\r\n");
            
            // Delta-Delta MFCC
            printf("DELTA_DELTA_MFCC_BEGIN\r\n");
            for (uint16_t i = 0; i < NUM_CEPS; i++) {
                printf("DELTA_DELTA_%d %.6f\r\n", i, delta_delta_features[frame_idx][i]);
            }
            printf("DELTA_DELTA_MFCC_END\r\n");
        }
    }
    printf("MFCC_FEATURES_END\r\n");
    
    // 发送端点检测结果
    printf("VAD_RESULT_BEGIN\r\n");
    printf("SPEECH_START:%d\r\n", speech_start);
    printf("SPEECH_END:%d\r\n", speech_end);
    printf("VAD_RESULT_END\r\n");
    
    // 发送数据尾标识
    printf("DATA_END\r\n");
}

// 发送CNN训练数据到PC
void ASR_SendCNNTrainingData(void)
{
    // 确保已经提取了特征
    if (speech_start == 0 && speech_end == 0) {
        printf("Error: No speech detected for CNN training data\r\n");
        return;
    }
    
    // 合并特征
    ASR_CombineFeatures();
    
    // 发送数据头标识
    printf("CNN_DATA_BEGIN\r\n");
    
    // 发送语音段信息
    printf("SPEECH_START:%d\r\n", speech_start);
    printf("SPEECH_END:%d\r\n", speech_end);
    uint16_t valid_frames = speech_end - speech_start + 1;
    printf("VALID_FRAMES:%d\r\n", valid_frames);
    printf("FEATURE_DIM:%d\r\n", FEATURE_DIM);
    
    // 发送特征数据
    printf("FEATURE_DATA_BEGIN\r\n");
    for (uint16_t frame = speech_start; frame <= speech_end; frame++) {
        printf("FRAME:%d\r\n", frame - speech_start);
        for (uint16_t i = 0; i < FEATURE_DIM; i++) {
            printf("%.6f", combined_features[frame][i]);
            if (i < FEATURE_DIM - 1) {
                printf(",");
            }
        }
        printf("\r\n");
    }
    printf("FEATURE_DATA_END\r\n");
    
    // 添加波形数据发送部分
    printf("RAW_WAVEFORM_BEGIN\r\n");
    uint16_t start_sample = speech_start * (FRAME_SIZE - FRAME_OVERLAP);
    uint16_t end_sample = (speech_end + 1) * (FRAME_SIZE - FRAME_OVERLAP) + FRAME_OVERLAP;
    if (end_sample > ADC_BUFFER_SIZE) end_sample = ADC_BUFFER_SIZE;
    
    printf("SAMPLES:%d\r\n", end_sample - start_sample);
    for (uint16_t i = start_sample; i < end_sample; i++) {
        printf("%.4f", processed_buffer[i]);
        if (i < end_sample - 1) {
            printf(",");
        }
    }
    printf("\r\n");
    printf("RAW_WAVEFORM_END\r\n");
    
    // 发送数据尾标识
    printf("CNN_DATA_END\r\n");
    
    printf("CNN training data sent successfully\r\n");
}









