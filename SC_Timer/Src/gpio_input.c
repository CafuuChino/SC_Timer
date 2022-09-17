//
// Created by CafuuChino on 2022/9/17.
//

#include <stdlib.h>
#include <math.h>
#include "gpio_input.h"
#include "SC_Timer_Core.h"

#define LONG_PRESS_TIME 700
uint8_t release = 1;

GPIO_TypeDef *get_GPIO_Type(const char* gpio_str){
    switch(gpio_str[0]){
#ifdef GPIOA
        case 'A': return GPIOA;
#endif
#ifdef GPIOB
        case 'B': return GPIOB;
#endif
#ifdef GPIOC
        case 'C': return GPIOC;
#endif
#ifdef GPIOD
        case 'D': return GPIOD;
#endif
#ifdef GPIOE
            case 'E': return GPIOE;
#endif
#ifdef GPIOF
            case 'F': return GPIOF;
#endif
#ifdef GPIOG
            case 'G': return GPIOG;
#endif
#ifdef GPIOH
            case 'H': return GPIOH;
#endif
        default: return GPIOA;
    }
}
uint16_t get_GPIO_Num(const char* gpio_str){
    char *not_dig_ptr;
    uint8_t pin_num = strtol(gpio_str+1, &not_dig_ptr, 10);
    switch(pin_num){
        case 0 : return GPIO_PIN_0;
        case 1 : return GPIO_PIN_1;
        case 2 : return GPIO_PIN_2;
        case 3 : return GPIO_PIN_3;
        case 4 : return GPIO_PIN_4;
        case 5 : return GPIO_PIN_5;
        case 6 : return GPIO_PIN_6;
        case 7 : return GPIO_PIN_7;
        case 8 : return GPIO_PIN_8;
        case 9 : return GPIO_PIN_9;
        case 10 : return GPIO_PIN_10;
        case 11 : return GPIO_PIN_11;
        case 12 : return GPIO_PIN_12;
        case 13 : return GPIO_PIN_13;
        case 14 : return GPIO_PIN_14;
        case 15 : return GPIO_PIN_15;
        default : return GPIO_PIN_0;
    }
}

uint8_t digitalPin_Read(char* GPIO){
    return HAL_GPIO_ReadPin(
            get_GPIO_Type(GPIO),
            get_GPIO_Num(GPIO));
}
void digitalPin_Write(char* GPIO, uint8_t pin_state){
    HAL_GPIO_WritePin(
            get_GPIO_Type(GPIO),
            get_GPIO_Num(GPIO),
            pin_state);
}
void digitalPin_Toggle(char* GPIO){
    HAL_GPIO_TogglePin(get_GPIO_Type(GPIO),
                       get_GPIO_Num(GPIO));
}

/**
 * @brief Detect KeyPress Type
 * @return 0-NoPress 1-ShortPress 2-LongPress 3-DoublePress
 */
uint8_t key_detect(){
    uint16_t t = 0;
    if (digitalPin_Read(BUTTON_PIN)){
        release = 1;
        return 0;
    }
    if (!digitalPin_Read(BUTTON_PIN) && release){
        release = 0;
        HAL_Delay(10);
        if(digitalPin_Read(BUTTON_PIN)){
            return 0;
        }
        while((!digitalPin_Read(BUTTON_PIN)) && (t < LONG_PRESS_TIME)){
            t++;
            HAL_Delay(1);
        }
        if(t >= LONG_PRESS_TIME){
            return 2;
        }
        HAL_Delay(220);
        if(digitalPin_Read(BUTTON_PIN)){
            release = 1;
            return 1;

        }
        else{
            return 3;
        }
    }
    return 0;
}

uint16_t rot_change_u16(uint16_t u16, uint8_t digit, uint8_t *update){
    uint16_t target_u16 = u16;
    if (__HAL_TIM_GET_COUNTER(&htim2) != ENCODER_DEFAULT){
        *update = 1;
        uint32_t pow1 = (uint32_t)pow(10, digit);
        uint16_t pow2 = (uint16_t)pow(10, digit-1);
        int16_t enc_delta = (__HAL_TIM_GET_COUNTER(&htim2) - ENCODER_DEFAULT);
        uint8_t num = (target_u16 - (pow1 * (target_u16 / pow1))) / pow2;
        num = num + 10 + enc_delta;
        num -= 10 * (num / 10);
        uint32_t test_target = (pow1 * (target_u16 / pow1)) + (num * pow2) + (target_u16 % pow2);
        //OLED_ShowU16(0,2,num,5,0,1);
        //OLED_ShowU16(0,4,enc_delta + 10,5,0,1);
        while (test_target > 65535){
            if (enc_delta >= 0) num = 0;
            else num--;
            test_target = (pow1 * (target_u16 / pow1)) + (num * pow2) + (target_u16 % pow2);
        }
        target_u16 = test_target;
        __HAL_TIM_SET_COUNTER(&htim2, ENCODER_DEFAULT);
    }
    return target_u16;
}

uint8_t rot_change_u8(uint8_t u8, uint8_t min, uint8_t max, uint8_t *update){
    int16_t target_u8 = u8;
    if (__HAL_TIM_GET_COUNTER(&htim2) != ENCODER_DEFAULT){
        *update = 1;
        int16_t enc_delta = (__HAL_TIM_GET_COUNTER(&htim2) - ENCODER_DEFAULT);
        target_u8 = (enc_delta > 0) ? target_u8 + 1 : target_u8 - 1;
        if (target_u8 > max) target_u8 = min;
        if (target_u8 < min) target_u8 = max;
        __HAL_TIM_SET_COUNTER(&htim2, ENCODER_DEFAULT);
    }
    return (uint8_t)target_u8;
}