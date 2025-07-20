/*OLED代码对应头文件*/
#ifndef __OLED_H            //预处理，条件编译
#define __OLED_H            //宏定义
#include "main.h"

/*函数声明*/
HAL_StatusTypeDef OLED_Write_Command(uint8_t* Command, uint16_t Size);
HAL_StatusTypeDef OLED_Write_Data(uint8_t* Data, uint16_t Size);
HAL_StatusTypeDef OLED_Set_Position(uint8_t PageOffset, uint8_t ColumnOffset);
HAL_StatusTypeDef OLED_Clear_Screen(void);
HAL_StatusTypeDef OLED_Display_Char_8X16(uint8_t PageOffset, uint8_t ColumnOffset, uint8_t CharNum);
HAL_StatusTypeDef OLED_Display_String_8X16(uint8_t PageOffset, uint8_t ColumnOffset, uint8_t* String);
HAL_StatusTypeDef OLED_Display_Char_16X16(uint8_t PageOffset, uint8_t ColumnOffset, uint8_t CharNum);
HAL_StatusTypeDef OLED_Init(void);
HAL_StatusTypeDef OLED_Display_Image(uint8_t PageOffset, uint8_t ColumnOffset, uint8_t height,uint8_t weight);

#endif                    //结束条件编译
