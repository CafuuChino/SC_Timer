#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-interfaces-global-init"
//
// Created by CafuuChino on 2022/4/25.
//
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "oled.h"
#include "SC_Timer_Core.h"
#include "state_machine.h"
#include "usbd_cdc_if.h"
#include "gpio_input.h"

#define BUSY_PIN A4
#define BUSY_OUTPUT 0

volatile uint16_t itr_time_count = 0;
volatile uint8_t itr_exec_count = 0;
volatile uint8_t cdc_RX_enable = 0;

extern uint8_t display_mode; // OLED display mode 0-Absolute; 1-Relative
extern uint8_t statemachine_update;

extern USBD_HandleTypeDef hUsbDeviceFS;

uint8_t usb_status = 0;
uint8_t *cdc_cmd_ptr;

uint16_t trig_mode = 1;
uint16_t old_trig_mode;
uint16_t profile_data[PROFILE_NUM][OUTPUT_CHANNEL * 2];
uint16_t rel_disp[PROFILE_NUM][OUTPUT_CHANNEL * 2] = {};

uint8_t select_prof = 1;
uint8_t select_ch = 1;

uint8_t cnt = 0;


GPIO_TypeDef *Output_Type[OUTPUT_CHANNEL] = {
        GPIOC,
        GPIOC,
        GPIOC,
        GPIOB,
        GPIOB,
        GPIOB,
        GPIOB,
        GPIOB,
        GPIOA,
        GPIOB,
        GPIOB,
        GPIOB,
        GPIOB,
        GPIOB,
        GPIOB,
        GPIOB,
        GPIOB,
        GPIOB,
        GPIOA,
        GPIOA,
        GPIOA,
        GPIOA,
        GPIOA,
        GPIOA
};
uint16_t Output_Pin[OUTPUT_CHANNEL] = {
        GPIO_PIN_15,
        GPIO_PIN_14,
        GPIO_PIN_13,
        GPIO_PIN_7,
        GPIO_PIN_6,
        GPIO_PIN_5,
        GPIO_PIN_4,
        GPIO_PIN_3,
        GPIO_PIN_15,
        GPIO_PIN_8,
        GPIO_PIN_15,
        GPIO_PIN_14,
        GPIO_PIN_13,
        GPIO_PIN_12,
        GPIO_PIN_11,
        GPIO_PIN_10,
        GPIO_PIN_2,
        GPIO_PIN_1,
        GPIO_PIN_0,
        GPIO_PIN_7,
        GPIO_PIN_6,
        GPIO_PIN_5,
        GPIO_PIN_9,
        GPIO_PIN_10
};

void CDC_Print_Profile(uint8_t profile_index, uint8_t mode);
void init_data();

/**
 * @brief USB CDC String Command Handler Callback
 * @param s CDC Command String
 */
