//
// Created by CafuuChino on 2022/4/25.
//

#include <stdlib.h>
#include <string.h>
#include "SC_Timer_Core.h"
#include "usbd_cdc_if.h"

volatile uint16_t itr_time_count = 0;
volatile uint8_t itr_exec_count = 0;
volatile uint8_t cdc_RX_enable = 0;
uint8_t *cdc_cmd_ptr;
uint32_t cdc_cmd_len = 0;
uint16_t trig_mode = 1;
uint16_t profile_data[PROFILE_NUM][OUTPUT_CHANNEL * 2] = {};
GPIO_TypeDef *GPIO_Bank_List[OUTPUT_CHANNEL] = {GPIOC, GPIOC, GPIOC, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB,
                                                GPIOA, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB,
                                                GPIOA, GPIOA};
uint16_t GPIO_Pin_List[OUTPUT_CHANNEL] = {GPIO_PIN_15, GPIO_PIN_14, GPIO_PIN_13, GPIO_PIN_9, GPIO_PIN_8, GPIO_PIN_7,
                                          GPIO_PIN_6, GPIO_PIN_5, GPIO_PIN_4, GPIO_PIN_3, GPIO_PIN_15, GPIO_PIN_8,
                                          GPIO_PIN_15, GPIO_PIN_14, GPIO_PIN_13, GPIO_PIN_12, GPIO_PIN_11, GPIO_PIN_10,
                                          GPIO_PIN_2, GPIO_PIN_1, GPIO_PIN_0, GPIO_PIN_6};

void SerialPrint_Profile(uint8_t profile_index);

void test_data();

void Flash_ReadConfig();

void Flash_WriteConfig();

void CommandHandler(uint8_t buf, uint16_t size);

void CH(char *s) {
    uint16_t dig;
    char *ptr;
    uint16_t cmd[4];
    uint8_t buf2[1];
    char *p = s;
    uint8_t cmd_ptr = 0;
    while (*p != '\0') {
        if (*p >= '0' && *p <= '9') {
            cmd[cmd_ptr] = strtol(p, &ptr, 10);
            cmd_ptr++;
            p = ptr;
        }
        p = p + 1;
    }

    if (*s == '$') {
        s++;
        switch (*s) {
            case '$': {
                char *info = "Solenoid Coils Timer\r\nType \"$?\" to get command info\r\n";
                CDC_Transmit_FS((uint8_t *) info, strlen(info));
                break;
            }
            case '?': {
                char *info = "Support Command:\r\n$s - set value\r\n>>$s <profile>-<channel>-<value_type(1-offset/2-time)> <value>\r\n$r - print profile info\r\n>>$r <profile>\r\n"
                             "$e - execute profile Timer\r\n>>$e <profile>\r\n$u - update setting($s will not save changes to flash until $u)\r\n>>$u\r\n";
                CDC_Transmit_FS((uint8_t *) info, strlen(info));
                break;
            }

            case 's': {
                if (cmd[0] - 1 >= 8 || cmd[1] - 1 >= 22 || cmd[2] - 1 >= 2) {
                    char *info = "ERROR: profile should in 1~8; channel should in 1~22; type should in 1~2\r\n";
                    CDC_Transmit_FS((uint8_t *) info, strlen(info));
                    break;
                }
                profile_data[cmd[0] - 1][(cmd[1] - 1) * 2 + cmd[2] - 1] = cmd[3];
                break;
            }
            case 'r': {
                if (cmd[0] - 1 >= 8) {
                    char *info = "ERROR: profile should in 1~8\r\n";
                    CDC_Transmit_FS((uint8_t *) info, strlen(info));
                    break;
                }
                SerialPrint_Profile(cmd[0] - 1);
                break;
            }
            case 'e': {
                if (cmd[0] - 1 >= 8) {
                    char *info = "ERROR: profile should in 1~8\r\n";
                    CDC_Transmit_FS((uint8_t *) info, strlen(info));

                }
                start_running(cmd[0] - 1);
                break;
            }
            case 'u': {
                Flash_WriteConfig();
                break;
            }
        }
    }
}


void main_setup() {
    //test_data();
    //Flash_WriteConfig();
    for (uint8_t i = 0; i < OUTPUT_CHANNEL; i++) {
        HAL_GPIO_WritePin(GPIO_Bank_List[i], GPIO_Pin_List[i], !trig_mode);
    }
    Flash_ReadConfig();
}

void main_loop() {
//    uint8_t buf[2] = {*(__IO uint16_t*)(FLASH_DATA_ADDR) >> 8, *(__IO uint16_t*)(FLASH_DATA_ADDR)};
//    CDC_Transmit_FS(buf,2);
    if (cdc_RX_enable) {
        CH((char *) cdc_cmd_ptr);
        cdc_RX_enable = 0;
    }
    //SerialPrint_Profile(0);
    //HAL_Delay(1000);
}


