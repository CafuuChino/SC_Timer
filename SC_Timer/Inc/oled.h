//
// Created by CafuuChino on 2022/9/16.
//

#ifndef SC_TIMER_OLED_H
#define SC_TIMER_OLED_H
#include "main.h"



void WriteCmd(void);
void OLED_WR_CMD(uint8_t cmd);
void OLED_WR_DATA(uint8_t data);

void OLED_Init(void);
void OLED_Clear(void);
void OLED_Set_Pos(uint8_t x, uint8_t y);

uint8_t OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr, uint8_t mode);
uint8_t OLED_ShowString(uint8_t x,uint8_t y,char *chr, uint8_t mode);

#endif //SC_TIMER_OLED_H
