#include "main.h"
#include "ds1302.h"
#include "delay_us.h"

// BCD码转十进制：将BCD格式的数字转换为十进制整数
#define BCD2HEX(v)	((v) % 16 + (v) / 16 * 10)
#define HEX2BCD(v)	((v) % 10 + (v) / 10 * 16)	// 十进制转BCD码

// 存储从DS1302读取的原始时间数据（BCD格式）
uint8_t read_time[7];
 
// 存储转换后的十进制时间数据
struct TIMEData TimeSDA;
// 存储格式化后的日期字符串（格式：YYYY-MM-DD）
char DS1302_data_1[10];
// 存储格式化后的时间字符串（格式：HH:MM:SS）
char DS1302_data_2[8];
 
 
/*
 * 配置SDA引脚为输出模式
 * 用于向DS1302写入数据
 */
void ds1302_SDAOUT_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
 
  // 使能GPIOA时钟
  __HAL_RCC_GPIOA_CLK_ENABLE();
 
  // 初始化SDA引脚为低电平
  HAL_GPIO_WritePin(GPIOA, SDA_Pin, GPIO_PIN_RESET);
	
	// 配置SDA引脚为推挽输出模式
  GPIO_InitStruct.Pin = SDA_Pin;   
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SDA_GPIO_Port, &GPIO_InitStruct);
}
 
/*
 * 配置SDA引脚为输入模式
 * 用于从DS1302读取数据
 */
void ds1302_SDAINPUT_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
 
  // 使能GPIOA时钟
  __HAL_RCC_GPIOA_CLK_ENABLE();
 
	// 配置SDA引脚为浮空输入模式
  GPIO_InitStruct.Pin = SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SDA_GPIO_Port, &GPIO_InitStruct);
}
 
 
/*
 * 向DS1302发送一个字节数据（低位在前）
 * SDA: 要发送的字节数据
 */
void ds1302_write_onebyte(uint8_t SDA)
{
	uint8_t count = 0;
	ds1302_SDAOUT_init(); // 配置SDA为输出模式
	
	HAL_GPIO_WritePin(GPIOA, SCL_Pin, GPIO_PIN_RESET); // 拉低时钟线
	
	// 逐位发送数据（低位在前）
	for(count = 0; count < 8; count++)
	{	
		HAL_GPIO_WritePin(GPIOA, SCL_Pin, GPIO_PIN_RESET); // 拉低时钟线
		
		// 根据数据位设置SDA引脚电平
		if(SDA & 0x01)
		{
			HAL_GPIO_WritePin(GPIOA, SDA_Pin, GPIO_PIN_SET);
		}
		else
		{
			HAL_GPIO_WritePin(GPIOA, SDA_Pin, GPIO_PIN_RESET);
		}
		
		HAL_GPIO_WritePin(GPIOA, SCL_Pin, GPIO_PIN_SET); // 拉高时钟线，发送数据
		SDA >>= 1; // 右移一位，准备发送下一位
	}
}
 
/*
 * 向DS1302指定寄存器写入数据
 * address: 寄存器地址（写操作时最低位应为0）
 * SDA: 要写入的数据
 */
void ds1302_wirte_rig(uint8_t address, uint8_t SDA)
{
	uint8_t temp1 = address;
	uint8_t temp2 = SDA;
	
	// 通信开始：拉低RST和SCL
	HAL_GPIO_WritePin(GPIOA, RST_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, SCL_Pin, GPIO_PIN_RESET);
	DELAY_Us(1);
	
	// 拉高RST，启动通信
	HAL_GPIO_WritePin(GPIOA, RST_Pin, GPIO_PIN_SET);
	DELAY_Us(2);
	
	ds1302_write_onebyte(temp1); // 发送寄存器地址
	ds1302_write_onebyte(temp2); // 发送数据
	
	// 通信结束：拉低RST和SCL
	HAL_GPIO_WritePin(GPIOA, RST_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, SCL_Pin, GPIO_PIN_RESET);
	DELAY_Us(2);
}
 
/*
 * 从DS1302指定寄存器读取数据
 * address: 寄存器地址（读操作时最低位应为1）
 * 返回: 读取到的数据
 */
uint8_t ds1302_read_rig(uint8_t address)
{
	uint8_t temp3 = address;
	uint8_t count = 0;
	uint8_t return_SDA = 0x00;
	
	// 通信开始：拉低RST和SCL
	HAL_GPIO_WritePin(GPIOA, RST_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, SCL_Pin, GPIO_PIN_RESET);
	DELAY_Us(3);
	
	// 拉高RST，启动通信
	HAL_GPIO_WritePin(GPIOA, RST_Pin, GPIO_PIN_SET);
	DELAY_Us(3);
	
	ds1302_write_onebyte(temp3); // 发送读寄存器命令
	
	ds1302_SDAINPUT_init(); // 配置SDA为输入模式，准备接收数据
 
	// 逐位接收数据（低位在前）
	for(count = 0; count < 8; count++)
	{
		DELAY_Us(2); // 延时确保电平稳定
		return_SDA >>= 1; // 右移一位，准备接收新位
		
		HAL_GPIO_WritePin(GPIOA, SCL_Pin, GPIO_PIN_SET); // 拉高时钟线
		DELAY_Us(4); // 延时确保电平稳定
		
		HAL_GPIO_WritePin(GPIOA, SCL_Pin, GPIO_PIN_RESET); // 拉低时钟线
		DELAY_Us(14); // 延时等待DS1302输出稳定
		
		// 读取SDA引脚电平
		if(HAL_GPIO_ReadPin(GPIOA, SDA_Pin)) 
		{
			return_SDA |= 0x80; // 设置最高位
		}
	}
	
	DELAY_Us(2);
	HAL_GPIO_WritePin(GPIOA, RST_Pin, GPIO_PIN_RESET); // 拉低RST，结束通信
	HAL_GPIO_WritePin(GPIOA, SDA_Pin, GPIO_PIN_RESET);	// 拉低SDA
	
	return return_SDA; // 返回读取到的数据
}
 
