//
// Created by CafuuChino on 2022/9/16.
//

#ifndef SC_TIMER_OLED_H
#define SC_TIMER_OLED_H
#include "main.h"



void WriteCmd(void);
//向设备写控制命令
void OLED_WR_CMD(uint8_t cmd);
//向设备写数据
void OLED_WR_DATA(uint8_t data);
//初始化oled屏幕
void OLED_Init(void);
//清屏
void OLED_Clear(void);

//设置光标
void OLED_Set_Pos(uint8_t x, uint8_t y);

uint8_t OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr, uint8_t mode);
//显示一个字符号串
uint8_t OLED_ShowString(uint8_t x,uint8_t y,char *chr, uint8_t mode);
//显示汉字
//hzk 用取模软件得出的数组
void OLED_ShowChinese(uint8_t x,uint8_t y,uint8_t no);

//开启OLED显示
void OLED_Display_On(void);
//关闭OLED显示
void OLED_Display_Off(void);
void OLED_On(void);

//显示2个数字
//x,y :起点坐标
//len :数字的位数
//size:字体大小
//mode:模式	0,填充模式;1,叠加模式
//num:数值(0~4294967295);
void OLED_ShowNum(uint8_t x,uint8_t y,unsigned int num,uint8_t len,uint8_t size2);



#endif //SC_TIMER_OLED_H
