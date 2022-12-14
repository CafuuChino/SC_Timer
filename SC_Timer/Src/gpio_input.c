//
// Created by CafuuChino on 2022/9/17.
//

#include <math.h>
#include "gpio_input.h"
#include "SC_Timer_Core.h"

#define LONG_PRESS_TIME 500

uint8_t release = 1;

GPIO_Struct a0_ = {GPIOA, GPIO_PIN_0};
GPIO_Struct a1_ = {GPIOA, GPIO_PIN_1};
GPIO_Struct a2_ = {GPIOA, GPIO_PIN_2};
GPIO_Struct a3_ = {GPIOA, GPIO_PIN_3};
GPIO_Struct a4_ = {GPIOA, GPIO_PIN_4};
GPIO_Struct a5_ = {GPIOA, GPIO_PIN_5};
GPIO_Struct a6_ = {GPIOA, GPIO_PIN_6};
GPIO_Struct a7_ = {GPIOA, GPIO_PIN_7};
GPIO_Struct a8_ = {GPIOA, GPIO_PIN_8};
GPIO_Struct a9_ = {GPIOA, GPIO_PIN_9};
GPIO_Struct a10_ = {GPIOA, GPIO_PIN_10};
GPIO_Struct a11_ = {GPIOA, GPIO_PIN_11};
GPIO_Struct a12_ = {GPIOA, GPIO_PIN_12};
GPIO_Struct a13_ = {GPIOA, GPIO_PIN_13};
GPIO_Struct a14_ = {GPIOA, GPIO_PIN_14};
GPIO_Struct a15_ = {GPIOA, GPIO_PIN_15};
GPIO_Struct b0_ = {GPIOB, GPIO_PIN_0};
GPIO_Struct b1_ = {GPIOB, GPIO_PIN_1};
GPIO_Struct b2_ = {GPIOB, GPIO_PIN_2};
GPIO_Struct b3_ = {GPIOB, GPIO_PIN_3};
GPIO_Struct b4_ = {GPIOB, GPIO_PIN_4};
GPIO_Struct b5_ = {GPIOB, GPIO_PIN_5};
GPIO_Struct b6_ = {GPIOB, GPIO_PIN_6};
GPIO_Struct b7_ = {GPIOB, GPIO_PIN_7};
GPIO_Struct b8_ = {GPIOB, GPIO_PIN_8};
GPIO_Struct b9_ = {GPIOB, GPIO_PIN_9};
GPIO_Struct b10_ = {GPIOB, GPIO_PIN_10};
GPIO_Struct b11_ = {GPIOB, GPIO_PIN_11};
GPIO_Struct b12_ = {GPIOB, GPIO_PIN_12};
GPIO_Struct b13_ = {GPIOB, GPIO_PIN_13};
GPIO_Struct b14_ = {GPIOB, GPIO_PIN_14};
GPIO_Struct b15_ = {GPIOB, GPIO_PIN_15};
GPIO_Struct c13_ = {GPIOC, GPIO_PIN_13};
GPIO_Struct c14_ = {GPIOC, GPIO_PIN_14};
GPIO_Struct c15_ = {GPIOC, GPIO_PIN_15};


uint8_t digitalPin_Read(GPIO_Type gpio){
    return HAL_GPIO_ReadPin(
            gpio->gpio,
            gpio->pin);
}
void digitalPin_Write(GPIO_Type gpio, uint8_t pin_state){
    HAL_GPIO_WritePin(
            gpio->gpio,
            gpio->pin,
            pin_state);
}

/**
 * @brief Detect KeyPress Type
 * @return 0-NoPress 1-ShortPress 2-LongPress 3-DoublePress
 */
uint8_t key_detect(){
    uint16_t t = 0;
    if (digitalPin_Read(BUTTON_PIN) != BUTTON_TRIG){
        release = 1;
        return 0;
    }
    if ((digitalPin_Read(BUTTON_PIN) == BUTTON_TRIG) && release){
        release = 0;
        HAL_Delay(10);
        // jitter
        if(digitalPin_Read(BUTTON_PIN) != BUTTON_TRIG){
            return 0;
        }
        while((digitalPin_Read(BUTTON_PIN) == BUTTON_TRIG) && (t < LONG_PRESS_TIME)){
            t++;
            HAL_Delay(1);
        }
        if(t >= LONG_PRESS_TIME){
            return 2;
        }
        t = 0;
        HAL_Delay(5);
        while(t < 200){
            // double click
            if ((digitalPin_Read(BUTTON_PIN) == BUTTON_TRIG)){
                return 3;
            }
            HAL_Delay(1);
            t++;
        }
        release = 1;
        return 1;
    }
    return 0;
}

/**
 * @brief using encoder to setting a u16(or lager) number
 * @param u16 setting number
 * @param digit setting digit (54321)
 * @param update pointer to number changed flag
 * @return new setting number
 */
uint16_t rot_change_u16(uint16_t u16, uint8_t digit, uint8_t *update){
    uint16_t target_u16 = u16;
    int16_t enc_delta = ENC_DIR * (int16_t)(__HAL_TIM_GET_COUNTER(&htim2) - ENCODER_DEFAULT);
    if (enc_delta && !(enc_delta % ENC_PRES)){

        uint32_t pow1 = (uint32_t)pow(10, digit);
        uint16_t pow2 = (uint16_t)pow(10, digit-1);
        *update = 1;
        uint8_t num = (target_u16 - (pow1 * (target_u16 / pow1))) / pow2;
        num = num + 10 + (enc_delta > 0 ? 1 : -1);
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

/**
 * @brief using encoder to setting a u8 number in min-max loop
 * @param u8 setting number
 * @param min loop minimum number
 * @param max loop maximum number
 * @return new setting number
 */
uint8_t rot_change_u8(uint8_t u8, uint8_t min, uint8_t max, uint8_t *update){
    int16_t target_u8 = u8;
    int16_t enc_delta = ENC_DIR * (int16_t)(__HAL_TIM_GET_COUNTER(&htim2) - ENCODER_DEFAULT);
    if (enc_delta && !(enc_delta % ENC_PRES)){
        *update = 1;
        target_u8 = (enc_delta > 0) ? target_u8 + 1 : target_u8 - 1;
        if (target_u8 > max) target_u8 = min;
        if (target_u8 < min) target_u8 = max;
        __HAL_TIM_SET_COUNTER(&htim2, ENCODER_DEFAULT);
    }
    return (uint8_t)target_u8;
}