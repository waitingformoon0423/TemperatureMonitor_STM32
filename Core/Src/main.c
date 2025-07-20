/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 
  * @attention      : 2025 STM32温度监测系统--嘉庚学院实践周课设
  *
  * Copyright (c) 
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "delay_us.h"    // 微秒级延时函数
#include "ds1302.h"      // DS1302实时时钟驱动
#include "oled.h"        // OLED显示屏驱动
#include "ds18b20.h"     // DS18B20温度传感器驱动
#include "hal_iic.h"     // I2C通信协议驱动
#include "string.h"      // 字符串操作库
#include "stdio.h"       // 标准输入输出库
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define WEEK_DAY 7  // 一周天数常量（未使用，保留）
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
float temperature;                  // DS18B20传感器实时读取的温度值（单位：°C）
char strff[32];                     // OLED显示用的温度字符串缓冲区（格式：":X.XC"）
uint8_t PageOffset, ColOffset, CharNum;  // OLED显示坐标偏移量（Page: 页地址，Col: 列地址）
uint8_t mode = 0;                   // 界面模式（0:实时数据 1:历史记录 2:蓝牙设置 3:参数设置）
int Head, Point;                    // 温度值的整数部分和小数部分（用于字符串格式化）
int refreshing = 0;                 // 界面刷新标志（1:需要清屏重绘）
uint8_t start[] = {00, 42, 16, 16, 07, 05, 25};  // DS1302初始化时间数组（格式：秒,分,时,日,月,周,年）
int runningFun = 0;                 // 设备运行状态（0:关闭 1:开启）
int quitAlarming = 0;               // 报警状态标志（0:未报警 1:报警中 2:手动停止）
/*
uint8_t RxFlag = 0;                 // UART接收标志（未使用，保留）
uint8_t RxBuf;                      // UART接收缓存（未使用，保留）
*/
uint8_t rf = 0;                     // 蓝牙数据接收完成标志（1:接收到新数据）
uint8_t str[256];                   // 蓝牙接收数据缓冲区（存储蓝牙指令）

// 当天温度记录结构体（仅保留单日数据）
typedef struct {
    uint8_t day;                  // 当前日期（1-31）
    float max_temperature;        // 当日最高温度（°C）
    float min_temperature;        // 当日最低温度（°C）
    uint8_t max_hour;             // 最高温度发生小时（0-23）
    uint8_t max_minute;           // 最高温度发生分钟（0-59）
    uint8_t max_second;           // 最高温度发生秒（0-59）
    uint8_t min_hour;             // 最低温度发生小时（0-23）
    uint8_t min_minute;           // 最低温度发生分钟（0-59）
    uint8_t min_second;           // 最低温度发生秒（0-59）
} DailyTempRecord;
DailyTempRecord todayTemp = {0};    // 当日温度记录实例（初始化为0）

// 蓝牙设置的监测时间段结构体
typedef struct {
    uint8_t start_hour;           // 监测开始小时（0-23）
    uint8_t start_minute;         // 监测开始分钟（0-59）
    uint8_t start_second;         // 监测开始秒（0-59）
    uint8_t end_hour;             // 监测结束小时（0-23）
    uint8_t end_minute;           // 监测结束分钟（0-59）
    uint8_t end_second;           // 监测结束秒（0-59）
} BlueToothSettings;
BlueToothSettings blueToothSettings = {0};  // 监测时间段设置实例（初始化为0）

