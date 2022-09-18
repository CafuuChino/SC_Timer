//
// Created by CafuuChino on 2022/9/17.
//

#ifndef SC_TIMER_GPIO_INPUT_H
#define SC_TIMER_GPIO_INPUT_H
#include "main.h"

typedef struct GPIO_PackageStruct{
    GPIO_TypeDef *gpio;
    uint16_t pin;
}GPIO_Struct;

typedef GPIO_Struct* GPIO_Type;


#define A0 &a0_
#define A1 &a1_
#define A2 &a2_
#define A3 &a3_
#define A4 &a4_
#define A5 &a5_
#define A6 &a6_
#define A7 &a7_
#define A8 &a8_
#define A9 &a9_
#define A10 &a10_
#define A11 &a11_
#define A12 &a12_
#define A13 &a13_
#define A14 &a14_
#define A15 &a15_
#define B0 &b0_
#define B1 &b1_
#define B2 &b2_
#define B3 &b3_
#define B4 &b4_
#define B5 &b5_
#define B6 &b6_
#define B7 &b7_
#define B8 &b8_
#define B9 &b9_
#define B10 &b10_
#define B11 &b11_
#define B12 &b12_
#define B13 &b13_
#define B14 &b14_
#define B15 &b15_
#define C13 &c13_
#define C14 &c14_
#define C15 &c15_
extern GPIO_Struct a0_;
extern GPIO_Struct a1_;
extern GPIO_Struct a2_;
extern GPIO_Struct a3_;
extern GPIO_Struct a4_;
extern GPIO_Struct a5_;
extern GPIO_Struct a6_;
extern GPIO_Struct a7_;
extern GPIO_Struct a8_;
extern GPIO_Struct a9_;
extern GPIO_Struct a10_;
extern GPIO_Struct a11_;
extern GPIO_Struct a12_;
extern GPIO_Struct a13_;
extern GPIO_Struct a14_;
extern GPIO_Struct a15_;
extern GPIO_Struct b0_;
extern GPIO_Struct b1_;
extern GPIO_Struct b2_;
extern GPIO_Struct b3_;
extern GPIO_Struct b4_;
extern GPIO_Struct b5_;
extern GPIO_Struct b6_;
extern GPIO_Struct b7_;
extern GPIO_Struct b8_;
extern GPIO_Struct b9_;
extern GPIO_Struct b10_;
extern GPIO_Struct b11_;
extern GPIO_Struct b12_;
extern GPIO_Struct b13_;
extern GPIO_Struct b14_;
extern GPIO_Struct b15_;
extern GPIO_Struct c13_;
extern GPIO_Struct c14_;
extern GPIO_Struct c15_;
GPIO_TypeDef *get_GPIO_Type(const char* gpio_str);
uint16_t get_GPIO_Num(const char* gpio_str);
uint8_t digitalPin_Read(GPIO_Type);
void digitalPin_Write(GPIO_Type, uint8_t pin_state);
void digitalPin_Toggle(GPIO_Type);
uint8_t key_detect();
uint16_t rot_change_u16(uint16_t u16, uint8_t digit, uint8_t *update);
uint8_t rot_change_u8(uint8_t u8, uint8_t min, uint8_t max, uint8_t *update);
#endif //SC_TIMER_GPIO_INPUT_H
