#include ".\BSP\LCD\lcd.h"

/**
 * @brief       ILI9341寄存器初始化代码
 * @param       无
 * @retval      无
 */
void lcd_ex_ili9341_reginit(void)
{
    lcd_wt_cmd(0xCF);
    lcd_wt_data(0x00);
    lcd_wt_data(0xC1);
    lcd_wt_data(0X30);
    lcd_wt_cmd(0xED);
    lcd_wt_data(0x64);
    lcd_wt_data(0x03);
    lcd_wt_data(0X12);
    lcd_wt_data(0X81);
    lcd_wt_cmd(0xE8);
    lcd_wt_data(0x85);
    lcd_wt_data(0x10);
    lcd_wt_data(0x7A);
    lcd_wt_cmd(0xCB);
    lcd_wt_data(0x39);
    lcd_wt_data(0x2C);
    lcd_wt_data(0x00);
    lcd_wt_data(0x34);
    lcd_wt_data(0x02);
    lcd_wt_cmd(0xF7);
    lcd_wt_data(0x20);
    lcd_wt_cmd(0xEA);
    lcd_wt_data(0x00);
    lcd_wt_data(0x00);
    lcd_wt_cmd(0xC0); /* Power control */
    lcd_wt_data(0x1B);  /* VRH[5:0] */
    lcd_wt_cmd(0xC1); /* Power control */
    lcd_wt_data(0x01);  /* SAP[2:0];BT[3:0] */
    lcd_wt_cmd(0xC5); /* VCM control */
    lcd_wt_data(0x30);  /* 3F */
    lcd_wt_data(0x30);  /* 3C */
    lcd_wt_cmd(0xC7); /* VCM control2 */
    lcd_wt_data(0XB7);
    lcd_wt_cmd(0x36); /* Memory Access Control */
    lcd_wt_data(0x48);
    lcd_wt_cmd(0x3A);
    lcd_wt_data(0x55);
    lcd_wt_cmd(0xB1);
    lcd_wt_data(0x00);
    lcd_wt_data(0x1A);
    lcd_wt_cmd(0xB6); /* Display Function Control */
    lcd_wt_data(0x0A);
    lcd_wt_data(0xA2);
    lcd_wt_cmd(0xF2); /* 3Gamma Function Disable */
    lcd_wt_data(0x00);
    lcd_wt_cmd(0x26); /* Gamma curve selected */
    lcd_wt_data(0x01);
    lcd_wt_cmd(0xE0); /* Set Gamma */
    lcd_wt_data(0x0F);
    lcd_wt_data(0x2A);
    lcd_wt_data(0x28);
    lcd_wt_data(0x08);
    lcd_wt_data(0x0E);
    lcd_wt_data(0x08);
    lcd_wt_data(0x54);
    lcd_wt_data(0XA9);
    lcd_wt_data(0x43);
    lcd_wt_data(0x0A);
    lcd_wt_data(0x0F);
    lcd_wt_data(0x00);
    lcd_wt_data(0x00);
    lcd_wt_data(0x00);
    lcd_wt_data(0x00);
    lcd_wt_cmd(0XE1); /* Set Gamma */
    lcd_wt_data(0x00);
    lcd_wt_data(0x15);
    lcd_wt_data(0x17);
    lcd_wt_data(0x07);
    lcd_wt_data(0x11);
    lcd_wt_data(0x06);
    lcd_wt_data(0x2B);
    lcd_wt_data(0x56);
    lcd_wt_data(0x3C);
    lcd_wt_data(0x05);
    lcd_wt_data(0x10);
    lcd_wt_data(0x0F);
    lcd_wt_data(0x3F);
    lcd_wt_data(0x3F);
    lcd_wt_data(0x0F);
    lcd_wt_cmd(0x2B);
    lcd_wt_data(0x00);
    lcd_wt_data(0x00);
    lcd_wt_data(0x01);
    lcd_wt_data(0x3f);
    lcd_wt_cmd(0x2A);
    lcd_wt_data(0x00);
    lcd_wt_data(0x00);
    lcd_wt_data(0x00);
    lcd_wt_data(0xef);
    lcd_wt_cmd(0x11); /* Exit Sleep */
    HAL_Delay(120);
    lcd_wt_cmd(0x29); /* display on */
 }