void CommandHandler(uint8_t buf, uint16_t size) {

}

void test_data() {
    for (uint8_t i = 0; i < OUTPUT_CHANNEL * 2; i++) {
        profile_data[0][i] = 33 + 1 * i;
    }
}

void start_running(uint8_t profile_index) {

    //test_data();
    __HAL_TIM_CLEAR_IT(&htim1, TIM_IT_UPDATE);
    // initialize exec time
    uint16_t exec_time_list[OUTPUT_CHANNEL * 2];
    uint16_t time_sum = 0;
    for (uint8_t i = 0; i < OUTPUT_CHANNEL * 2; i++) {
        time_sum += profile_data[profile_index][i];
        exec_time_list[i] = time_sum;
    }
    // reset output pins to reset
    for (uint8_t i = 0; i < OUTPUT_CHANNEL; i++) {
        HAL_GPIO_WritePin(GPIO_Bank_List[i], GPIO_Pin_List[i], !trig_mode);
    }
    // set the BUSY signal pin
    HAL_GPIO_WritePin(BUSY_BANK, BUSY_PIN, 1);
    itr_exec_count = 0;
    itr_time_count = 0;
    // pre-calculated cycle number
    uint8_t running_cycle = OUTPUT_CHANNEL * 2;
    // start TIM1 count and interrupt
    HAL_TIM_Base_Start_IT(&htim1);

    // main cycle
    while ((itr_exec_count < running_cycle)) {
        if (itr_time_count >= exec_time_list[itr_exec_count]) {
            HAL_GPIO_TogglePin(GPIO_Bank_List[itr_exec_count >> 1], GPIO_Pin_List[itr_exec_count >> 1]);
            itr_exec_count++;
        }
    }
    HAL_TIM_Base_Stop_IT(&htim1);
    HAL_GPIO_WritePin(BUSY_BANK, BUSY_PIN, 0);
}

void SerialPrint_Profile(uint8_t profile_index) {

    char title_buf[30] = "Profile: ";
    char newline_buf[3] = "\r\n";
    char int_buf[6];
    itoa(profile_index, int_buf, 10);
    strcat(title_buf, int_buf);
    strcat(title_buf, newline_buf);
    CDC_Transmit_FS((uint8_t *) title_buf, strlen(title_buf));
    HAL_Delay(1);
    char timer_info_buf[30] = "Ch";
    char timer_info_buf2[20] = ": Offset: ";
    char timer_info_buf3[20] = ", Time: ";
    for (uint8_t i = 0; i < OUTPUT_CHANNEL; i++) {
        char send_buf[60] = "";
        strcat(send_buf, timer_info_buf);
        itoa(i + 1, int_buf, 10);
        strcat(send_buf, int_buf);
        strcat(send_buf, timer_info_buf2);
        itoa(profile_data[profile_index][i * 2], int_buf, 10);
        strcat(send_buf, int_buf);
        strcat(send_buf, timer_info_buf3);
        itoa(profile_data[profile_index][i * 2 + 1], int_buf, 10);
        strcat(send_buf, int_buf);
        strcat(send_buf, newline_buf);
        CDC_Transmit_FS((uint8_t *) send_buf, strlen(send_buf));
        HAL_Delay(1);
    }
}


void Flash_ReadConfig() {
    trig_mode = *(__IO uint16_t *) (FLASH_DATA_ADDR);
    uint16_t offset = 0;
    for (uint8_t i = 0; i < PROFILE_NUM; i++) {
        for (uint8_t j = 0; j < OUTPUT_CHANNEL * 2; j++) {
            offset += 2;
            profile_data[i][j] = *(__IO uint16_t *) (FLASH_DATA_ADDR + offset);
        }
    }
}

void Flash_WriteConfig() {
    FLASH_EraseInitTypeDef pEraseInit;
    HAL_FLASH_Unlock();
    pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    pEraseInit.PageAddress = FLASH_DATA_ADDR;
    pEraseInit.NbPages = 1;
    uint32_t PageError = 0;
    HAL_FLASHEx_Erase(&pEraseInit, &PageError);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FLASH_DATA_ADDR, trig_mode);
    uint16_t offset = 0;
    for (uint8_t i = 0; i < PROFILE_NUM; i++) {
        for (uint8_t j = 0; j < 2 * OUTPUT_CHANNEL; j++) {
            offset += 2;
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FLASH_DATA_ADDR + offset, profile_data[i][j]);
        }
    }
    HAL_FLASH_Lock();
}