#include "Motor.h"
#include "ADC.h"  // 添加ADC头文件
#include "ASR.h"  // 添加ASR头文件

// 定义速度档位
#define SPEED_LEVEL_0 0    // 停止
#define SPEED_LEVEL_1 30   // 低速
#define SPEED_LEVEL_2 60   // 中速
#define SPEED_LEVEL_3 90   // 高速
#define temp_threshold 3000 // 温度阈值


// 当前速度档位
static uint8_t current_speed_level = 0;

// 当前控制模式
static Motor_ModeTypeDef current_mode = MOTOR_MODE_KEY;

void Motor_SetSpeed(uint16_t speed)
{
    // 使用TIM3的PWM占空比设置函数
    PWM_TIM3_SetDuty(speed);
}

/**
  * @brief  初始化电机控制引脚
  * @param  None
  * @retval None
  */
void Motor_Init(void)
{
    // 使用TIM3初始化PWM
    PWM_TIM3_Init();
    
    // 设置默认正转方向
    GPIO_SetBits(GPIOA, GPIO_Pin_4);    // 设置正转方向
    GPIO_ResetBits(GPIOA, GPIO_Pin_5);  // 确保反转方向为低
    
    // 设置初始电机速度为0
    Motor_SetSpeed(0);
}

/**
  * @brief  按键控制电机转速
  * @param  None
  * @retval None
  */
void Motor_KeyControl(void)
{
    if(GetKey1())  // 使用新的按键1检测函数
    {
        // 切换风扇档位
        current_speed_level = (current_speed_level + 1) % 4;  // 循环切换0,1,2,3
        
        // 根据档位设置电机速度
        switch(current_speed_level)
        {
            case 0:
                Motor_SetSpeed(SPEED_LEVEL_0);
                break;
            case 1:
                Motor_SetSpeed(SPEED_LEVEL_1);
                break;
            case 2:
                Motor_SetSpeed(SPEED_LEVEL_2);
                break;
            case 3:
                Motor_SetSpeed(SPEED_LEVEL_3);
                break;
            default:
                break;
        }
    }
}

/**
  * @brief  温度控制电机转速
  * @param  None
  * @retval None
  */
void Motor_TempControl(void)
{
    uint16_t current_temp = ADC_GetTemperature();  // 获取当前温度值
    
    if(current_temp > temp_threshold)
    {
        // 温度超过阈值，使用最高档
        current_speed_level = 3;
        Motor_SetSpeed(SPEED_LEVEL_3);
    }
    else
    {
        // 温度正常，使用最低档
        current_speed_level = 1;
        Motor_SetSpeed(SPEED_LEVEL_1);
    }
}

/**
  * @brief  语音控制电机转速
  * @param  None
  * @retval None
  */
void Motor_VoiceControl(void)
{
    uint8_t result = Recognition();  // 获取语音识别结果索引
    
    switch(result)
    {
        case 0:  // "add"命令
            if(current_speed_level < 3)  // 如果当前不是最高档
            {
                current_speed_level++;  // 提高一档
            }
            break;
            
        case 2:  // "sub"命令
            if(current_speed_level > 0)  // 如果当前不是最低档
            {
                current_speed_level--;  // 降低一档
            }
            break;
            
        case 1:  // "none"命令
        default:
            // 保持当前档位不变
            break;
    }
    
    // 根据当前档位设置电机速度
    switch(current_speed_level)
    {
        case 0:
            Motor_SetSpeed(SPEED_LEVEL_0);
            break;
        case 1:
            Motor_SetSpeed(SPEED_LEVEL_1);
            break;
        case 2:
            Motor_SetSpeed(SPEED_LEVEL_2);
            break;
        case 3:
            Motor_SetSpeed(SPEED_LEVEL_3);
            break;
        default:
            break;
    }
}

/**
  * @brief  电机控制模式切换
  * @param  None
  * @retval None
  */
void Motor_ModeSwitch(void)
{
    if(GetKey2())  // 使用新的按键2检测函数
    {
        // 循环切换模式
        current_mode = (Motor_ModeTypeDef)(((uint8_t)current_mode + 1) % 3);
        
        // 切换到新模式时，重置速度档位
        switch(current_mode)
        {
            case MOTOR_MODE_KEY:
                current_speed_level = 0;  // 按键模式从停止开始
                Motor_SetSpeed(SPEED_LEVEL_0);
                break;
                
            case MOTOR_MODE_TEMP:
                // 温度模式根据当前温度设置初始档位
                if(ADC_GetTemperature() > temp_threshold)
                {
                    current_speed_level = 3;
                    Motor_SetSpeed(SPEED_LEVEL_3);
                }
                else
                {
                    current_speed_level = 1;
                    Motor_SetSpeed(SPEED_LEVEL_1);
                }
                break;
                
            case MOTOR_MODE_VOICE:
                // 语音模式保持当前速度
                break;
                
            default:
                break;
        }
    }
}

/**
  * @brief  电机控制主函数
  * @param  None
  * @retval None
  */
void Motor_Control(void)
{
    // 根据当前模式调用相应的控制函数
    switch(current_mode)
    {
        case MOTOR_MODE_KEY:
            Motor_KeyControl();
            break;
            
        case MOTOR_MODE_TEMP:
            Motor_TempControl();
            break;
            
        case MOTOR_MODE_VOICE:
            Motor_VoiceControl();
            break;
            
        default:
            break;
    }
}

// 外部调用的获取当前速度挡位和控制模式函数
uint8_t Motor_GetSpeedLevel(void)
{
	return current_speed_level;
}

uint8_t Motor_GetControlMode(void)
{
	if(current_mode == MOTOR_MODE_KEY)
		return 0;
	else if(current_mode == MOTOR_MODE_TEMP)
		return 1;
	else if(current_mode == MOTOR_MODE_VOICE)
		return 2;
	else
		return 3;
}