float temperatureThreshold = 30.0f; // 温度报警阈值（超过此值触发报警，单位：°C）
char btResponse[32];                // 蓝牙响应数据缓冲区（返回设置结果）
uint8_t isMonitoring = 1;           // 监测状态标志（1:处于监测时段 0:不在监测时段）
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// 简化函数声明（补充功能说明）
int8_t IsNewDay(void);              // 判断是否进入新的一天（基于DS1302日期）
void UpdateData(float temperature); // 更新当天最高/最低温度及发生时间
void Show_Current(void);            // 显示实时数据界面（温度、日期、时间）
void Show_History(void);            // 显示历史记录界面（当日最高/最低温度）
void temperature_monitor(void);     // 核心温度监测逻辑（控制设备及报警）
void Show_BlueTooth(void);          // 显示蓝牙设置界面（接收并处理蓝牙指令）
int8_t Bluetooth(uint8_t *dat);     // 解析蓝牙指令（设置阈值或时间段）
int8_t is_time_in_range(void);      // 检查当前时间是否在设定的监测时段内
void Show_Setting(void);            // 显示参数设置界面（展示监测时段和阈值）
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  主函数 - 系统入口点
  * @details 系统初始化流程：
  *          1. HAL库初始化
  *          2. 系统时钟配置
  *          3. 外设初始化（GPIO、定时器、串口）
  *          4. OLED显示屏初始化
  *          5. DS1302实时时钟初始化
  *          6. 启动串口中断接收
  * @note 主循环每200ms执行一次，包含：
  *       - 温度数据采集
  *       - OLED界面刷新
  *       - 设备状态控制
  *       - 蓝牙通信处理
  * @retval int 程序返回状态
  */
