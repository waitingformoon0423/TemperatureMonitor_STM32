#ifndef __DS1302_H__
#define __DS1302_H__

#include "main.h"
// BCD码转十进制：将BCD格式的数字转换为十进制整数
#define BCD2HEX(v)	((v) % 16 + (v) / 16 * 10)
#define HEX2BCD(v)	((v) % 10 + (v) / 10 * 16)	// 十进制转BCD码
struct TIMEData
{
char second;
char minute;
char hour;
char day;
char month;
char week;
char year;
};
extern struct TIMEData TimeSDA;

extern char DS1302_data_1[10];
extern char DS1302_data_2[8];
void ds1302_DATAOUT_init(void);
void ds1302_DATAINPUT_init(void);
void ds1302_write_onebyte(uint8_t data);
void ds1302_wirte_rig(uint8_t address,uint8_t data);
uint8_t ds1302_read_rig(uint8_t address);
void ds1302_init(uint8_t *start);
void ds1302_read_time(void);
void ds1302_read_realTime(void);

#endif
