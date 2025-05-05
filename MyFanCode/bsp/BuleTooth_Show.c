#include "BuleTooth_Show.h"

// 电池相关参数
#define BATTERY_FULL_VOLTAGE    12.6f    // 满电电压 12.6V
#define BATTERY_CUTOFF_VOLTAGE  10.0f    // 放电截止电压 10.0V
#define VOLTAGE_DIVIDER_RATIO   4.0f     // 分压比 1/4
#define ADC_REFERENCE_VOLTAGE   3.3f     // ADC参考电压 3.3V
#define ADC_RESOLUTION         4096.0f   // 12位ADC分辨率

// 温度传感器相关参数
#define NTC_B_VALUE            3950.0f   // NTC热敏电阻B值
#define NTC_R_REF              10000.0f  // NTC参考电阻值(10k)
#define NTC_T_REF              298.15f   // 参考温度(25℃ = 298.15K)
#define NTC_R_SERIES           10000.0f  // 串联电阻值(10k)
#define VCC                    3.3f      // 电源电压

/**
  * @brief  计算电池实际电压
  * @param  adc_value: ADC采样值
  * @retval 实际电压值（单位：V）
  */
float Battery_GetVoltage(uint16_t adc_value)
{
    // 1. 将ADC值转换为电压（考虑分压比）
    float adc_voltage = (adc_value / ADC_RESOLUTION) * ADC_REFERENCE_VOLTAGE;
    
    // 2. 计算实际电池电压
    float battery_voltage = adc_voltage * VOLTAGE_DIVIDER_RATIO;
    
    return battery_voltage;
}

/**
  * @brief  计算电池电量百分比
  * @param  adc_value: ADC采样值
  * @retval 电量百分比（0-100）
  */
uint8_t Battery_GetPercentage(uint16_t adc_value)
{
    // 1. 获取实际电压
    float battery_voltage = Battery_GetVoltage(adc_value);
    
    // 2. 计算电量百分比（线性映射）
    float percentage = 100.0f * (battery_voltage - BATTERY_CUTOFF_VOLTAGE) / 
                      (BATTERY_FULL_VOLTAGE - BATTERY_CUTOFF_VOLTAGE);
    
    // 3. 限制在0-100范围内
    if(percentage > 100.0f)
        percentage = 100.0f;
    else if(percentage < 0.0f)
        percentage = 0.0f;
    
    return (uint8_t)percentage;
}

/**
  * @brief  计算NTC热敏电阻的阻值
  * @param  adc_value: ADC采样值
  * @retval NTC热敏电阻的阻值（单位：Ω）
  */
float NTC_GetResistance(uint16_t adc_value)
{
    // 1. 将ADC值转换为电压
    float v_ntc = (adc_value / ADC_RESOLUTION) * VCC;
    
    // 2. 计算NTC阻值（分压电路公式）
    float r_ntc = (NTC_R_SERIES * v_ntc) / (VCC - v_ntc);
    
    return r_ntc;
}

/**
  * @brief  计算实际温度值
  * @param  adc_value: ADC采样值
  * @retval 温度值（单位：℃）
  */
float NTC_GetTemperature(uint16_t adc_value)
{
    // 1. 获取NTC阻值
    float r_ntc = NTC_GetResistance(adc_value);
    
    // 2. 使用Steinhart-Hart方程计算温度
    float steinhart = logf(r_ntc / NTC_R_REF) / NTC_B_VALUE;
    steinhart += 1.0f / NTC_T_REF;
    steinhart = 1.0f / steinhart;
    
    // 3. 转换为摄氏度
    float temperature = steinhart - 273.15f;
    
    return temperature;
}

/**
  * @brief  通过蓝牙发送系统参数
  * @param  None
  * @retval None
  */
void Bluetooth_SendParameters(void)
{
    // 获取电池信息
    uint16_t battery_adc = ADC_GetVoltage();  // 假设这是电池电压的ADC值
    float battery_voltage = Battery_GetVoltage(battery_adc);
    uint8_t battery_percentage = Battery_GetPercentage(battery_adc);
    
    // 获取温度信息
    uint16_t temp_adc = ADC_GetTemperature();  // 假设这是温度传感器的ADC值
    float temperature = NTC_GetTemperature(temp_adc);
    
    // 获取风扇挡位信息
    uint8_t fan_speed_level = Motor_GetSpeedLevel();  // 使用Motor.c中的函数获取当前挡位
    
    // 获取控制模式
    uint8_t control_mode = Motor_GetControlMode();  // 使用Motor.c中的函数获取当前控制模式
    
    // 发送电池信息
    printf("Battery: %.1fV (%d%%)\r\n", battery_voltage, battery_percentage);
    
    // 发送温度信息
    printf("Temperature: %.1f℃\r\n", temperature);
    
    // 发送风扇挡位信息
    printf("Fan Speed Level: %d\r\n", fan_speed_level);
    
    // 发送控制模式信息
    switch(control_mode) {
        case 0:
            printf("Control Mode: Button\r\n");
            break;
        case 1:
            printf("Control Mode: Temperature\r\n");
            break;
        case 2:
            printf("Control Mode: Voice\r\n");
            break;
        default:
            printf("Control Mode: Unknown\r\n");
            break;
    }
}
