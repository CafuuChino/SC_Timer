//
// Created by CafuuChino on 2022/9/17.
//

#ifndef SC_TIMER_GPIO_INPUT_H
#define SC_TIMER_GPIO_INPUT_H
#include "main.h"

typedef struct GPIO_Package_def {
    GPIO_TypeDef *GPIO_Type;
    uint16_t GPIO_Num;
} GPIO_Package;

GPIO_TypeDef *get_GPIO_Type(const char* gpio_str);
uint16_t get_GPIO_Num(const char* gpio_str);
uint8_t digitalPin_Read(char* GPIO);
void digitalPin_Write(char* GPIO, uint8_t pin_state);
void digitalPin_Toggle(char* GPIO);
uint8_t key_detect();
uint16_t rot_change_u16(uint16_t u16, uint8_t digit, uint8_t *update);
uint8_t rot_change_u8(uint8_t u8, uint8_t min, uint8_t max, uint8_t *update);
#endif //SC_TIMER_GPIO_INPUT_H