void CDC_Command_Handler(char *s) {
    char *ptr;
    uint16_t cmd[4];
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
                char *info = "Solenoid Coils Timer by CafuuChino\r\nType \"$?\" to get command info\r\n";
                CDC_Transmit_FS((uint8_t *) info, strlen(info));
                break;
            }
            case '?': {
                char *info = "Support Command:\r\n"
                             "-----Setting-----\r\n"
                             "$t - set trigger level (Immediately Change&Save)\r\n>>$t <0-Low/1-High>\r\n"
                             "$sa - set value in Absolute Time Mode\r\n"
                             ">> $ss <profile> <channel> <value_type(1-begin_time/2-duration)> <value>\r\n"
                             "$sr - set value in Absolute Time Mode\r\n"
                             ">> $sr <profile> <channel> <value_type(1-begin_time/2-duration)> <value>\r\n"
                             "-----Loading-----\r\n"
                             "$ra - print profile info in Absolute Time\r\n"
                             ">>$ra <profile>\r\n"
                             "$rr - print profile info in Relative Offset\r\n>"
                             ">$rr <profile>\r\n"
                             "-----Executing-----\r\n"
                             "$e - execute profile Timer\r\n>>$e <profile>\r\n"
                             "$u - update setting($s will not save changes to flash until $u)\r\n>>$u\r\n";
                CDC_Transmit_FS((uint8_t *) info, strlen(info));
                break;
            }

            case 's': {
                if (cmd[0] - 1 >= 8 || cmd[1] - 1 >= OUTPUT_CHANNEL || cmd[2] - 1 >= 2) {
                    char *info = "ERROR: profile should in 1~8; channel should in 1~24; value type should in 1~2\r\n";
                    CDC_Transmit_FS((uint8_t *) info, strlen(info));
                    break;
                }
                if (*(s+1) == 'r'){
                    rel_disp[cmd[0] - 1][(cmd[1] - 1) * 2 + cmd[2] - 1] = cmd[3];
                    uint8_t ret = rel_available();
                    if (ret){
                        uint8_t err_ch, err_time;
                        err_ch = (ret - 1) / 2 + 1;
                        err_time = ret % 2;
                        char send_buffer[50] = "Channel";
                        char digit_buffer[5];
                        itoa(err_ch, digit_buffer, 10);
                        strcat(send_buffer, digit_buffer);
                        strcat(send_buffer, err_time?"Begin":"Duration");
                        strcat(send_buffer, "Time Error");
                        CDC_Transmit_FS((uint8_t*)send_buffer, strlen(send_buffer));
                        return;
                    }
                    rel2abs();
                }
                else if(*(s+1) == 'a'){
                    profile_data[cmd[0] - 1][(cmd[1] - 1) * 2 + cmd[2] - 1] = cmd[3];
                    uint8_t ret = abs_available();
                    if (ret) {
                        uint8_t err_ch, err_time;
                        err_ch = (ret - 1) / 2 + 1;
                        err_time = ret % 2;
                        char send_buffer[50] = "Channel";
                        char digit_buffer[5];
                        itoa(err_ch, digit_buffer, 10);
                        strcat(send_buffer, digit_buffer);
                        strcat(send_buffer, err_time ? "Begin" : "Duration");
                        strcat(send_buffer, "Time Error");
                        CDC_Transmit_FS((uint8_t *) send_buffer, strlen(send_buffer));
                        return;
                    }
                    abs2rel();
                }
                else{
                    CDC_Transmit_FS((uint8_t *)"Please using $sa or $sr", 24);
                }
                statemachine_update = 1;
                char *info = "Set Success!\r\n";
                CDC_Transmit_FS((uint8_t *) info, strlen(info));
                break;
            }
            case 'r': {
                if (cmd[0] - 1 >= 8) {
                    char *info = "ERROR: profile should in 1~8\r\n";
                    CDC_Transmit_FS((uint8_t *) info, strlen(info));
                    break;
                }
                if (*(s+1) == 'r'){
                    CDC_Print_Profile(cmd[0] - 1, 1);
                }
                else{
                    CDC_Print_Profile(cmd[0] - 1, 0);
                }
                break;
            }
            case 'e': {
                if (cmd[0] - 1 >= PROFILE_NUM) {
                    char *info = "ERROR: profile should in 1~8\r\n";
                    CDC_Transmit_FS((uint8_t *) info, strlen(info));
                    break;
                }
                start_running(cmd[0] - 1);
                char *info = "Executed Success!\r\n";
                CDC_Transmit_FS((uint8_t *) info, strlen(info));
                break;
            }
            case 'u': {
                Flash_WriteConfig();
                char *info = "Save Config Success!\r\n";
                CDC_Transmit_FS((uint8_t *) info, strlen(info));
                statemachine_update = 1;
                break;
            }
            case 't':{
                if (cmd[0] > 1) {
                    char *info = "ERROR: Trigger Level should in 0~1\r\n";
                    CDC_Transmit_FS((uint8_t *) info, strlen(info));
                }
                trig_mode = cmd[0];
                Flash_WriteConfig();
                char *info = "Setting Trig Level Success!\r\n";
                CDC_Transmit_FS((uint8_t *) info, strlen(info));
                statemachine_update = 1;
            }
            default:{
                char *info = "Solenoid Coils Timer\r\nType \"$?\" to get command info\r\n";
                CDC_Transmit_FS((uint8_t *) info, strlen(info));
                break;
            }
        }
    }
}

/**
 * @brief Update relative time profile by absolute time profile
 */
void abs2rel(){
    for (uint8_t j = 0; j < PROFILE_NUM; j++){
        rel_disp[j][0] = profile_data[j][0];
        for (uint8_t i = 1; i < 2*OUTPUT_CHANNEL; i++){
            rel_disp[j][i] = profile_data[j][i] - profile_data[j][i - 1];
        }
    }
}

/**
 * @brief Update absolute time profile by relative time profile
 */
void rel2abs(){
    for (uint8_t j = 0; j < PROFILE_NUM; j++){
        profile_data[j][0] = rel_disp[j][0];
        uint16_t sum = rel_disp[j][0];
        for (uint8_t i = 1; i < 2*OUTPUT_CHANNEL; i++) {
            sum += rel_disp[j][i];
            profile_data[j][i] = sum;
        }
    }
}

/**
 * @brief check relative time profile if has error_setting
 * @return 0 for success, other for error index
 */