int main(void)
{
    /* USER CODE BEGIN 1 */
  
    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_TIM3_Init();
    MX_TIM1_Init();
    MX_USART2_UART_Init();

    /* USER CODE BEGIN 2 */
    OLED_Init();
    ds1302_init(start);
    HAL_UART_Receive_IT(&huart2, (uint8_t *)&str, 8);

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        // 读取DS18B20温度
        temperature = DS18B20_GetTemp_SkipRom();
        UpdateData(temperature);
        Head = (int)temperature;
        Point = (int)((temperature - Head) * 10.0);
        sprintf(strff, ":%d.%dC", Head, Point);  // 显示温度
        // 屏幕显示模式

        switch (mode)
        {
            case 0:
                // 默认 展示 当前时间 和 温度 显示 温度监测系统
                // refreshing 用来避免每秒刷新导致闪屏
                if (refreshing)
                    OLED_Clear_Screen();
                refreshing = 0;
                Show_Current();
                break;
            case 1:
                // 展示 当日历史最高温度 和最低温度
                if (refreshing)
                    OLED_Clear_Screen();
                refreshing = 0;
                Show_History();
                break;
            case 2:
                // 蓝牙模式 调试
                if (refreshing)
                    OLED_Clear_Screen();
                refreshing = 0;
                Show_BlueTooth();
                break;
            case 3:
                // 展示 参数设置
                // 监测时间段 和 温度阈值
                if (refreshing)
                    OLED_Clear_Screen();
                refreshing = 0;
                Show_Setting();
                break;
            default:
                break;
        }
        // 温度监测 风扇 工作 和 蜂鸣器报警逻辑
        temperature_monitor();
        // 阻塞型延时 如之后需要添加短时即时性项目

        HAL_Delay(200);

        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */
/**
  * @brief  外部中断回调函数，处理按键事件。
  * @param  GPIO_Pin: 触发中断的 GPIO 引脚（KEY0/KEY1）。
  * @note   KEY0: 短按停止当前报警（仅在报警中有效）
  *         KEY1: 短按切换界面模式（0→1→2→3→0循环）
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == KEY0_Pin)
    {
        HAL_Delay(50);
        if (quitAlarming == 1)
        {
            quitAlarming++;
            HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
        }
    }

    if (GPIO_Pin == KEY1_Pin)
    {
        HAL_Delay(50);
        mode = (mode + 1) % 4; // 只切换模式
        refreshing = 1;
    }
}

/**
  * @brief  更新当天的温度记录（最高/最低温度及发生时间）。
  * @param  temperature: 当前读取到的温度值（°C）。
  * @note   每日0点自动重置记录，仅保留当日数据。
  */
void UpdateData(float temperature)
{
    if (IsNewDay())
    {
        // 新的一天重置记录
        todayTemp.day = TimeSDA.day;
        todayTemp.max_temperature = -100;
        todayTemp.min_temperature = 100;
        todayTemp.max_hour = TimeSDA.hour;
        todayTemp.max_minute = TimeSDA.minute;
        todayTemp.max_second = TimeSDA.second;
        todayTemp.min_hour = TimeSDA.hour;
        todayTemp.min_minute = TimeSDA.minute;
        todayTemp.min_second = TimeSDA.second;
    }
    else
    {
        // 更新当天记录
        if (temperature > todayTemp.max_temperature) {
            todayTemp.max_temperature = temperature;
            // 记录当前时间
            ds1302_read_realTime(); // 确保读取最新时间
            todayTemp.max_hour = TimeSDA.hour;
            todayTemp.max_minute = TimeSDA.minute;
            todayTemp.max_second = TimeSDA.second;
        }
        else if (temperature < todayTemp.min_temperature) {
            todayTemp.min_temperature = temperature;
            todayTemp.min_hour = TimeSDA.hour;
            todayTemp.min_minute = TimeSDA.minute;
            todayTemp.min_second = TimeSDA.second;
        }
    }
}

/**
  * @brief  判断是否为新的一天（基于DS1302实时时钟的日期）。
  * @retval 1: 是新的一天；0: 不是新的一天。
  */
int8_t IsNewDay()
{
    static uint8_t last_day = 0;
    ds1302_read_realTime();
    uint8_t current_day = TimeSDA.day;
    if (current_day != last_day)
    {
        last_day = current_day;
        return 1; 
    }
    return 0; 
}

/**
  * @brief  显示当前实时信息，包括温度、日期和时间。
  * @retval None
  */
void Show_Current()
{
    PageOffset = 0, ColOffset = 0;
    // 第一行显示温度监测系统
    for (CharNum = 0; CharNum < 6; CharNum++)   
    {
        OLED_Display_Char_16X16(PageOffset, ColOffset, CharNum);
        ColOffset += 16;
    }
  
    ds1302_read_realTime();
    // 打印年月日和当前时间
    OLED_Display_String_8X16(2, 0, (uint8_t *)DS1302_data_1);
    OLED_Display_String_8X16(4, 0, (uint8_t *)DS1302_data_2);
  
    // 打印温度
    // 显示中文温度两个字
    PageOffset = 6, ColOffset = 0;
    for (CharNum = 0; CharNum < 2; CharNum++)   
    {
        OLED_Display_Char_16X16(PageOffset, ColOffset, CharNum);
        ColOffset += 16;
    }
    OLED_Display_String_8X16(6, 32, (uint8_t *)strff);
}

/**
  * @brief  显示当天的温度历史记录，包括最高和最低温度及其时间。
  * @retval None
  */
void Show_History()
{
    char temp_str[32];
  
    // 显示当天记录
    sprintf(temp_str, "Today:%d", todayTemp.day );
    OLED_Display_String_8X16(0, 0, (uint8_t *)temp_str);
  
    // 最高温度显示
    sprintf(temp_str, "H:%.1fC %02d:%02d", 
            todayTemp.max_temperature,
            todayTemp.max_hour,
            todayTemp.max_minute);
    OLED_Display_String_8X16(2, 0, (uint8_t *)temp_str);
  
    // 最低温度显示
    sprintf(temp_str, "L:%.1fC %02d:%02d",
            todayTemp.min_temperature,
            todayTemp.min_hour,
            todayTemp.min_minute);
    OLED_Display_String_8X16(4, 0, (uint8_t *)temp_str);
  
    PageOffset = 6, ColOffset = 0;
    // 打印历史记录显示
    for (CharNum = 6; CharNum < 12; CharNum++)
    {
        OLED_Display_Char_16X16(PageOffset, ColOffset, CharNum);
        ColOffset += 16;
    }
}

/**
  * @brief  处理蓝牙指令，设置温度阈值和时段。
  * @param  dat: 接收到的蓝牙数据。
  * @retval 1: 指令处理成功；0: 指令无效。
  */
int8_t Bluetooth(uint8_t *dat) {
    char cmd[32] = {0}; // 初始化缓冲区为 0
    float temp;
    uint8_t h1, m1, h2, m2;
    // 打印接收到的数据用于调试
    char debug_msg[100];
    sprintf(debug_msg, "Received data: %.*s\n", 8, dat); // 只打印前 8 个字符用于调试
    HAL_UART_Transmit(&huart2, (uint8_t *)debug_msg, strlen(debug_msg), 100);

    // 手动添加字符串结束符
    for (int i = 0; i < 8; i++) {
        if (dat[i] == '\0') break;
        cmd[i] = dat[i];
    }
    cmd[8] = '\0'; // 确保字符串以 '\0' 结尾

    // 温度阈值设置指令
    if (strncmp(cmd, "temp", 4) == 0) {
        if (sscanf(cmd + 4, "%f", &temp) == 1) {
            temperatureThreshold = temp;
            sprintf(btResponse, "TEMP SET:%.1f", temperatureThreshold);
            HAL_UART_Transmit(&huart2, (uint8_t *)btResponse, strlen(btResponse), 100);
            OLED_Display_String_8X16(4, 56, (uint8_t *)"OK");
            quitAlarming=0;
            HAL_GPIO_WritePin(BEEP_GPIO_Port,BEEP_Pin, GPIO_PIN_RESET);
            return 1;
        }
    }
    // 时段设置指令 格式: 09301830
    else if (strlen(cmd) == 8) {
        if (sscanf(cmd, "%02hhu%02hhu%02hhu%02hhu", &h1, &m1, &h2, &m2) == 4) {
            if (h1 < 24 && m1 < 60 && h2 < 24 && m2 < 60) {
                blueToothSettings.start_hour = h1;
                blueToothSettings.start_minute = m1;
                blueToothSettings.start_second = 0;
                blueToothSettings.end_hour = h2;
                blueToothSettings.end_minute = m2;
                blueToothSettings.end_second = 0;

                sprintf(btResponse, "TIME SET:%02hhu:%02hhu:00-%02hhu:%02hhu:00", 
                       h1, m1, h2, m2);
                HAL_UART_Transmit(&huart2, (uint8_t *)btResponse, strlen(btResponse), 100);
                OLED_Display_String_8X16(4, 56, (uint8_t *)"OK");
                return 1;
            }
        }
    }

    strcpy(btResponse, "ERR:INVALID CMD");
    HAL_UART_Transmit(&huart2, (uint8_t *)btResponse, strlen(btResponse), 100);
    OLED_Display_String_8X16(4, 56, (uint8_t *)"ERR");
    return 0;
}

/**
  * @brief  检查当前时间是否在设定的监测时间段内。
  * @retval 1: 在时间段内；0: 不在时间段内。
  */
int8_t is_time_in_range() {
    ds1302_read_realTime();
    uint32_t current_sec = TimeSDA.hour * 3600 + TimeSDA.minute * 60 + TimeSDA.second;
    uint32_t start_sec = blueToothSettings.start_hour * 3600 + 
                        blueToothSettings.start_minute * 60 + 
                        blueToothSettings.start_second;
    uint32_t end_sec = blueToothSettings.end_hour * 3600 + 
                      blueToothSettings.end_minute * 60 + 
                      blueToothSettings.end_second;

    // 处理跨天情况
    if (end_sec < start_sec) {
        return (current_sec >= start_sec) || (current_sec <= end_sec);
    }
    return (current_sec >= start_sec) && (current_sec <= end_sec);
}

/**
  * @brief  温度监测函数 - 实时控制风扇和蜂鸣器
  * @details 根据当前温度与阈值的关系，智能控制风扇和蜂鸣器
  *          - 温度 < 阈值-1°C：关闭风扇和蜂鸣器
  *          - 阈值-1°C ≤ 温度 < 阈值：启动风扇，关闭蜂鸣器
  *          - 温度 ≥ 阈值：启动风扇和蜂鸣器（可手动关闭）
  * @note 使用状态机确保设备状态变化时立即响应，避免延迟
  * @retval None
  */
void temperature_monitor()
{
    // 检查时间段
    if (blueToothSettings.start_hour != 0 || blueToothSettings.end_hour != 0) {
        isMonitoring = is_time_in_range();
    }
    
    if (!isMonitoring) {
        // 不在监测时段时关闭所有设备
        HAL_GPIO_WritePin(IN_GPIO_Port, IN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
        runningFun = 0;
        quitAlarming = 0;
        return;
    }

    // 温度低于阈值时，强制停止报警（新增逻辑）和解除被锁的蜂鸣器
    if (temperature < temperatureThreshold) {
        quitAlarming=0;
    }

    // 温度控制逻辑 - 修复响应延迟问题
    static float lastTemperature = 0;
    static uint8_t fanState = 0;
    static uint8_t alarmState = 0;
    
    // 立即响应温度变化
    if (temperature < temperatureThreshold - 1) {
        if (fanState != 0) {
            fanState = 0;
            runningFun = 0;
            HAL_GPIO_WritePin(IN_GPIO_Port, IN_Pin, GPIO_PIN_SET);
            HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
        }
        if (alarmState != 0) {
            alarmState = 0;
            HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
        }
    } 
    else if (temperature >= temperatureThreshold) {
        if (fanState != 1) {
            fanState = 1;
            runningFun = 1;
            HAL_GPIO_WritePin(IN_GPIO_Port, IN_Pin, GPIO_PIN_RESET);
            HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 1000);
        }
        if (alarmState != 1 && quitAlarming != 2) {
            alarmState = 1;
            quitAlarming = 1;
            HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
        }
    } 
    else if (temperature >= temperatureThreshold - 1 && temperature < temperatureThreshold) {
        if (fanState != 1) {
            fanState = 1;
            runningFun = 1;
            HAL_GPIO_WritePin(IN_GPIO_Port, IN_Pin, GPIO_PIN_RESET);
            HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 1000);
        }
        if (alarmState != 0) {
            alarmState = 0;
            HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
        }
    }
    
    lastTemperature = temperature;
}

