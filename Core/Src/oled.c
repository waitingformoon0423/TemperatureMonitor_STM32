#include "oled.h"
#include "oledfont.h"
#include "hal_iic.h"

#define OLED_ADDR_WRITE               0x78   //写器件地址
#define OLED_CTRL_CMD                 0x00   //写控制指令
#define OLED_CTRL_DATA                0x40   //写数据指令

/*********************** 基本命令表 *************************/
#define OLED_CMD_CONTRAST_CTRL        0x81   //设置对比度
#define OLED_CMD_CHARGE_PUMP          0x8D   //设置电荷泵
#define OLED_CMD_NORMAL_DISPLAY       0xA6   //正常显示
#define OLED_CMD_DISPLAY_OFF          0xAE   //关闭显示指令
#define OLED_CMD_DISPLAY_ON           0xAF   //开启显示指令

/*********************** 寻址设置命令表 *************************/
#define OLED_CMD_L_COL_START_ADDR     0x00   //低列起始地址，取值范围为0x00-0x0F
#define OLED_CMD_H_COL_START_ADDR     0x10   //高列起始地址，取值范围为0x10-0x1F 
#define OLED_CMD_PAGE_START_ADDR      0xB0   //页面起始地址，取值范围为0xB0-0xB7

/*********************** 硬件配置（面板分辨率和布局）命令表 *************************/
#define OLED_CMD_DISPLAY_START_LINE   0x40   //起始行，取值范围为0x40-0x7F
#define OLED_CMD_SEG_REMAP            0xA1   //段重定义, 取值为0xA0或者0xA1
#define OLED_CMD_MUX_RATIO            0xA8   //驱动路数
#define OLED_CMD_COM_SCAN_DIRECTION   0xC8   //重映射模式，取值为0xC0或者0xC8
#define OLED_CMD_DISPLAY_OFFSET       0xD3   //显示偏移
#define OLED_CMD_COM_PIN              0xDA   //COM硬件引脚配置

/*********************** 定时和驱动方案设置命令表 *************************/
#define OLED_CMD_DCLK_OSC             0xD5   //时钟分频因子,震荡频率,低4位为时钟分频因子，高4位为震荡频率
#define OLED_CMD_PRECHARGE            0xD9   //预充电周期
#define OLED_CMD_VCOMH                0xDB   //VCOMH 电压倍率

/****************************
函数名称：OLED_Write_Command
函数功能: 写指令
输入参数：指令
输出参数：状态值
****************************/
HAL_StatusTypeDef OLED_Write_Command(uint8_t* Command, uint16_t Size)
{
	IIC_Start();
	IIC_SendOneByte(OLED_ADDR_WRITE);
	if(IIC_WaitAck())
		return HAL_ERROR;
	IIC_SendOneByte(OLED_CTRL_CMD);
	if(IIC_WaitAck())
		return HAL_ERROR;
	for(uint16_t i=0; i<Size; i++)
	{
		IIC_SendOneByte(Command[i]);
	  if(IIC_WaitAck())
		  return HAL_ERROR;
	}
	IIC_Stop();
	return HAL_OK;
}

/****************************
函数名称：OLED_Write_Data
函数功能: 写数据
输入参数：数据
输出参数：状态值
****************************/
HAL_StatusTypeDef OLED_Write_Data(uint8_t* Data, uint16_t Size)
{
	IIC_Start();
	IIC_SendOneByte(OLED_ADDR_WRITE);
	if(IIC_WaitAck())
		return HAL_ERROR;
	IIC_SendOneByte(OLED_CTRL_DATA);
	if(IIC_WaitAck())
		return HAL_ERROR;
	for(uint16_t i=0; i<Size; i++)
	{
		IIC_SendOneByte(Data[i]);
	  if(IIC_WaitAck())
		  return HAL_ERROR;
	}	
	IIC_Stop();
	return HAL_OK;
}

/****************************
函数名称：OLED_Set_Position
函数功能: 设置位置
输入参数：PageOffset 页偏移量，取值范围0-7
          ColumnOffset 起始列偏移量，取值范围0-127
输出参数：状态值
****************************/
HAL_StatusTypeDef OLED_Set_Position(uint8_t PageOffset, uint8_t ColumnOffset)
{
	if(PageOffset>7 || ColumnOffset>127)
		return HAL_ERROR;
	uint8_t pos[3];
//设置页地址
	pos[0]=0xb0+PageOffset;
//起始列地址的高4位
	pos[2]=0x10+(ColumnOffset>>4);
//起始列地址的低4位
	pos[1]=ColumnOffset&0x0f;
	return OLED_Write_Command(pos, sizeof(pos));
}

