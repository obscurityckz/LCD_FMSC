#ifndef _LCD_H
#define _LCD_H

#include "stdlib.h"
#include "main.h"
#include "fsmc.h"
#include "usart.h"


//LCD重要参数集
typedef struct
{
	uint16_t width;			//LCD窗口宽度
	uint16_t height;		//LCD窗口高度
	uint16_t id;			//LCD ID
	uint8_t dir;			//LCD显示方向
	uint16_t wramcmd;		//开始写GRAM指令
	uint16_t setxcmd;		//设置x坐标指令
	uint16_t setycmd;		//设置y坐标指令
}_lcd_dev;
//LCD参数
extern _lcd_dev lcddev;	//在c文件中管理重要参数

//LDC端口定义，实现读写寄存器的基础API
typedef struct
{
	uint16_t LCD_REG;		//写寄存器指令，低电平有效
	uint16_t LCD_RAM;		//写入内存的数据，高电平有效
} LCD_Typedef;
//使用NOR/SRAM的 Bank1.sector1,地址位HADDR[27,26]=00   A18作为数据命令区分线
//注意设置时STM32内部会右移一位对齐! 	即HADDR[n-1] = FSMC_A[n](An) n = 1,2,3...，HADDR[0]在16bits位宽的时候不使用
#define LCD_BASE				((uint32_t)(0x60000000 | 0x0007FFFE))//LCD在NOR/SRAM的对应基地址，此时对应的是A18低电平的情况，对应LCD_Typedef第一个写寄存器bit位低电平有效,+2byte即为高电平，代表输入数据
#define LCD						((LCD_Typedef *)LCD_BASE)

//背光控制
#define LCD_LED_BL_H	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET)
#define LCD_LED_BL_L	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET)


//扫描方向
#define L2R_U2D  0 //从左到右,从上到下
#define L2R_D2U  1 //从左到右,从下到上
#define R2L_U2D  2 //从右到左,从上到下
#define R2L_D2U  3 //从右到左,从下到上

#define U2D_L2R  4 //从上到下,从左到右
#define U2D_R2L  5 //从上到下,从右到左
#define D2U_L2R  6 //从下到上,从左到右
#define D2U_R2L  7 //从下到上,从右到左	

#define DFT_SCAN_DIR	L2R_U2D	//默认的扫描方向

//LCD颜色
extern uint16_t POINT_COLOR;	//默认画笔颜色
extern uint16_t BACK_COLOR;		//默认背景颜色

//画笔颜色
#define WHITE   		0xFFFF
#define BLACK   		0x0000
#define BLUE    		0x001F
#define RED     		0xF800
#define GREEN   		0x07E0
#define MAGENTA 		0xF81F	/* 品红色/紫红色 = BLUE + RED */
#define CYAN    		0x07FF	/* 青色 = GREEN + BLUE */
#define YELLOW  		0xFFE0
#define BROWN 	        0XBC40 //棕色
#define BRRED 	        0XFC07 //棕红色
#define GRAY  	        0X8430 //灰色
//GUI颜色
#define DARKBLUE      	 0X01CF	//深蓝色
#define LIGHTBLUE      	 0X7D7C	//浅蓝色
#define GRAYBLUE       	 0X5458 //灰蓝色
//以上三色为PANEL的颜色
#define LIGHTGREEN     	 0X841F //浅绿色
//#define LIGHTGRAY      0XEF5B //浅灰色(PANNEL)
#define LGRAY 			 0XC618 //浅灰色(PANNEL),窗体背景色
#define LGRAYBLUE        0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE           0X2B12 //浅棕蓝色(选择条目的反色)

/*函数声明*/
void lcd_wt_reg(uint16_t reg, uint16_t regval);
void lcd_wt_cmd(volatile uint16_t reg_cmd);
void lcd_wt_data(volatile uint16_t ram_data);

void lcd_init(uint8_t dir);
void lcd_scan_dir(uint8_t dir);												//设置扫描方向
void lcd_display_dir(uint8_t dir);											//设置显示方向，包含了显示、扫描方向、宽高设置、寄存器设置等参数配置，放在初始化函数中执行

void lcd_set_cursor(uint16_t x, uint16_t y);								//设置光标位置，在清屏和画点函数中都有使用，基本API
void lcd_clear(uint16_t color);												//清屏
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);//一个矩形区域内填充颜色


void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color);								//画点，图像显示的基本函数
uint32_t lcd_read_point(uint16_t x, uint16_t y);											//读点，返回点的颜色值
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);		//画线
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);					//画圆


void lcd_show_char(uint16_t x, uint16_t y, char chr_index, uint8_t size, uint8_t mode, uint8_t color);	//显示一个字符
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color);	//显示字符串


/*LCD驱动IC指令*/
#define XSETCMD 		0x2B		//设置x坐标
#define YSETCMD 		0x2A		//设置y坐标
#define RAMWRCMD 		0x2C		//开始写GRAM
#define RAMRDCMD 		0x2E		//开始读GRAM
#define DIRWRCMD 		0x36		//设置扫描方向


#endif
