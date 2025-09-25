#ifndef _TOUCH_H
#define _TOUCH_H

#include "gpio.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define T_CS(x)    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, (GPIO_PinState)x)
#define T_CLK(x)   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, (GPIO_PinState)x)
#define T_MOSI(x)  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, (GPIO_PinState)x)

#define T_MISO()    HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14)
#define T_Pen_IRQ() HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5)

#define TP_PRES_DOWN    0x8000  //检测到按下，只要按下就标记，包括瞬间按下后松手和持续触摸
#define TP_CATCH_PRES   0x4000  //检测瞬间按下后松手，用于区分按下后松手还是触摸，每次检测完后要清除该标志位，用于单次检测
#define TOUCH_MAX  10           // 触摸点个数，电容屏支持多点触摸，最多10点，电阻屏不支持，此处为兼容设置多点采样值
typedef struct
{
    uint8_t (*init)(void);      // 触摸屏初始化
    uint8_t (*scan)(uint8_t);   // 触摸屏扫描:0,屏幕坐标;1,物理坐标。返回值：0，触屏有触摸;1,触屏无触摸
    void (*adjust)(void);       // 触摸屏校准

    /*
    用第0位表示s当前坐标，第9位记录当前坐标
    */
    uint16_t x[TOUCH_MAX]; 
    uint16_t y[TOUCH_MAX]; 
   
    uint16_t sta;      /* 触摸点状态 
                          b15：按下1/松开0
                          b14:0,没有按键按下;1,有按键按下.
                          b13~b10:保留
                          b9~b0:电容触摸屏按下的点数(0,表示未按下,1表示按下)
                        */

    /* 触摸屏类型，当触摸屏左右上下颠倒的时候需要用到
        b0：竖屏(适合左右为X坐标,上下为Y坐标的TP)；1：横屏(适合左右为Y坐标,上下为X坐标的TP)
        b1~6：保留
        b7：0，电阻屏；1，电容屏
    */
    uint8_t touchtype;

    /*以下为电阻屏校准用参数，电容屏不需要*/
    float xfac;    // 触摸屏校准参数X
    float yfac;    // 触摸屏校准参数Y
     short xc;      //中心x坐标物理值（AD采样）五点校准法
     short yc;      //中心y坐标物理值（AD采样）
//    short xoff;  //四点校准法
//    short yoff;

}_touch_dev;    //定义结构体类型，用于管理触摸信息
extern _touch_dev tp_dev;

uint8_t XPT2046_init(void);                                                         //初始化
uint16_t XPT2046_Read_AD(uint8_t cmd);                                              //XPT读写时序API

 uint16_t TP_AverageFilterRead_xy(uint8_t cmd);                               //均值滤波
void TP_ReadForDifferentDir_xy(uint16_t *x, uint16_t *y);                    //根据方向写入数据
 uint8_t TP_RangeFilterRead_xy(uint16_t *x, uint16_t *y);                     //范围滤波，滤出超范围的数据

static  uint8_t TP_Scan(uint8_t mode);                                              //触控屏扫描，校准和触控两用
static void TP_Adjust(void);                                                        //数据校准
uint8_t TP_GetAdjustData(void);                                                     //获得校准数据
void TP_SaveAdjustData(void);                                                       //保存校准数据
static void TP_ShowAdjustInformation(uint16_t xy[5][2], float px, float py);        //显示校准数据

static void lcd_draw_touch_cross(uint16_t x, uint16_t y, uint16_t color);           //画十字架，用于校准
void tp_draw_big_point(uint16_t x, uint16_t y, uint16_t color);                    //画大点，用于显示笔触轨迹

#endif
