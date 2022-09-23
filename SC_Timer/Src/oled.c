//
// Created by CafuuChino on 2022/9/16.
//

#include "oled.h"
#include "oledfont.h"

uint8_t CMD_Data[] = {
        0xAE, 0x00, 0x10, 0x40, 0xB0, 0x81, 0xFF, 0xA1, 0xA6, 0xA8, 0x3F, 0xC8, 0xD3, 0x00,
        0xD5, 0xF0, 0xD8, 0x05, 0xD9, 0xF1, 0xDA, 0x12, 0xDB, 0x30, 0x8D, 0x14, 0xAF
};

void WriteCmd() {
    for (uint8_t i = 0; i < 27; i++) {
        HAL_I2C_Mem_Write(&hi2c1, 0x78, 0x00, I2C_MEMADD_SIZE_8BIT, CMD_Data + i, 1, 0x01);
    }
}

void OLED_WR_CMD(uint8_t cmd) {
    HAL_I2C_Mem_Write(&hi2c1, 0x78, 0x00, I2C_MEMADD_SIZE_8BIT, &cmd, 1, 0x01);
}

void OLED_WR_DATA(uint8_t data) {
    HAL_I2C_Mem_Write(&hi2c1, 0x78, 0x40, I2C_MEMADD_SIZE_8BIT, &data, 1, 0x01);
}


void OLED_Init(void) {
    HAL_Delay(100);
    WriteCmd();
}

void OLED_Clear() {
    uint8_t i, n;
    for (i = 0; i < 8; i++) {
        OLED_WR_CMD(0xb0 + i);
        OLED_WR_CMD(0x00);
        OLED_WR_CMD(0x10);
        for (n = 0; n < 128; n++)
            OLED_WR_DATA(0);
    }
}

void OLED_Set_Pos(uint8_t x, uint8_t y) {
    OLED_WR_CMD(0xb0 + y);
    OLED_WR_CMD(((x & 0xf0) >> 4) | 0x10);
    OLED_WR_CMD(x & 0x0f);
}

uint8_t OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t mode) {
    uint8_t c, i;
    c = chr - ' ';
    if (x > 127) {
        x = 0;
        y = y + 2;
    }
    OLED_Set_Pos(x, y);
    for (i = 0; i < 8; i++) {
        if (mode) {
            OLED_WR_DATA(~F8x16[c * 16 + i]);
        } else {
            OLED_WR_DATA(F8x16[c * 16 + i]);
        }
    }
    OLED_Set_Pos(x, y + 1);
    for (i = 0; i < 8; i++) {
        if (mode) {
            OLED_WR_DATA(~F8x16[c * 16 + i + 8]);
        } else {
            OLED_WR_DATA(F8x16[c * 16 + i + 8]);
        }
    }
    return x + 8;
}

uint8_t OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t mode) {
    uint8_t j = 0;
    while (str[j] != '\0') {
        OLED_ShowChar(x, y, str[j], mode);
        x += 8;
        if (x > 120) {
            x = 0;
            y += 2;
        }
        j++;
    }
    return x;
}
