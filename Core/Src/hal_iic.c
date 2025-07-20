#include "hal_iic.h"
#include "delay_us.h"

/*************************
函数名称：IIC_Set_SDAMode
函数功能: 设置SDA引脚模式
输入参数：GPIO模式
输出参数：无
*************************/
void IIC_Set_SDAMode(uint32_t Mode)
{
	GPIO_InitTypeDef GPIO_InitStruct = {I2C_SDA_Pin, Mode, GPIO_PULLDOWN, GPIO_SPEED_FREQ_LOW};
  HAL_GPIO_Init(I2C_SDA_GPIO_Port, &GPIO_InitStruct);
}

/*************************
函数名称：IIC_Start
函数功能: 产生IIC起始信号
输入参数：无
输出参数：无
*************************/
void IIC_Start(void)
{
//sda线输出
	IIC_Set_SDAMode(GPIO_MODE_OUTPUT_PP);
	HAL_GPIO_WritePin(GPIOB, I2C_SCL_Pin|I2C_SDA_Pin, GPIO_PIN_SET);
	DELAY_Us(4);
	HAL_GPIO_WritePin(GPIOB,I2C_SDA_Pin, GPIO_PIN_RESET);
	DELAY_Us(4);
//钳住I2C总线，准备发送或接收数据
	HAL_GPIO_WritePin(GPIOB,I2C_SCL_Pin, GPIO_PIN_RESET);
}

/*************************
函数名称：IIC_Stop
函数功能: 产生IIC停止信号
输入参数：无
输出参数：无
*************************/
void IIC_Stop(void)
{
//sda线输出
	IIC_Set_SDAMode(GPIO_MODE_OUTPUT_PP);
	HAL_GPIO_WritePin(GPIOB, I2C_SCL_Pin|I2C_SDA_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, I2C_SCL_Pin, GPIO_PIN_SET);
	DELAY_Us(4);
//发送I2C总线结束信号
	HAL_GPIO_WritePin(GPIOB, I2C_SDA_Pin, GPIO_PIN_SET);
  DELAY_Us(4);
}

/*************************
函数名称：IIC_WaitAck
函数功能: 等待应答信号到来
输入参数：无
输出参数：1，接收应答失败
          0，接收应答成功
*************************/
HAL_StatusTypeDef IIC_WaitAck(void)
{
	uint8_t count=0;
//sda线输入
	IIC_Set_SDAMode(GPIO_MODE_INPUT);
	HAL_GPIO_WritePin(GPIOB,I2C_SDA_Pin, GPIO_PIN_SET);
	DELAY_Us(1);
	HAL_GPIO_WritePin(GPIOB,I2C_SCL_Pin, GPIO_PIN_SET);
	DELAY_Us(1);
	while(HAL_GPIO_ReadPin(GPIOB, I2C_SDA_Pin))
	{
		count++;
		if(count>250)
		{
			IIC_Stop();
			return HAL_ERROR;
		}
	}
//时钟输出0	
	HAL_GPIO_WritePin(GPIOB,I2C_SCL_Pin, GPIO_PIN_RESET);
	return HAL_OK;
}

/*********************
函数名称：IIC_SendAck
函数功能: 发送应答
输入参数：无
输出参数：无
*********************/
void IIC_SendAck(void)
{
//sda线输出
	IIC_Set_SDAMode(GPIO_MODE_OUTPUT_PP);
	HAL_GPIO_WritePin(GPIOB, I2C_SCL_Pin|I2C_SDA_Pin, GPIO_PIN_RESET);
	DELAY_Us(2);
	HAL_GPIO_WritePin(GPIOB,I2C_SCL_Pin, GPIO_PIN_SET);
	DELAY_Us(2);
	HAL_GPIO_WritePin(GPIOB,I2C_SCL_Pin, GPIO_PIN_RESET);
}

/***********************
函数名称：IIC_SendNoAck
函数功能: 发送非应答
输入参数：无
输出参数：无
***********************/
void IIC_SendNoAck(void)
{
//sda线输出
	IIC_Set_SDAMode(GPIO_MODE_OUTPUT_PP);
	HAL_GPIO_WritePin(GPIOB, I2C_SCL_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, I2C_SDA_Pin, GPIO_PIN_SET);
	DELAY_Us(2);
	HAL_GPIO_WritePin(GPIOB,I2C_SCL_Pin, GPIO_PIN_SET);
	DELAY_Us(2);
	HAL_GPIO_WritePin(GPIOB,I2C_SCL_Pin, GPIO_PIN_RESET);
}

/*************************
函数名称：IIC_SendOneByte
函数功能: IIC发送一个字节
输入参数：无
输出参数：无
*************************/
void IIC_SendOneByte(uint8_t Data)
{
//sda线输出
	IIC_Set_SDAMode(GPIO_MODE_OUTPUT_PP);
//拉低时钟开始数据传输
	HAL_GPIO_WritePin(GPIOB,I2C_SCL_Pin, GPIO_PIN_RESET);
	for(uint8_t i=0;i<8;i++)
	{
		(((Data&0x80)>>7) == 0) ? HAL_GPIO_WritePin(GPIOB,I2C_SDA_Pin, GPIO_PIN_RESET): HAL_GPIO_WritePin(GPIOB,I2C_SDA_Pin, GPIO_PIN_SET);
		Data<<=1;
		DELAY_Us(2);
		HAL_GPIO_WritePin(GPIOB,I2C_SCL_Pin, GPIO_PIN_SET);
		DELAY_Us(2);
		HAL_GPIO_WritePin(GPIOB,I2C_SCL_Pin, GPIO_PIN_RESET);
		DELAY_Us(2);
	}
}

/************************************************
函数名称：IIC_SendOneByte
函数功能: IIC读出一个字节
输入参数：IsLastByte: 1, 发送应答; 0, 发送非应答
输出参数：读到的数据
************************************************/
uint8_t IIC_ReadOneByte(_Bool SendAck)
{
	uint8_t rx_buf=0;
//sda线输入
	IIC_Set_SDAMode(GPIO_MODE_INPUT);
	for(uint8_t i=0; i<8; i++)
	{
		HAL_GPIO_WritePin(GPIOB,I2C_SCL_Pin, GPIO_PIN_RESET);
		DELAY_Us(2);
		HAL_GPIO_WritePin(GPIOB,I2C_SCL_Pin, GPIO_PIN_SET);
		rx_buf<<=1;
		if(HAL_GPIO_ReadPin(GPIOB, I2C_SDA_Pin))
			rx_buf++;
		DELAY_Us(1);
	}
	if(SendAck)
		IIC_SendAck();
	else
		IIC_SendNoAck();
	return rx_buf;
}