/****************************
函数名称：OLED_Clear_Screen
函数功能: 清屏
输入参数：无
输出参数：状态值
****************************/
HAL_StatusTypeDef OLED_Clear_Screen(void)
{
	uint8_t data[128];
	for(uint8_t i=0; i<8; i++)
	{
		if(OLED_Set_Position(i, 0))
			return HAL_ERROR;
		for(uint8_t j=0; j<128; j++)
		{
			data[j]=0;
		}
		if(OLED_Write_Data(data, 128))
			return HAL_ERROR;
	}
	return HAL_OK;
}

/**********************************
函数名称：OLED_Init
函数功能: OLED初始化
输入参数：无
输出参数：状态值
**********************************/
HAL_StatusTypeDef OLED_Init(void)
{
	uint8_t init_cmd[]={
		OLED_CMD_DISPLAY_OFF,           //关闭显示
		OLED_CMD_DCLK_OSC, 0x80,        //设置时钟分频因子,震荡频率
		OLED_CMD_MUX_RATIO, 0x3f,       //设置驱动路数,默认0X3F(1/64)
		OLED_CMD_DISPLAY_OFFSET,0,      //设置显示偏移,默认为0
		OLED_CMD_DISPLAY_START_LINE,    //设置显示开始行
		OLED_CMD_CHARGE_PUMP,0x14,      //设置电荷泵，开显示
		OLED_CMD_SEG_REMAP,             //设置段重定义，0->127
		OLED_CMD_COM_SCAN_DIRECTION,    //设置重映射模式，COM[N-1]~COM[0]扫描	
		OLED_CMD_COM_PIN, 0x12,         //设置COM硬件引脚配置
		OLED_CMD_CONTRAST_CTRL, 239,    //设置对比度,1~255 (亮度设置,越大越亮)
		OLED_CMD_PRECHARGE,0xf1,        //设置预充电周期	
		OLED_CMD_VCOMH, 0x40,           //设置VCOMH 电压倍率
		OLED_CMD_NORMAL_DISPLAY         //设置显示方式，正常显示		
	};
//写入初始化命令
  if(OLED_Write_Command(init_cmd, sizeof(init_cmd)))
		return HAL_ERROR;
//清屏
	if(OLED_Clear_Screen())
		return HAL_ERROR;
//开启显示
  init_cmd[0]=OLED_CMD_DISPLAY_ON;
	if(OLED_Write_Command(init_cmd, 1))
		return HAL_ERROR;	
	return HAL_OK;
}

HAL_StatusTypeDef OLED_Display_Char_8X16(uint8_t PageOffset, uint8_t ColumnOffset, uint8_t CharNum)
{
	uint8_t i;
	for(i=0; i<2; i++)
  {
		if(OLED_Set_Position(PageOffset, ColumnOffset))
			return HAL_ERROR;
		if(OLED_Write_Data((uint8_t*)(OLEDFONT_ASCII_8X16[CharNum]+i*8), 8))
			return HAL_ERROR;
		PageOffset++;
	}
	return HAL_OK;
}

HAL_StatusTypeDef OLED_Display_String_8X16(uint8_t PageOffset, uint8_t ColumnOffset, uint8_t* String)
{
	while((*String<='~')&&(*String>=' '))
	{
		if(OLED_Display_Char_8X16(PageOffset, ColumnOffset, (*String)-' '))
			return HAL_ERROR;
		ColumnOffset += 8;
		String++;
	}
	return HAL_OK;
}

HAL_StatusTypeDef OLED_Display_Char_16X16(uint8_t PageOffset, uint8_t ColumnOffset, uint8_t CharNum)
{
	uint8_t i;
	for(i=0; i<2; i++)
  {
		if(OLED_Set_Position(PageOffset, ColumnOffset))
			return HAL_ERROR;
		if(OLED_Write_Data((uint8_t*)(OLEDFONT_CHAR_16X16[CharNum*2+i]), 16))
			return HAL_ERROR;
		PageOffset++;
	}
	return HAL_OK;
}
HAL_StatusTypeDef OLED_Display_Image(uint8_t PageOffset, uint8_t ColumnOffset, uint8_t height,uint8_t weight)
{

	uint8_t i,pages=height/8;
	for(i=0; i<pages; i++)
  {
		if(OLED_Set_Position(PageOffset, ColumnOffset))
			return HAL_ERROR;
		if(OLED_Write_Data((uint8_t*)(image[i*weight/16]), weight))
			return HAL_ERROR;
		PageOffset++;
	}
	return HAL_OK;
}