uint8_t rel_available(){
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 2*OUTPUT_CHANNEL; i++){
        sum += rel_disp[select_prof-1][i];
        if (sum > 65535){
            return i+1;
        }
    }
    return 0;
}
/**
 * @brief check absolute time profile if has error_setting
 * @return 0 for success, other for error index
 */
uint8_t abs_available(){
    for (uint8_t i = 0; i < (uint8_t)(2*OUTPUT_CHANNEL-1); i++) {
        if (profile_data[select_prof-1][i] > profile_data[select_prof-1][i+1]) return i+1;
    }
    return 0;
}

/**
 * @brief Timer setup entrance
 */
void main_setup() {
    OLED_Init();
    OLED_Clear();
    if (*(__IO uint16_t *) (FLASH_DATA_ADDR) != 0xCC){
        OLED_ShowString(4,3,"From Reset Init",0);
        HAL_Delay(1500);
        init_data();
        Flash_WriteConfig();
    }
    else{
        Flash_ReadConfig();
    }
    old_trig_mode = trig_mode;

    for (uint8_t i = 0; i < OUTPUT_CHANNEL; i++){
        HAL_GPIO_WritePin(Output_Type[i], Output_Pin[i], !trig_mode);
    }
    digitalPin_Write(BUTTON_PIN, !BUTTON_TRIG);
    digitalPin_Write(BUSY_PIN, !BUSY_OUTPUT);
    digitalPin_Write(A0, 0);
    digitalPin_Write(A1, 0);
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
    __HAL_TIM_SET_COUNTER(&htim2, ENCODER_DEFAULT);


}

/**
 * @brief Timer loop entrance
 */
void main_loop() {

    if ((hUsbDeviceFS.dev_state==USBD_STATE_CONFIGURED) != usb_status){
        usb_status = (hUsbDeviceFS.dev_state==USBD_STATE_CONFIGURED);
        OLED_ShowChar(104, 0, usb_status?'U':' ', 0);
    }
    statemachine();
    if (cdc_RX_enable) {
        CDC_Command_Handler((char *) cdc_cmd_ptr);
        cdc_RX_enable = 0;
    }
    HAL_Delay(1);
}

/**
 * @brief Summon default data form reset
 */
void init_data() {
    for (int j = 0; j < PROFILE_NUM; j++)
    for (uint8_t i = 0; i < OUTPUT_CHANNEL * 2; i++) {
        profile_data[j][i] = 33 + (j+1) * i;
    }
}

/**
 * @brief Rewrite ST HAL_Delay to support 1ms delay
 * @param Delay delay time(support 1ms)
 */
void HAL_Delay(uint32_t Delay){
    uint32_t tickstart = HAL_GetTick();
    uint32_t wait = Delay;
    while ((HAL_GetTick() - tickstart) < wait){}
}

/**
 * @brief Timer Main Function
 * @param profile_index
 */
void start_running(uint8_t profile_index) {
    __HAL_TIM_CLEAR_IT(&htim1, TIM_IT_UPDATE);
    // initialize exec time list
    volatile uint16_t exec_time_list[OUTPUT_CHANNEL * 2];
    for (uint8_t i = 0; i < OUTPUT_CHANNEL * 2; i++) {
        exec_time_list[i] = profile_data[profile_index-1][i];
    }
    // reset output pins to reset
    for (uint8_t i = 0; i < OUTPUT_CHANNEL; i++) {
        HAL_GPIO_WritePin(Output_Type[i], Output_Pin[i], !trig_mode);
    }
    // set the BUSY signal pin
    itr_exec_count = 0;
    itr_time_count = 0;
    // pre-calculated cycle number
    // start TIM1 count and interrupt
    digitalPin_Write(BUSY_PIN, BUSY_OUTPUT);
    HAL_TIM_Base_Start_IT(&htim1);
    // main cycle
    while ((itr_exec_count < OUTPUT_CHANNEL * 2)) {
        if (itr_time_count >= exec_time_list[itr_exec_count]) {
            HAL_GPIO_TogglePin(Output_Type[itr_exec_count >> 1], Output_Pin[itr_exec_count >> 1]);
            itr_exec_count++;
        }
    }
//    HAL_TIM_Base_Stop_IT(&htim1);
//    OLED_ShowU16(0,0,exec_time_rec[0],5,0,0);
//    OLED_ShowU16(0,2,exec_time_rec[1],5,0,0);
//    OLED_ShowU16(0,4,exec_time_rec[2],5,0,0);
//    OLED_ShowU16(0,6,exec_time_rec[3],5,0,0);
    digitalPin_Write(BUSY_PIN, !BUSY_OUTPUT);


}

