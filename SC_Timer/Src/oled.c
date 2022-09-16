//
// Created by CafuuChino on 2022/9/16.
//

#include "oled.h"
#include "oledfont.h"

//几个变量声明
//uint8_t **Hzk;

//初始化命令
uint8_t CMD_Data[]=
        {
        0xAE, //display off
        0x20, //Set Memory Addressing Mode
        0x10, //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
        0xb0, //Set Page Start Address for Page Addressing Mode,0-7
        0xc8, //Set COM Output Scan Direction
        0x00, //---set low column address
        0x10, //---set high column address
        0x40, //--set start line address
        0x81, //--set contrast control register
        0xff, //亮度调节 0x00~0xff
        0xa1, //--set segment re-map 0 to 127
        0xa6, //--set normal display
        0xa8, //--set multiplex ratio(1 to 64)
        0x3F, //
        0xa4, //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
        0xd3, //-set display offset
        0x00, //-not offset
        0xd5, //--set display clock divide ratio/oscillator frequency
        0xf0, //--set divide ratio
        0xd9, //--set pre-charge period
        0x22, //
        0xda, //--set com pins hardware configuration
        0x12,
        0xdb, //--set vcomh
        0x20, //0x20,0.77xVcc
        0x8d, //--set DC-DC enable
        0x14, //
        0xaf//--turn on oled panel
        };
void WriteCmd()
{
    uint8_t i = 0;
    for(i=0; i<27; i++){
        HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x00,I2C_MEMADD_SIZE_8BIT,CMD_Data+i,1,0x100);
    }
}
//向设备写控制命令
void OLED_WR_CMD(uint8_t cmd)
{
    HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x00,I2C_MEMADD_SIZE_8BIT,&cmd,1,0x100);
}
//向设备写数据
void OLED_WR_DATA(uint8_t data)
{
    HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x40,I2C_MEMADD_SIZE_8BIT,&data,1,0x100);
}
//初始化oled屏幕
void OLED_Init(void)
{
    HAL_Delay(200);
    WriteCmd();
}
//清屏
void OLED_Clear()
{
    uint8_t i,n;
    for(i=0;i<8;i++)
    {
        OLED_WR_CMD(0xb0+i);
        OLED_WR_CMD (0x00);
        OLED_WR_CMD (0x10);
        for(n=0;n<128;n++)
            OLED_WR_DATA(0);
    }
}
//开启OLED显示
void OLED_Display_On(void)
{
    OLED_WR_CMD(0X8D);  //SET DCDC命令
    OLED_WR_CMD(0X14);  //DCDC ON
    OLED_WR_CMD(0XAF);  //DISPLAY ON
}
//关闭OLED显示
void OLED_Display_Off(void)
{
    OLED_WR_CMD(0X8D);  //SET DCDC命令
    OLED_WR_CMD(0X10);  //DCDC OFF
    OLED_WR_CMD(0XAE);  //DISPLAY OFF
}
void OLED_Set_Pos(uint8_t x, uint8_t y)
{
    OLED_WR_CMD(0xb0+y);
    OLED_WR_CMD(((x&0xf0)>>4)|0x10);
    OLED_WR_CMD(x&0x0f);
}

void OLED_On(void)
{
    uint8_t i,n;
    for(i=0;i<8;i++)
    {
        OLED_WR_CMD(0xb0+i);    //设置页地址（0~7）
        OLED_WR_CMD(0x00);      //设置显示位置—列低地址
        OLED_WR_CMD(0x10);      //设置显示位置—列高地址
        for(n=0;n<128;n++)
            OLED_WR_DATA(1);
    } //更新显示
}
unsigned int oled_pow(uint8_t m,uint8_t n)
{
    unsigned int result=1;
    while(n--)result*=m;
    return result;
}

//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~63
//mode:0,反白显示;1,正常显示
//size:选择字体 16/12
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t Char_Size)
{
    unsigned char c=0,i=0;
    c=chr-' ';//得到偏移后的值
    if(x>128-1){x=0;y=y+2;}
    if(Char_Size ==16)
    {
        OLED_Set_Pos(x,y);
        for(i=0;i<8;i++)
            OLED_WR_DATA(F8x16[c*16+i]);
        OLED_Set_Pos(x,y+1);
        for(i=0;i<8;i++)
            OLED_WR_DATA(F8x16[c*16+i+8]);
    }
    else {
        OLED_Set_Pos(x,y);
        for(i=0;i<6;i++)
            OLED_WR_DATA(F6x8[c][i]);

    }
}
//显示2个数字
//x,y :起点坐标
//len :数字的位数
//size:字体大小
//mode:模式	0,填充模式;1,叠加模式
//num:数值(0~4294967295);
void OLED_ShowNum(uint8_t x,uint8_t y,unsigned int num,uint8_t len,uint8_t size2)
{
    uint8_t t,temp;
    uint8_t enshow=0;
    for(t=0;t<len;t++)
    {
        temp=(num/oled_pow(10,len-t-1))%10;
        if(enshow==0&&t<(len-1))
        {
            if(temp==0)
            {
                OLED_ShowChar(x+(size2/2)*t,y,' ',size2);
                continue;
            }else enshow=1;

        }
        OLED_ShowChar(x+(size2/2)*t,y,temp+'0',size2);
    }
}
//显示一个字符号串
void OLED_ShowString(uint8_t x,uint8_t y,uint8_t *chr,uint8_t Char_Size)
{
    unsigned char j=0;
    while (chr[j]!='\0')
    {		OLED_ShowChar(x,y,chr[j],Char_Size);
        x+=8;
        if(x>120){x=0;y+=2;}
        j++;
    }
}
//显示汉字
//hzk 用取模软件得出的数组
void OLED_ShowChinese(uint8_t x,uint8_t y,uint8_t no)
{
    uint8_t t,adder=0;
    OLED_Set_Pos(x,y);
    for(t=0;t<16;t++)
    {
        OLED_WR_DATA(Hzk[2*no][t]);
        adder+=1;
    }
    OLED_Set_Pos(x,y+1);
    for(t=0;t<16;t++)
    {
        OLED_WR_DATA(Hzk[2*no+1][t]);
        adder+=1;
    }
}
