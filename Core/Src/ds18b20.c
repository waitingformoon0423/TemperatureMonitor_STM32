#include "ds18b20.h"
#include "tim.h"


/**
  *
	*/
void delay_us(uint16_t nms)
{
	uint16_t counter=0;
	__HAL_TIM_SET_COUNTER(&htim3,0);
	HAL_TIM_Base_Start(&htim3);
	do
	{
		counter=__HAL_TIM_GET_COUNTER(&htim3);
	}
	while(counter<nms);
	HAL_TIM_Base_Stop(&htim3);
}
/**
  * @brief DS18B20 输出模式
  */

static void DS18B20_Mode_OUT_PP(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
	GPIO_InitStruct.Pin = DS18B20_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	
	HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);
}
 
/**
  * @brief DS18B20 输入模式
  */
static void DS18B20_Mode_IN_NP(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
	GPIO_InitStruct.Pin = DS18B20_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
 
	HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);
}
 
/**
  * @brief 主机给从机发送复位脉冲
  */
static void DS18B20_Reset(void)
{
	DS18B20_Mode_OUT_PP();		// 主机输出
	
	DS18B20_OUT_0;						// 主机至少产生 480us 的低电平复位信号 
	delay_us(750);
	DS18B20_OUT_1;						// 主机在产生复位信号后，需将总线拉高 
	
	// 从机接收到主机的复位信号后，会在 15 ~ 60 us 后给主机发一个存在脉冲
	delay_us(15);					
}
 
/**
  * @brief  检测从机给主机返回的存在脉冲
  * @return 0：成功		1：失败
	*/
