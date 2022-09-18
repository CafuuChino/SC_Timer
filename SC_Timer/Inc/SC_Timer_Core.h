//
// Created by CafuuChino on 2022/4/25.
//

#ifndef SC_TIMER_SC_TIMER_CORE_H
#define SC_TIMER_SC_TIMER_CORE_H


#define FLASH_DATA_ADDR 0x08019000

#define PROFILE_NUM 8
#define OUTPUT_CHANNEL 22

#define BUTTON_PIN A2
#define TRIG_PIN A3
#define ENCODER_DEFAULT 32767
extern volatile uint16_t itr_time_count;
extern volatile uint8_t itr_exec_count;
extern volatile uint8_t cdc_RX_enable;
extern uint8_t *cdc_cmd_ptr;
extern uint16_t trig_mode;
extern uint16_t old_trig_mode;
extern uint16_t profile_data[PROFILE_NUM][OUTPUT_CHANNEL * 2];
extern uint16_t rel_disp[PROFILE_NUM][OUTPUT_CHANNEL * 2];
extern uint8_t select_prof;
extern uint8_t select_ch;

void abs2rel();
void rel2abs();
uint8_t rel_available();
uint8_t abs_available();


void main_setup();
void main_loop();
void start_running(uint8_t profile_index);

void Flash_ReadConfig();
void Flash_WriteConfig();

void OLED_DispProfile(uint8_t profile_index, uint8_t mode);
void OLED_DispChannel(uint8_t row, uint8_t profile, uint8_t channel, uint8_t mode);
uint8_t OLED_ShowU16(uint8_t x, uint8_t row, uint16_t num, uint8_t width, uint8_t reverse, uint8_t mode);
void OLED_Disp_RelErr(uint8_t index);
void OLED_Disp_AbsErr(uint8_t index);
#endif //SC_TIMER_SC_TIMER_CORE_H