/*
 * 初始化DS1302时钟芯片，设置初始时间
 * 默认设置为2025年7月3日23:04:00，星期四
 */
void ds1302_init(uint8_t *start)
{
	ds1302_wirte_rig(0x8e, 0x00); // 关闭写保护，允许写入时钟寄存器
	
	// 设置初始时间（BCD格式）
//	ds1302_wirte_rig(0x80, 0x00); // 秒：00
//	ds1302_wirte_rig(0x82, 0x10); // 分：04
//	ds1302_wirte_rig(0x84, 0x23); // 时：23
//	ds1302_wirte_rig(0x86, 0x03); // 日：03
//	ds1302_wirte_rig(0x88, 0x07); // 月：07
//	ds1302_wirte_rig(0x8a, 0x07); // 星期：星期日(7)
//	ds1302_wirte_rig(0x8c, 0x25); // 年：2025 (BCD码为0x25)
	
		ds1302_wirte_rig(0x80, HEX2BCD(start[0])); // 秒：00
	ds1302_wirte_rig(0x82, HEX2BCD(start[1])); // 分：04
	ds1302_wirte_rig(0x84, HEX2BCD(start[2])); // 时：23
	ds1302_wirte_rig(0x86, HEX2BCD(start[3])); // 日：03
	ds1302_wirte_rig(0x88, HEX2BCD(start[4])); // 月：07
	ds1302_wirte_rig(0x8a, HEX2BCD(start[5])); // 星期：星期日(7)
	ds1302_wirte_rig(0x8c, HEX2BCD(start[6])); // 年：2025 (BCD码为0x25)
	
	ds1302_wirte_rig(0x8e, 0x80); // 开启写保护，防止意外写入
	
}
 
/*
 * 从DS1302读取时间数据（BCD格式）
 * 读取的时间数据存储在全局数组read_time中
 */
void ds1302_read_time(void)
{
	read_time[0] = ds1302_read_rig(0x81); // 读秒
	read_time[1] = ds1302_read_rig(0x83); // 读分
	read_time[2] = ds1302_read_rig(0x85); // 读时
	read_time[3] = ds1302_read_rig(0x87); // 读日
	read_time[4] = ds1302_read_rig(0x89); // 读月
	read_time[5] = ds1302_read_rig(0x8B); // 读星期
	read_time[6] = ds1302_read_rig(0x8D); // 读年
}
 
/*
 * 读取DS1302的实时时间并格式化为字符串
 * 将时间数据转换为十进制并格式化为字符串
 */
void ds1302_read_realTime(void)
{
	ds1302_read_time();  // 读取DS1302时间（BCD格式）
	
	// 将BCD格式的时间数据转换为十进制
	TimeSDA.second = BCD2HEX(read_time[0]);
	TimeSDA.minute = BCD2HEX(read_time[1]);
	TimeSDA.hour = BCD2HEX(read_time[2]);
	TimeSDA.day = BCD2HEX(read_time[3]);
	TimeSDA.month = BCD2HEX(read_time[4]);
	TimeSDA.week = BCD2HEX(read_time[5]);
	TimeSDA.year = BCD2HEX(read_time[6]);
	
	// 格式化日期字符串（格式：YYYY-MM-DD）
	DS1302_data_1[0] = '2';
	DS1302_data_1[1] = '0';
	DS1302_data_1[2] = '0' + (TimeSDA.year) / 10;      // 年的十位
	DS1302_data_1[3] = '0' + (TimeSDA.year) % 10;      // 年的个位
	DS1302_data_1[4] = '-';                              // 分隔符
	DS1302_data_1[5] = '0' + TimeSDA.month / 10;        // 月的十位
	DS1302_data_1[6] = '0' + TimeSDA.month % 10;        // 月的个位
	DS1302_data_1[7] = '-';                              // 分隔符
	DS1302_data_1[8] = '0' + TimeSDA.day / 10;          // 日的十位
	DS1302_data_1[9] = '0' + TimeSDA.day % 10;          // 日的个位

	// 格式化时间字符串（格式：HH:MM:SS）
	DS1302_data_2[0] = '0' + TimeSDA.hour / 10;         // 时的十位
	DS1302_data_2[1] = '0' + TimeSDA.hour % 10;         // 时的个位
	DS1302_data_2[2] = ':';                               // 分隔符
	DS1302_data_2[3] = '0' + TimeSDA.minute / 10;       // 分的十位
	DS1302_data_2[4] = '0' + TimeSDA.minute % 10;       // 分的个位
	DS1302_data_2[5] = ':';                               // 分隔符
	DS1302_data_2[6] = '0' + TimeSDA.second / 10;       // 秒的十位
	DS1302_data_2[7] = '0' + TimeSDA.second % 10;       // 秒的个位
}