/**
 *
 * @param profile_index profile need to be print
 * @param mode 0-absolute time; 1-relative time
 */
void CDC_Print_Profile(uint8_t profile_index, uint8_t mode) {
    Flash_ReadConfig();
    if (mode) abs2rel();
    char title_buf[40] = "Profile: ";
    char newline_buf[3] = "\r\n";
    char int_buf[6];
    itoa(profile_index+1, int_buf, 10);
    strcat(title_buf, int_buf);
    strcat(title_buf, newline_buf);
    strcat(title_buf, "Display Mode: ");
    strcat(title_buf, mode?"Relative Offset\r\n":"Absolute Time\r\n");
    CDC_Transmit_FS((uint8_t *)title_buf, strlen(title_buf));
    HAL_Delay(1);
    char trig_buf[30] = "Trigger Mode: ";
    strcat(trig_buf, trig_mode?"High Level\r\n":"Low Level\r\n");
    CDC_Transmit_FS((uint8_t *)trig_buf, strlen(trig_buf));
    HAL_Delay(1);
    char timer_info_buf[30] = "Ch";
    char timer_info_buf2[20] = ": Begin: ";
    char timer_info_buf3[20] = "us, Duration: ";
    for (uint8_t i = 0; i < OUTPUT_CHANNEL; i++) {
        char send_buf[60] = "";
        strcat(send_buf, timer_info_buf);
        itoa(i + 1, int_buf, 10);
        if (i < 9) strcat(send_buf, "0");
        strcat(send_buf, int_buf);
        strcat(send_buf, timer_info_buf2);
        if (mode){ // rel
            itoa(rel_disp[profile_index][i * 2], int_buf, 10);
            strcat(send_buf, int_buf);
            strcat(send_buf, timer_info_buf3);
            itoa(rel_disp[profile_index][i * 2 + 1], int_buf, 10);
        }
        else{
            itoa(profile_data[profile_index][i * 2], int_buf, 10);
            strcat(send_buf, int_buf);
            strcat(send_buf, timer_info_buf3);
            itoa(profile_data[profile_index][i * 2 + 1], int_buf, 10);
        }
        strcat(send_buf, int_buf);
        strcat(send_buf, "us");
        strcat(send_buf, newline_buf);
        CDC_Transmit_FS((uint8_t *) send_buf, strlen(send_buf));
        HAL_Delay(1);
    }
}


void Flash_ReadConfig() {
    uint16_t offset = 2;
    trig_mode = *(__IO uint16_t *) (FLASH_DATA_ADDR + offset);
    for (uint8_t i = 0; i < PROFILE_NUM; i++) {
        for (uint8_t j = 0; j < OUTPUT_CHANNEL * 2; j++) {
            offset += 2;
            profile_data[i][j] = *(__IO uint16_t *) (FLASH_DATA_ADDR + offset);
        }
    }
    abs2rel();
}

void Flash_WriteConfig() {
    OLED_ShowString(0,3,"Saving Config...",0);
    FLASH_EraseInitTypeDef pEraseInit;
    HAL_FLASH_Unlock();
    pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    pEraseInit.PageAddress = FLASH_DATA_ADDR;
    pEraseInit.NbPages = 1;
    uint32_t PageError = 0;
    HAL_FLASHEx_Erase(&pEraseInit, &PageError);
    uint16_t offset = 2;
    // reset flag is 0xCC at the first half word
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FLASH_DATA_ADDR, 0xCC);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FLASH_DATA_ADDR + offset, trig_mode);

    for (uint8_t i = 0; i < PROFILE_NUM; i++) {
        for (uint8_t j = 0; j < 2 * OUTPUT_CHANNEL; j++) {
            offset += 2;
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FLASH_DATA_ADDR + offset, profile_data[i][j]);
        }
    }
    HAL_FLASH_Lock();
    statemachine_update = 1;
}
/**
 *
 * @param x OLED Display X_Pos
 * @param row OLED Display Y_Row(8 pix/row)
 * @param num display number
 * @param width display width
 * @param reverse 0-default 1~5-reverse_set_digit(54321) 0xFF-reverse_all
 * @param mode 0-hide_invalid_0 1-display_invalid_0
 * @return Next display x_pos
 */
