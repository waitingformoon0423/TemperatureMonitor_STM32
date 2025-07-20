#ifndef __BSP_DS18B20_H
#define __BSP_DS18B20_H
 
#include "stm32f1xx_hal.h"
#include "main.h"

#define DS18B20_PORT             DQ_GPIO_Port
#define DS18B20_PIN              DQ_Pin
 
#define DS18B20_OUT_1            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET)
#define DS18B20_OUT_0            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET)
 
#define DS18B20_IN			         HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN)

void delay_us(uint16_t nms);
 
uint8_t DS18B20_Init(void);
void DS18B20_ReadId(uint8_t *ds18b20_id);
float DS18B20_GetTemp_SkipRom(void);
float DS18B20_GetTemp_MatchRom(uint8_t * ds18b20_id);
 
#endif