/**
  * @brief  显示蓝牙设置界面，处理蓝牙接收的数据。
  * @retval None
  */
void Show_BlueTooth()
{
    PageOffset = 0, ColOffset = 0;
    // 第一行显示
    for (CharNum = 12; CharNum < 14; CharNum++)   
    {
        OLED_Display_Char_16X16(PageOffset, ColOffset, CharNum);
        ColOffset += 16;
    }
    OLED_Display_String_8X16(2, 0, "Rec:");
    ColOffset += 32;
    if (rf == 1)
    {
        rf = 0;
        // 接收到字符串
        OLED_Display_String_8X16(2, 32, (uint8_t *)str);
        OLED_Display_String_8X16(4, 0, "result:");
        Bluetooth(str);

        HAL_UART_Receive_IT(&huart2, (uint8_t *)&str, 8);
    }
}
/**
  * @brief  显示设置界面，展示时间段和温度阈值。
  * @retval None
  */
void Show_Setting()
{ 
    char temp_str[32];
    PageOffset = 0, ColOffset = 0;
    // 第一行显示
    for (CharNum = 14; CharNum < 16; CharNum++)   
    {
        OLED_Display_Char_16X16(PageOffset, ColOffset, CharNum);
        ColOffset += 16;
    }

    // 显示时间段开始时间
    sprintf(temp_str, "Start: %02d:%02d", 
            blueToothSettings.start_hour, blueToothSettings.start_minute);
    OLED_Display_String_8X16(2, 0, (uint8_t *)temp_str);

    // 显示时间段结束时间
    sprintf(temp_str, "End: %02d:%02d", 
            blueToothSettings.end_hour, blueToothSettings.end_minute);
    OLED_Display_String_8X16(4, 0, (uint8_t *)temp_str);

    // 显示温度阈值
    sprintf(temp_str, "Temp: %.1f°C", temperatureThreshold);
    OLED_Display_String_8X16(6, 0, (uint8_t *)temp_str);
}
/**
  * @brief  UART 接收完成回调函数，处理蓝牙数据接收。
  * @param  huart: UART 句柄。
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart2)
    {
        // 确保接收到的数据有正确的结束符
        str[8] = '\0'; 
        rf = 1;
        HAL_UART_Receive_IT(&huart2, (uint8_t *)&str, 8);
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
