//
// Created by CafuuChino on 2022/9/17.
//

#include "state_machine.h"
#include "main.h"
#include "SC_Timer_Core.h"
#include "gpio_input.h"

extern

void state_default();
void state_highlight_profile();
void state_highlight_channel();
void state_highlight_time1();
void state_highlight_time2();


uint8_t display_mode = DISPLAY_MODE_ABS;
uint8_t statemachine_state = STATEMACHINE_DEFAULT;
uint8_t statemachine_update = 1;
uint8_t rot_update = 0;



void statemachine(){
    switch (statemachine_state){
        case STATEMACHINE_DEFAULT:{
            state_default();
            break;
        }
        case STATEMACHINE_HL_PROFILE:{
            state_highlight_profile();
            break;
        }
        case STATEMACHINE_HL_CH:{
            state_highlight_channel();
            break;
        }
        case STATEMACHINE_HL_TIME1:{
            state_highlight_time1();
            break;
        }
        case STATEMACHINE_HL_TIME2:{
            state_highlight_time2();
            break;
        }
        default: break;
    }

}

void state_default(){
    if (statemachine_update){
        statemachine_update = 0;
        OLED_DispProfile(select_prof, 0);
        if (select_ch > OUTPUT_CHANNEL-2) select_ch = OUTPUT_CHANNEL - 2;
        OLED_DispChannel(1, select_prof, select_ch, 0);
        OLED_DispChannel(2, select_prof, select_ch+1, 0);
        OLED_DispChannel(3, select_prof, select_ch+2, 0);
    }
    select_ch = rot_change_u8(select_ch, 1, OUTPUT_CHANNEL, &statemachine_update);
    uint8_t key = key_detect();
    if (key == 1){
        statemachine_update = 1;
        statemachine_state = STATEMACHINE_HL_PROFILE;
        return;
    }
    else if(key == 2){
        statemachine_update = 1;
        statemachine_state = STATEMACHINE_HL_CH;
        return;
    }
    else if(key == 3){
        statemachine_update = 1;
        display_mode = !display_mode;
        if (display_mode) abs2rel();
        else rel2abs();
    }

}

void state_highlight_profile(){
    if (statemachine_update){
        statemachine_update = 0;
        OLED_DispProfile(select_prof, 1);
        if (select_ch > OUTPUT_CHANNEL-2) select_ch = OUTPUT_CHANNEL - 2;
        OLED_DispChannel(1, select_prof, select_ch, 0);
        OLED_DispChannel(2, select_prof, select_ch+1, 0);
        OLED_DispChannel(3, select_prof, select_ch+2, 0);
    }
    select_prof = rot_change_u8(select_prof, 1, PROFILE_NUM, &statemachine_update);
    if (statemachine_update) select_ch = 1;
    uint8_t key = key_detect();
    if (key == 1){
        statemachine_update = 1;
        statemachine_state = STATEMACHINE_DEFAULT;
        return;
    }
    else if(key == 3){
        statemachine_update = 1;
        display_mode = !display_mode;
        if (display_mode) abs2rel();
        else rel2abs();
    }

}

void state_highlight_channel(){
    if (statemachine_update){
        statemachine_update = 0;
        OLED_DispProfile(select_prof, 0);
        if (select_ch <= OUTPUT_CHANNEL-2){
            OLED_DispChannel(1, select_prof, select_ch, 1);
            OLED_DispChannel(2, select_prof, select_ch+1, 0);
            OLED_DispChannel(3, select_prof, select_ch+2, 0);
        }
        else{
            OLED_DispChannel(1, select_prof, OUTPUT_CHANNEL-2, 0);
            OLED_DispChannel(2, select_prof, OUTPUT_CHANNEL-1, select_ch < OUTPUT_CHANNEL);
            OLED_DispChannel(3, select_prof, OUTPUT_CHANNEL, select_ch == OUTPUT_CHANNEL);
        }
    }
    select_ch = rot_change_u8(select_ch, 1, OUTPUT_CHANNEL, &statemachine_update);
    uint8_t key = key_detect();
    if (key == 1){
        statemachine_update = 1;
        statemachine_state = STATEMACHINE_HL_TIME1;
        return;
    }
    else if (key == 2){
        statemachine_update = 1;




        statemachine_state = STATEMACHINE_DEFAULT;
        return;
    }

}

