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

volatile uint16_t itr_time_count = 0;
volatile uint8_t itr_exec_count = 0;
volatile uint8_t cdc_RX_enable = 0;

extern uint8_t display_mode;
extern uint8_t statemachine_update;

uint8_t *cdc_cmd_ptr;
//uint8_t cdc_cmd_len;
uint16_t trig_mode = 1;
uint16_t profile_data[PROFILE_NUM][OUTPUT_CHANNEL * 2] = {};
uint16_t rel_disp[PROFILE_NUM][OUTPUT_CHANNEL * 2] = {};



uint8_t select_prof = 1;
uint8_t select_ch = 1;

uint8_t cnt = 0;
#define BUSY_PIN "A4"
#define TRIG_PIN "A3"


char* Output_GPIO_List[OUTPUT_CHANNEL] = {
        "C15","C14","C13","B7","B6","B5","B4","B3","A15","B8","B15",
        "B14","B13","B12","B11","B10","B2","B1","A0","A7","A6","A5"
};

void start_running(uint8_t profile_index);
void CDC_Print_Profile(uint8_t profile_index, uint8_t mode);
void test_data();

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
                char *info = "Solenoid Coils Timer\r\nType \"$?\" to get command info\r\n";
                CDC_Transmit_FS((uint8_t *) info, strlen(info));
                break;
            }
            case '?': {
                char *info = "Support Command:\r\n"
                             "$t - set trigger mode (Immediately Change&Save)\r\n>>$t <0-Low/1-High>\r\n"
                             "$ss - set value in Abs Time Mode\r\n"
                             ">>$ss <profile> <channel> <value_type(1-begin_time/2-duration)> <value>\r\n"
                             ">>$sr <profile> <channel> <value_type(1-begin_time/2-duration)> <value>\r\n"
                             "$rr - print profile info in Relative Offset\r\n>>$rr <profile>\r\n"
                             "$ra - print profile info in Absolute Time\r\n>>$ra <profile>\r\n"
                             "$e - execute profile Timer\r\n>>$e <profile>\r\n"
                             "$u - update setting($s will not save changes to flash until $u)\r\n>>$u\r\n";
                CDC_Transmit_FS((uint8_t *) info, strlen(info));
                break;
            }

            case 's': {
                if (cmd[0] - 1 >= 8 || cmd[1] - 1 >= 22 || cmd[2] - 1 >= 2) {
                    char *info = "ERROR: profile should in 1~8; channel should in 1~22; value type should in 1~2\r\n";
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
                else if(*(s+1) == 's'){
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
                    CDC_Transmit_FS((uint8_t *)"Please choose $ss or $sr", 24);
                }
                statemachine_update = 1;
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
                if (cmd[0] - 1 >= 8) {
                    char *info = "ERROR: profile should in 1~8\r\n";
                    CDC_Transmit_FS((uint8_t *) info, strlen(info));
                    break;
                }
                start_running(cmd[0] - 1);
                break;
            }
            case 'u': {
                Flash_WriteConfig();
                statemachine_update = 1;
                break;
            }
            case 't':{
                if (cmd[0] > 1) {
                    char *info = "ERROR: Trigger Mode should in 0~1\r\n";
                    CDC_Transmit_FS((uint8_t *) info, strlen(info));
                }
                trig_mode = cmd[0];
                Flash_WriteConfig();
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

void abs2rel(){
    for (uint8_t j = 0; j < PROFILE_NUM; j++){
        rel_disp[j][0] = profile_data[j][0];
        for (uint8_t i = 1; i < 2*OUTPUT_CHANNEL; i++){
            rel_disp[j][i] = profile_data[j][i] - profile_data[j][i - 1];
        }
    }
}

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

uint8_t abs_available(){
    for (uint8_t i = 0; i < (uint8_t)(2*OUTPUT_CHANNEL-1); i++) {
        if (profile_data[select_prof-1][i] > profile_data[select_prof-1][i+1]) return i+1;
    }
    return 0;
}
void main_setup() {
    Flash_ReadConfig();
    //test_data();
    //Flash_WriteConfig();
    OLED_Init();
    OLED_Clear();
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);

    __HAL_TIM_SET_COUNTER(&htim2, ENCODER_DEFAULT);
}

void main_loop() {

    statemachine();
    if (cdc_RX_enable) {
        CDC_Command_Handler((char *) cdc_cmd_ptr);
        cdc_RX_enable = 0;
    }

}



void test_data() {
    for (int j = 0; j < PROFILE_NUM; j++)
    for (uint8_t i = 0; i < OUTPUT_CHANNEL * 2; i++) {
        profile_data[j][i] = 33 + (j+1) * i;
    }

}





void HAL_Delay(uint32_t Delay){
    uint32_t tickstart = HAL_GetTick();
    uint32_t wait = Delay;

    while ((HAL_GetTick() - tickstart) < wait)
    {
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
        digitalPin_Write(Output_GPIO_List[i], !trig_mode);
    }
    // set the BUSY signal pin

    itr_exec_count = 0;
    itr_time_count = 0;
    // pre-calculated cycle number
    uint8_t running_cycle = OUTPUT_CHANNEL * 2;
    // start TIM1 count and interrupt
    digitalPin_Write(BUSY_PIN, 1);
    HAL_TIM_Base_Start_IT(&htim1);

    // main cycle
    while ((itr_exec_count < running_cycle)) {
        if (itr_time_count >= exec_time_list[itr_exec_count]) {
            digitalPin_Toggle(Output_GPIO_List[itr_exec_count >> 1]);
            itr_exec_count++;
        }
    }
    HAL_TIM_Base_Stop_IT(&htim1);
    digitalPin_Write(BUSY_PIN, 0);
}
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
    trig_mode = *(__IO uint16_t *) (FLASH_DATA_ADDR);
    uint16_t offset = 0;
    for (uint8_t i = 0; i < PROFILE_NUM; i++) {
        for (uint8_t j = 0; j < OUTPUT_CHANNEL * 2; j++) {
            offset += 2;
            profile_data[i][j] = *(__IO uint16_t *) (FLASH_DATA_ADDR + offset);
        }
    }
    abs2rel();
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