uint8_t OLED_ShowU16(uint8_t x, uint8_t row, uint16_t num, uint8_t width, uint8_t reverse, uint8_t mode){
    uint8_t x_pos = x;
    uint8_t digit;
    uint16_t origin_digit = num;
    uint8_t begin_ava = 0;
    for (uint8_t i = width; i > 0; i--){
        digit = origin_digit / (uint16_t)pow(10, i-1);

        origin_digit -= digit*(uint16_t)pow(10, i-1);
        // ava 0
        if (begin_ava){
            x_pos = OLED_ShowChar(x_pos, row, '0' + digit, (reverse == i || reverse == 0xFF));
        }
        else{
            if (digit){
                begin_ava = 1;
                x_pos = OLED_ShowChar(x_pos, row, '0' + digit, (reverse == i || reverse == 0xFF));
            }
            else{
                if (!mode && i>1){
                    x_pos = OLED_ShowChar(x_pos, row, ' ', (reverse == i || reverse == 0xFF));
                }
                else{
                    x_pos = OLED_ShowChar(x_pos, row, '0', (reverse == i || reverse == 0xFF));
                }

            }
        }
    }
    return x_pos;
}

void OLED_DispProfile(uint8_t profile_index, uint8_t mode){
    uint8_t x_pos = 0;
    x_pos = OLED_ShowString(x_pos,0,"Profile ",mode);
    x_pos = OLED_ShowChar(x_pos,0,'0'+profile_index,mode);
    x_pos = OLED_ShowChar(x_pos,0,'/',mode);
    x_pos = OLED_ShowChar(x_pos,0,'0'+PROFILE_NUM,mode);
    OLED_ShowChar(x_pos, 0, ' ', 0);
    OLED_ShowChar(112, 0, display_mode?'R':'A',0);
    OLED_ShowChar(120, 0, trig_mode?'H':'L', 0);
}

/**
 *
 * @param row 1-3
 * @param channel 1-OUTPUT_CHANNEL
 * @param mode 0-default 1-all_reverse 2-time1_reverse 3-time2_reverse
 */
void OLED_DispChannel(uint8_t row, uint8_t profile, uint8_t channel, uint8_t mode){
    uint8_t x_pos = 0;
    x_pos = OLED_ShowString(x_pos, row*2, "Ch",(mode == 1));
    x_pos = OLED_ShowChar(x_pos, row*2, '0' + (channel / 10), (mode == 1));
    x_pos = OLED_ShowChar(x_pos, row*2, '0' + (channel % 10), (mode == 1));
    x_pos = OLED_ShowChar(x_pos, row*2, '|', (mode == 1));
    x_pos = OLED_ShowU16(x_pos,
                         row*2,
                         display_mode ? rel_disp[profile-1][(channel-1)*2] : profile_data[profile-1][(channel-1)*2],
                         5,
                         (0xFF * (mode == 1 || mode == 2)),
                         0);
    x_pos = OLED_ShowChar(x_pos, row*2, '|', (mode == 1));
    OLED_ShowU16(x_pos,
                 row*2,
                 display_mode ? rel_disp[profile-1][(channel-1)*2+1] : profile_data[profile-1][(channel-1)*2+1],
                 5,
                 (0xFF * (mode == 1 || mode == 3)),
                 0);
}

void OLED_Disp_RelErr(uint8_t index){
    uint8_t x_pos = 0;
    uint8_t err_ch, err_time;
    err_ch = (index - 1) / 2 + 1;
    err_time = index % 2;
    x_pos = OLED_ShowString(x_pos, 2, "Err: Ch-", 1);
    x_pos = OLED_ShowChar(x_pos, 2, '0' + (err_ch / 10), 1);
    x_pos = OLED_ShowChar(x_pos, 2, '0' + (err_ch % 10), 1);
    x_pos = OLED_ShowChar(x_pos, 2, ' ', 1); //9
    OLED_ShowString(x_pos, 2, err_time?"Begin":"End  ", 1);
    x_pos = 0;
    OLED_ShowString(x_pos, 4, "Larger than lim ", 1);
}

void OLED_Disp_AbsErr(uint8_t index){
    uint8_t x_pos = 0;
    uint8_t err_ch, err_time;
    err_ch = (index - 1) / 2 + 1;
    err_time = index % 2;
    x_pos = OLED_ShowString(x_pos, 2, "Err: Ch-", 1);
    x_pos = OLED_ShowChar(x_pos, 2, '0' + (err_ch / 10), 1);
    x_pos = OLED_ShowChar(x_pos, 2, '0' + (err_ch % 10), 1);
    x_pos = OLED_ShowChar(x_pos, 2, ' ', 1); //9
    OLED_ShowString(x_pos, 2, err_time?"Begin":"End  ", 1);
    x_pos = 0;
    OLED_ShowString(x_pos, 4, "Ahead of prev   ", 1);
}