void state_highlight_time1(){
    if (statemachine_update){
        statemachine_update = 0;
        OLED_DispProfile(select_prof, 0);
        if (select_ch <= OUTPUT_CHANNEL-2){
            OLED_DispChannel(1, select_prof, select_ch, 2);
            OLED_DispChannel(2, select_prof, select_ch+1, 0);
            OLED_DispChannel(3, select_prof, select_ch+2, 0);
        }
        else{
            OLED_DispChannel(1, select_prof, OUTPUT_CHANNEL-2, 0);
            OLED_DispChannel(2, select_prof, OUTPUT_CHANNEL-1, 2*(select_ch < OUTPUT_CHANNEL));
            OLED_DispChannel(3, select_prof, OUTPUT_CHANNEL, 2*(select_ch == OUTPUT_CHANNEL));
        }
    }
    uint8_t key = key_detect();
    if (key == 1){
        statemachine_update = 1;
        // temp save
        statemachine_state = STATEMACHINE_HL_TIME2;
        return;
    }
    else if (key == 2) {
        rot_update = 1;
        uint8_t target_row;
        uint8_t flag = 1, key_inner;
        if (select_ch <= OUTPUT_CHANNEL-2) target_row = 2;
        else if(select_ch == OUTPUT_CHANNEL-1) target_row = 4;
        else target_row = 6;
        while (flag){
            for (uint8_t i = 5; i > 0;) {
                while (1) {
                    if (rot_update) {
                        rot_update = 0;
                        OLED_ShowU16(40, target_row, profile_data[select_prof-1][(select_ch-1)*2],
                                     5,
                                     i,
                                     1);
                    }
                    profile_data[select_prof-1][(select_ch-1)*2] = rot_change_u16(profile_data[select_prof-1][(select_ch-1)*2],
                                                                            i,
                                                                            &rot_update);
                    key_inner = key_detect();
                    if (key_inner == 1) {
                        rot_update = 1;
                        i--;
                        break;
                    }
                    else if(key_inner == 2){
                        statemachine_update = 1;
                        i = 0;
                        flag = 0;
                        break;
                    }
                }
            }
        }

    }

}

void state_highlight_time2(){
    if (statemachine_update){
        statemachine_update = 0;
        OLED_DispProfile(select_prof, 0);
        if (select_ch <= OUTPUT_CHANNEL-2){
            OLED_DispChannel(1, select_prof, select_ch, 3);
            OLED_DispChannel(2, select_prof, select_ch+1, 0);
            OLED_DispChannel(3, select_prof, select_ch+2, 0);
        }
        else{
            OLED_DispChannel(1, select_prof, OUTPUT_CHANNEL-2, 0);
            OLED_DispChannel(2, select_prof, OUTPUT_CHANNEL-1, 3*(select_ch < OUTPUT_CHANNEL));
            OLED_DispChannel(3, select_prof, OUTPUT_CHANNEL, 3*(select_ch == OUTPUT_CHANNEL));
        }
    }
    uint8_t key = key_detect();
    if (key == 1){
        statemachine_update = 1;
        // temp save
        statemachine_state = STATEMACHINE_HL_CH;
    }
    else if (key == 2) {
        rot_update = 1;
        uint8_t target_row;
        uint8_t flag = 1, key_inner;
        if (select_ch <= OUTPUT_CHANNEL - 2) target_row = 2;
        else if (select_ch == OUTPUT_CHANNEL - 1) target_row = 4;
        else target_row = 6;
        while (flag) {
            for (uint8_t i = 5; i > 0;) {
                while (1) {
                    if (rot_update) {
                        rot_update = 0;
                        OLED_ShowU16(88, target_row, profile_data[select_prof - 1][(select_ch - 1) * 2 + 1],
                                     5,
                                     i,
                                     1);
                    }
                    profile_data[select_prof - 1][(select_ch - 1) * 2 + 1] = rot_change_u16(
                            profile_data[select_prof - 1][(select_ch - 1) * 2 + 1],
                            i,
                            &rot_update);
                    key_inner = key_detect();
                    if (key_inner == 1) {
                        rot_update = 1;
                        i--;
                        break;
                    } else if (key_inner == 2) {
                        statemachine_update = 1;
                        i = 0;
                        flag = 0;
                        break;
                    }
                }
            }
        }
    }
}