/**
 * 语音识别CNN模型推理实现
 * 适用于STM32F407
 * 使用CMSIS-DSP库优化性能
 */
#include "ASR_CNN.h"
#include "ASR.h"
#include "speech_model.h"  // 包含生成的模型权重
#include <string.h>
#include <stdio.h>
#include <float.h>  // 为FLT_MAX定义
#include "arm_math.h"

// 中间缓冲区
static float32_t flattened_input[MODEL_FLATTENED_SIZE];
static float32_t fc0_output[FC0_OUTPUT_SIZE];
static float32_t fc1_output[FC1_OUTPUT_SIZE]; // 新增：第二个全连接层的输出缓冲区
static float32_t output[MODEL_NUM_CLASSES];

// 辅助函数：ReLU激活函数
static void apply_relu(float32_t* data, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        if (data[i] < 0.0f) {
            data[i] = 0.0f;
        }
    }
}

// 辅助函数：Softmax激活函数 - 使用CMSIS-DSP优化
static void softmax(float32_t* input, float32_t* output, uint16_t length) {
    float32_t max_val = input[0];
    float32_t sum = 0.0f;
    
    // 找出最大值（用于数值稳定性）
    arm_max_f32(input, length, &max_val, NULL);
    
    // 计算指数和总和
    for (uint16_t i = 0; i < length; i++) {
        output[i] = expf(input[i] - max_val);
        sum += output[i];
    }
    
    // 归一化
    float32_t inv_sum = 1.0f / sum;
    arm_scale_f32(output, inv_sum, output, length);
}

// 辅助函数：全连接层操作 - 使用CMSIS-DSP优化
static void dense_layer(float32_t* input, const float32_t* weights, const float32_t* bias,
                      uint16_t input_size, uint16_t output_size, float32_t* output,
                      uint8_t apply_relu_flag) {
    
    // 创建矩阵实例
    arm_matrix_instance_f32 input_matrix;
    arm_matrix_instance_f32 weights_matrix;
    arm_matrix_instance_f32 output_matrix;
    
    // 初始化矩阵
    arm_mat_init_f32(&input_matrix, 1, input_size, input);
    arm_mat_init_f32(&weights_matrix, input_size, output_size, (float32_t*)weights);
    arm_mat_init_f32(&output_matrix, 1, output_size, output);
    
    // 执行矩阵乘法: output = input * weights
    arm_mat_mult_f32(&input_matrix, &weights_matrix, &output_matrix);
    
    // 添加偏置
    for (uint16_t i = 0; i < output_size; i++) {
        output[i] += bias[i];
    }
    
    // 应用ReLU激活函数
    if (apply_relu_flag) {
        apply_relu(output, output_size);
    }
}

// 辅助函数：BatchNormalization操作
static void batch_normalization(float32_t* data, uint16_t size, 
                               const float32_t* gamma, const float32_t* beta, 
                               const float32_t* moving_mean, const float32_t* moving_variance,
                               float32_t epsilon) {
    for (uint16_t i = 0; i < size; i++) {
        // 标准化
        data[i] = (data[i] - moving_mean[i]) / sqrtf(moving_variance[i] + epsilon);
        // 缩放和平移
        data[i] = gamma[i] * data[i] + beta[i];
    }
}

// 获取识别结果对应的标签
const char* ASR_CNN_GetLabel(uint8_t result) {
    if (result < MODEL_NUM_CLASSES) {
        return LABELS[result];
    }
    return "unknown";
}

// 使用CNN模型进行语音识别
uint8_t ASR_CNN_Recognize(float32_t features[MODEL_INPUT_HEIGHT][MODEL_INPUT_WIDTH], 
                          uint8_t *result, float32_t *confidence) {
    
//    printf("Starting simplified model inference...\r\n");
    
    // 1. 展平输入数据
//    printf("Flattening input...\r\n");
    uint32_t idx = 0;
    for (uint16_t h = 0; h < MODEL_INPUT_HEIGHT; h++) {
        for (uint16_t w = 0; w < MODEL_INPUT_WIDTH; w++) {
            flattened_input[idx++] = features[h][w];
        }
    }
    
    // 2. 第一个全连接层
//    printf("FC1 layer...\r\n");
    dense_layer(flattened_input, fc0_weights, fc0_bias,
              FC0_INPUT_SIZE, FC0_OUTPUT_SIZE, fc0_output, 1);
    
    // 3. 第一个BatchNormalization层
//    printf("BatchNorm1 layer...\r\n");
    batch_normalization(fc0_output, FC0_OUTPUT_SIZE, 
                       bn0_gamma, bn0_beta, bn0_moving_mean, bn0_moving_variance, 0.001f);
    
    // 4. 第二个全连接层
//    printf("FC2 layer...\r\n");
    dense_layer(fc0_output, fc1_weights, fc1_bias,
              FC1_INPUT_SIZE, FC1_OUTPUT_SIZE, fc1_output, 1);
    
    // 5. 第二个BatchNormalization层
//    printf("BatchNorm2 layer...\r\n");
    batch_normalization(fc1_output, FC1_OUTPUT_SIZE, 
                       bn1_gamma, bn1_beta, bn1_moving_mean, bn1_moving_variance, 0.001f);
    
    // 6. 输出层（第三个全连接层）
//    printf("Output layer...\r\n");
    dense_layer(fc1_output, fc2_weights, fc2_bias,
              FC2_INPUT_SIZE, FC2_OUTPUT_SIZE, output, 0);
    
    // 7. 应用Softmax获取概率
//    printf("Softmax layer...\r\n");
    softmax(output, output, MODEL_NUM_CLASSES);
    
    // 8. 找出最大概率的类别
    float32_t max_prob;
    uint32_t max_idx;
    arm_max_f32(output, MODEL_NUM_CLASSES, &max_prob, &max_idx);
    
    // 设置结果
    *result = (uint8_t)max_idx;
    *confidence = max_prob;
    
//    printf("Model inference completed\r\n");
    
    // 如果置信度太低，认为识别失败
    if (max_prob < 0.6f) {
        return 0;
    }
    
    return 1;
}

// 初始化CNN模型
void ASR_CNN_Init(void) {   
    // 初始化中间缓冲区
    memset(flattened_input, 0, sizeof(flattened_input));
    memset(fc0_output, 0, sizeof(fc0_output));
    memset(output, 0, sizeof(output));
}
							