static uint8_t DS18B20_Presence(void)
{
	uint8_t pulse_time = 0;
	
	DS18B20_Mode_IN_NP();				// 主机设为输入 
	
	// 等待存在脉冲的到来，存在脉冲为一个 60 ~ 240 us 的低电平信号 
	// 如果存在脉冲没有来则做超时处理，从机接收到主机的复位信号后，会在 15 ~ 60 us 后给主机发一个存在脉冲
 
	while (DS18B20_IN && (pulse_time < 100))		// 等待低电平响应脉冲
	{
		pulse_time++;
		delay_us(1);
	}	
	// 经过 100 us 后，存在脉冲都还没有到来
	if (pulse_time >= 100)
	{
		return 1;
	}
	else
	{
		pulse_time = 0;		
	}
		
	// 响应脉冲（低电平）到来，且存在的时间不能超过 240 us 
	while(!(DS18B20_IN) && pulse_time < 240)
	{
		pulse_time++;
		delay_us(1);
	}	
	if(pulse_time >= 240)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
 
 /**
   * @brief  DS18B20 初始化函数
   * @reurn  0：成功		1：失败
   */
uint8_t DS18B20_Init(void)
{
	DS18B20_Mode_OUT_PP();
	DS18B20_OUT_1;
	
	DS18B20_Reset();
	return DS18B20_Presence();
}
 
/**
  * @brief 从DS18B20读取一个bit
  */
static uint8_t DS18B20_ReadBit(void)
{
	uint8_t dat;
	
	DS18B20_Mode_OUT_PP();	// 读 0 和读 1 的时间至少要大于 60 us 
	
	DS18B20_OUT_0;					// 读时间的起始：必须由主机产生 > 1us < 15us 的低电平信号 
	
	// 这个时间非常重要，设置为 < 15，需要多次尝试；如果设置不合理，数据会直接出错
	// 参数建议设为 10 11 12，具体值要根据延时函数的执行效率测试
	delay_us(10);		
	
	DS18B20_Mode_IN_NP();		// 设置成输入，释放总线，由外部上拉电阻将总线拉高 
	
	if (DS18B20_IN == 1)
	{
		dat = 1;
	}
	else
	{
		dat = 0;
	}
		
	delay_us(45);				// 这个延时参数参考时序图 
	
	return dat;
}
 
/**
  * @brief 从 DS18B20 读一个字节，低位先行
  */
static uint8_t DS18B20_ReadByte(void)
{
	uint8_t i, j, dat = 0;	
	
	for(i = 0; i < 8; i++) 
	{
		j = DS18B20_ReadBit();		
		dat = (dat) | (j << i);
	}
	
	return dat;
}
 
/**
  * @brief 写一个字节到 DS18B20，低位先行
  */
static void DS18B20_WriteByte(uint8_t dat)
{
	uint8_t i, testb;
	DS18B20_Mode_OUT_PP();
	
	for( i = 0; i < 8; i++ )
	{
		testb = dat & 0x01;
 
		dat = dat >> 1;		
		
		// 写 0 和写 1 的时间至少要大于60us 
		
		if (testb)			// 当前位写 1
		{			
			DS18B20_OUT_0;
			
			delay_us(5);		// 拉低发送写时段信号
			
			DS18B20_OUT_1;			// 读取电平时间保持高电平
			delay_us(65);		
		}		
		else						// 当前位写 0			
		{			
			DS18B20_OUT_0;			// 拉低发送写时段信号
			delay_us(70);		// 读取电平时间保持低电平
			
			DS18B20_OUT_1;				
			delay_us(2);		// 恢复时间
		}
	}
}
 
/**
  * @brief  跳过匹配 DS18B20 ROM
  */
static void DS18B20_SkipRom(void)
{
	DS18B20_Reset();	   
	
	DS18B20_Presence();	 
	
	DS18B20_WriteByte(0XCC);		/* 跳过 ROM */
}
 
/**
  * @brief  执行匹配 DS18B20 ROM
  */
static void DS18B20_MatchRom(void)
{
	DS18B20_Reset();	   
	
	DS18B20_Presence();	 
	
	DS18B20_WriteByte(0X55);		/* 匹配 ROM */
}
 
/**
	* 存储的温度是16 位的带符号扩展的二进制补码形式
	* 当工作在12位分辨率时，其中5个符号位，7个整数位，4个小数位
	*
	*         |---------整数----------|-----小数 分辨率 1/(2^4)=0.0625----|
	* 低字节  | 2^3 | 2^2 | 2^1 | 2^0 | 2^(-1) | 2^(-2) | 2^(-3) | 2^(-4) |
	*
	*
	*         |-----符号位：0->正  1->负-------|-----------整数-----------|
	* 高字节  |  s  |  s  |  s  |  s  |    s   |   2^6  |   2^5  |   2^4  |
	*
	* 
	* 温度 = 符号位 + 整数 + 小数*0.0625
	*/
 
/**
  * @brief  在跳过匹配 ROM 情况下获取 DS18B20 温度值 
  * @param  无
  * @retval 温度值
  */
float DS18B20_GetTemp_SkipRom(void)
{
	uint8_t tpmsb, tplsb;
	int16_t s_tem;
	float f_tem;
	
	DS18B20_SkipRom();
	DS18B20_WriteByte(0X44);				/* 开始转换 */
	HAL_Delay(750);                 	                               /*延迟750ms，过滤默认值85°，十分重要*/
	DS18B20_SkipRom();
  DS18B20_WriteByte(0XBE);				/* 读温度值 */
	
	tplsb = DS18B20_ReadByte();		 
	tpmsb = DS18B20_ReadByte(); 
	
	s_tem = tpmsb << 8;
	s_tem = s_tem | tplsb;
	
	if(s_tem < 0)			/* 负温度 */
	{
		f_tem = (~s_tem + 1) * 0.0625f;	
	}
	else
	{
		f_tem = s_tem * 0.0625f;
	}
		
	return f_tem; 	
}
 
/**
  * @brief  在匹配 ROM 情况下获取 DS18B20 温度值 
  * @param  ds18b20_id：用于存放 DS18B20 序列号的数组的首地址
  */
void DS18B20_ReadId(uint8_t *ds18b20_id)
{
	uint8_t uc;
	
	DS18B20_WriteByte(0x33);       //读取序列号
	
	for (uc = 0; uc < 8; uc++)
	{
		ds18b20_id[uc] = DS18B20_ReadByte();
	}
}
 
/**
  * @brief  在匹配 ROM 情况下获取 DS18B20 温度值 
  * @param  ds18b20_id：存放 DS18B20 序列号的数组的首地址
  * @retval 温度值
  */
float DS18B20_GetTemp_MatchRom(uint8_t * ds18b20_id)
{
	uint8_t tpmsb, tplsb, i;
	int16_t s_tem;
	float f_tem;
	
	DS18B20_MatchRom();            	/* 匹配ROM */
	
  for(i = 0;i < 8; i++)
	{
		DS18B20_WriteByte(ds18b20_id[i]);
	}
		
	DS18B20_WriteByte(0X44);				/* 开始转换 */
	HAL_Delay(750);                 /*延迟750ms，过滤初始值85°，十分重要*/ 
	DS18B20_MatchRom();            	/* 匹配ROM */
	
	for(i = 0; i < 8; i++)
	{
		DS18B20_WriteByte(ds18b20_id[i]);	
	}
		
	DS18B20_WriteByte(0XBE);				/* 读温度值 */
	
	tplsb = DS18B20_ReadByte();		 
	tpmsb = DS18B20_ReadByte(); 
	
	s_tem = tpmsb << 8;
	s_tem = s_tem | tplsb;
	
	if (s_tem < 0)									/* 负温度 */
	{
		f_tem = (~s_tem + 1) * 0.0625f;	
	}
	else
	{
		f_tem = s_tem * 0.0625f;
	}
		
	return f_tem; 		
}






