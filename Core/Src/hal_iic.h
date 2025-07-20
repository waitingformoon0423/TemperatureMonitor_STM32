#ifndef __IIC_H            //预处理，条件编译
#define __IIC_H            //宏定义
#include "main.h"

/*函数声明*/
void IIC_Set_SDAMode(uint32_t Mode);
void IIC_Start(void);
void IIC_Stop(void);
HAL_StatusTypeDef IIC_WaitAck(void);
void IIC_SendAck(void);
void IIC_SendNoAck(void);
void IIC_SendOneByte(uint8_t Data);
uint8_t IIC_ReadOneByte(_Bool SendAck);

#endif                    //结束条件编译
