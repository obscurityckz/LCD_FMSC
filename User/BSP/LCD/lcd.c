#include ".\BSP\LCD\lcd.h"
#include ".\BSP\LCD\lcd_ex.h"
#include ".\BSP\LCD\lcdfont.h"

/*
仿照正点原子例程，简化驱动代码，驱动ILI9341 LCD芯片，以下是驱动步骤：
1.配置相关GPIO、FSMC外设
2.实现基本的读写接口，数据读写、寄存器写，几个基本API实现,
3.对LCD的参数结构体进行初始化，其中包括清屏函数lcd_clear、显示函数cd_display_dir，
	分别实现了光标设置lcd_set_cursor、方向显示lcd_scan_dir的寄存器配置指令+参数写入/读取，
	完成了对lcddev的x，y坐标、wramcmd、dir、窗口大小等参数的配置
4.完成以上的驱动部分，接下来就可以实现画点、划线的一些列应用函数的实现
5.通过取模软件还可以对字符进行取模
*/

//LCD的画笔颜色和背景色
uint16_t POINT_COLOR = 0x0000;		//画笔颜色
uint16_t BACK_COLOR = 0xFFFF;		//背景颜色

//管理LCD重要参数
//默认为竖屏
_lcd_dev lcddev;

/**
 *@brief 		LCD寄存器写入数据，这里由FSMC自动实现写入时序
 *@param		reg：寄存器地址
 *@param		data：待写入数据
 *@retval		无
 */
void lcd_wt_reg(uint16_t reg, uint16_t regval)
{
	LCD->LCD_REG = reg;
	LCD->LCD_RAM = regval;
}

/**
 *@brief 		LCD寄存器写入指令，这里由FSMC自动实现写入时序
 *@param		reg：寄存器地址
 *@retval		无
 */
void lcd_wt_cmd(volatile uint16_t reg_cmd)
{
	reg_cmd = reg_cmd;
	LCD->LCD_REG = reg_cmd;
}

/**
 *@brief 		LCD写数据，这里由FSMC自动实现写入时序
 *@param		ram_data：要写入的数据
 *@retval		无
 */
void lcd_wt_data(volatile uint16_t ram_data)
{
	ram_data = ram_data;
	LCD->LCD_RAM = ram_data;
}

/**
 *@brief 		LCD读数据，这里由FSMC自动实现写入时序
 *@param		无
 *@retval		ram_data：读取到的数据
 */
uint16_t lcd_rd_data(void)
{
	volatile uint16_t ram_data;
	ram_data = LCD->LCD_RAM;
	return ram_data;
}

void lcd_init(uint8_t dir)
{
	MX_FSMC_Init();//完成FSMC外设和GPIO、时钟等配置

	/*读取ILI9341的ID*/
	lcd_wt_cmd(0xD3);			//读取Id指令，后面的参数就是ILI9341接受应答后返回的值，如果能够读取到id说明芯片正常工作
	lcddev.id = lcd_rd_data();	//dummy读取
	lcddev.id = lcd_rd_data();	//读到0x00
	lcddev.id = lcd_rd_data();	//读到0x93
	lcddev.id <<= 8;
	lcddev.id |= lcd_rd_data();	//读到0x41
	
	if(lcddev.id != 0x9341)
	{
		printf("LCD ID error:%#x\r\n", lcddev.id);
		return ;
	}
	else 
	{
		printf("LCD ID:%#x\r\n", lcddev.id);
		lcd_ex_ili9341_reginit();				/*寄存器初始化配置*/
	}
	
	lcd_display_dir(dir);		/*默认位竖屏*/
	LCD_LED_BL_H;			/*点亮背光*/
	lcd_clear(WHITE);
}

/**
 *@brief 	设置LCD的自动扫描方向(对RGB屏无效)，包含了寄存器的写入方式和扫描方向
 *@param	dir：0~7代表8个方向，默认L2R_U2D扫描，其他扫描方式可能会导致显示方向异常
 *@retval	无
 */
void lcd_scan_dir(uint8_t dir)
{
	uint16_t regval = 0;
	uint16_t dirreg = 0x36;			//对于绝大多数驱动IC的方向写入寄存器都是0x36
	
	/*根据扫描方式，设置0x36寄存器bit 5,6,7位的值*/
	switch(dir)
	{
		case L2R_U2D:
			regval |= (0 << 7) | (0 << 6) | (0 << 5);
			break;
		
		case U2D_L2R :
			regval |= (0 << 7) | (0 << 6) | (1 << 5);
			break;
		
		case R2L_U2D:
			regval |= (0 << 7) | (1 << 6) | (0 << 5);
			break;
		
		case U2D_R2L:
			regval |= (0 << 7) | (1 << 6) | (1 << 5);
			break;
		
		case L2R_D2U:
			regval |= (1 << 7) | (0 << 6) | (0 << 5);
			break;
		
		case D2U_L2R:
			regval |= (1 << 7) | (0 << 6) | (1 << 5);
			break;
		
		case R2L_D2U:
			regval |= (1 << 7) | (1 << 6) | (0 << 5);
			break;
		
		case D2U_R2L:
			regval |= (1 << 7) | (1 << 6) | (1 << 5);
			break;
	}
	
	if(lcddev.id == 0x9341)		//不同的驱动IC可能寄存器不一样,93491的0x36寄存器bit3控制RGB-BGR顺序,0:RGB;1:BGR
	{
		regval |= (1 << 3);		//RGB-BGR顺序
	}

	lcd_wt_reg(dirreg, regval);
	//printf("lcd_scan_dir:%#x\r\n", regval);
	

	/*设置显示区域（开窗）大小*/
	if(lcddev.id == 0x9341)
	{
		lcd_wt_cmd(lcddev.setxcmd);
		lcd_wt_data(0);							//16位起始坐标高位
		lcd_wt_data(0);							//16位起始坐标低位
		lcd_wt_data((lcddev.width - 1) >> 8);	//16位终止地址高位
		lcd_wt_data((lcddev.width - 1) & 0xFF);	//16位终止地址低位
		lcd_wt_cmd(lcddev.setycmd);
		lcd_wt_data(0);							
		lcd_wt_data(0);							
		lcd_wt_data((lcddev.height - 1) >> 8);	
		lcd_wt_data((lcddev.height - 1) & 0xFF);
	}
}

/**
 *@brief 		设置LCD显示方向
 *@param		dir：1横屏；0竖屏
 *@retval		无
 */
void lcd_display_dir(uint8_t dir)
{
	lcddev.dir = dir;				//设置显示方向
	if(lcddev.dir == 0)				//竖屏
	{
		lcddev.width = 240;
		lcddev.height = 320;
		if(lcddev.id == 0x9341)			//设置指令
		{
			lcddev.wramcmd = 0x2C;		//写入GRAM
			lcddev.setxcmd = 0x2A;		//设置X坐标
			lcddev.setycmd = 0x2B;		//设置y坐标
		}
		lcd_scan_dir(DFT_SCAN_DIR);		//默认扫描方向
	}
	else
	{
		lcddev.width = 320;			//横屏
		lcddev.height = 240;
		
		if(lcddev.id == 0x9341)			
		{
			lcddev.wramcmd = 0x2C;		
			lcddev.setxcmd = 0x2A;		//写入方式、扫描方向决定了字模的取法，字模的方向也要旋转为横向
			lcddev.setycmd = 0x2B;		
		}
		lcd_scan_dir(D2U_L2R);		//会随着字模的显示方式发生改变
	}
}

/**
 * @brief       设置光标位置(对RGB屏无效)
 * @param       x,y: 坐标
 * @retval      无
 */
void lcd_set_cursor(uint16_t x, uint16_t y)
{
	if(lcddev.id == 0x9341)
	{
		lcd_wt_cmd(lcddev.setxcmd);
		lcd_wt_data(x >> 8);
		lcd_wt_data(x & 0xFF);
		lcd_wt_cmd(lcddev.setycmd);
		lcd_wt_data(y >> 8);
		lcd_wt_data(y & 0xFF);
	}

}

/**
 *@brief 	清屏函数
 *@param	color：要清屏的颜色
 *@retval	无
 */
void lcd_clear(uint16_t color)
{
	uint32_t index = 0;
	uint32_t totalpoint = lcddev.width * lcddev.height;
	
	lcd_set_cursor(0x0000, 0x0000);		//将光标移到起始坐标
	
	LCD->LCD_REG = lcddev.wramcmd;					//写入指令：准备写GRAM
	for(index = 0; index < totalpoint; index++)		//写入数据，即清屏的画笔颜色
	{
		LCD->LCD_RAM = color;
	}
}

/**
 * @brief	在指定区域内填充颜色，部分清屏
 * @param	（sx， sy），（ex， ey）:填充矩形对角坐标，区域大小：(ex - sx + 1) * (ey - sy + 1)
 * @note	起始坐标在左上角，结束坐标在右下角
 * @param	color：32位颜色，便于兼容LTDC
 * @retval	无
 */
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
	uint16_t xlen = 0;
	xlen = ex - (sx - 1);
	for(uint16_t i = sy; i < ey; i++)
	{
		lcd_set_cursor(sx, i);					//设置光标的航向
		LCD->LCD_REG = lcddev.wramcmd;
		for(uint16_t j = 0; j < xlen; j++)
		{
			LCD->LCD_RAM = color;				//横向写入颜色
		}
	}
}

/**
 *@brief 	在指定位置画一个点
 *@param	x,y：坐标
 *@param	color：颜色
 *@retval	无
 */
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
	lcd_set_cursor(x, y);
	LCD->LCD_REG = lcddev.wramcmd;		
	LCD->LCD_RAM = color;
}

/**
 *@brief 	读取指定位置的颜色值
 *@param	x,y：坐标
 *@retval	颜色值,32位方便兼容LTDC，ILI9341在使用时只取低16位，RGB565格式
 */
uint32_t lcd_read_point(uint16_t x, uint16_t y)
{
	uint16_t color_R = 0, color_G = 0, color_B = 0;

	if((x >= lcddev.width) || (y >= lcddev.height)) return 0;	//坐标超出范围，直接返回

	lcd_set_cursor(x, y);						//设置光标位置
	lcd_wt_cmd(0x2E);							//开始读GRAM
	color_R = lcd_rd_data();					//dummy read
	color_R = lcd_rd_data();					//读到R1、G1值 	R1[15:11] xxx G1[7:2] xx	16bit
	color_B = lcd_rd_data();					//读到B1值	 	B1[15:11] xxx R2[7:3] xxx	16bit,R2及其后续的连续颜色值不需要，都是一样的
	color_G = color_R & 0xFF;					//取出G1的值
	
	return ((color_R >> 11) << 11) | ((color_G >> 2) << 5) | (color_B >> 11);	//拼接成RGB565格式返回，不能省略左移右移的操作，
																				//因为读出来的值中间有些位是无效的，位移操作可以将无效位清零
}

/**
 *@brief 	画线函数
 *@param	x1,y1：起点坐标
 *@param	x2,y2：终点坐标
 *@retval	无
 *@note	使用 DDA（数字微分分析）算法,用整数运算模拟直线斜率
 */
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	int16_t xerr = 0, yerr = 0, delta_x, delta_y, distance;
	int16_t incx, incy, uRow, uCol;
	delta_x = x2 - x1; 								//计算坐标增量
	delta_y = y2 - y1;
	uRow = x1;
	uCol = y1;

	if(delta_x > 0) incx = 1; 						//设置单步方向
	else if(delta_x == 0) incx = 0;					//垂直线
	else { incx = -1; delta_x = -delta_x; }

	if(delta_y > 0) incy = 1;
	else if(delta_y == 0) incy = 0;					//水平线
	else { incy = -1; delta_y = -delta_y; }

	if(delta_x > delta_y) distance = delta_x; 		//选取基本增量坐标轴，选择增量大的作为基本坐标轴
	else distance = delta_y;

	for(int t = 0; t <= distance + 1; t++) 			//画线输出
	{
		lcd_draw_point(uRow, uCol, color); 	//画点
		xerr += delta_x;
		yerr += delta_y;
		if(xerr > distance)							//不一定每一次误差都会大于增量，增量是取delta较大值，通过累加误差来逼近真实直线在像素屏幕上的显示，避免阶梯化扭曲
		{
			xerr -= distance;
			uRow += incx;
		}
		if(yerr > distance)
		{
			yerr -= distance;
			uCol += incy;
		}
	}	
}

/**
 * @brief       画圆
 * @param       x0,y0 : 圆中心坐标
 * @param       r     : 半径
 * @param       color : 圆的颜色
 * @retval      无
 */
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int a, b;
    int di;

    a = 0;
    b = r;
    di = 3 - (r << 1);       /* 判断下个点位置的标志 */

    while (a <= b)
    {
        lcd_draw_point(x0 + a, y0 - b, color);  /* 5 */
        lcd_draw_point(x0 + b, y0 - a, color);  /* 0 */
        lcd_draw_point(x0 + b, y0 + a, color);  /* 4 */
        lcd_draw_point(x0 + a, y0 + b, color);  /* 6 */
        lcd_draw_point(x0 - a, y0 + b, color);  /* 1 */
        lcd_draw_point(x0 - b, y0 + a, color);
        lcd_draw_point(x0 - a, y0 - b, color);  /* 2 */
        lcd_draw_point(x0 - b, y0 - a, color);  /* 7 */
        a++;

        /* 使用Bresenham算法画圆 */
        if (di < 0)
        {
            di += 4 * a + 6;
        }
        else
        {
            di += 10 + 4 * (a - b);
            b--;
        }
    }
}

/**
 *@brief 	在指定位置显示一个字符
 *@param	x,y：起始坐标
 *@param	chr_index：ASCII字符值
 *@param	size：字体大小，目前只支持12/16/24
 *@param	mode：0非叠加方式，1叠加方式
 *@param	color：字体颜色
 *@retval	无
 *@note		该函数只支持显示ASCII字符,ASCII字符取值范围为' '-->'~'
 */
void lcd_show_char(uint16_t x, uint16_t y, char chr_index, uint8_t size, uint8_t mode, uint8_t color)
{
	uint8_t temp, t1, t;
	uint16_t y0 = y;
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); /* 得到字体一个字符对应点阵集所占的字节数 */
    uint8_t *pfont = 0;

    chr_index = chr_index - ' ';    /* 得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库） */

	if(lcddev.dir == 0)		/* 竖屏显示 */
	{
		switch (size)
		{
			case 12:
				pfont = (uint8_t *)asc2_1206_vertical[chr_index];  /* 调用1206字体 */
				break;

			case 16:
				pfont = (uint8_t *)asc2_1608_vertical[chr_index];  /* 调用1608字体 */
				break;

			case 24:
				pfont = (uint8_t *)asc2_2412_vertical[chr_index];  /* 调用2412字体 */
				break;

			// case 32:
			//     pfont = (uint8_t *)asc2_3216[chr_index];  /* 调用3216字体 */
			//     break;

			default:
				return ;
		}
	}
    else
	{
		switch (size)
		{
			// case 12:
			// 	pfont = (uint8_t *)asc2_1206[chr_index];  /* 调用1206字体 */
			// 	break;

			case 16:
				pfont = (uint8_t *)asc2_1608_horizontal[chr_index];  /* 调用1608字体 */
				break;

			// case 24:
			// 	pfont = (uint8_t *)asc2_2412[chr_index];  /* 调用2412字体 */
			// 	break;

			// case 32:
			//     pfont = (uint8_t *)asc2_3216[chr_index];  /* 调用3216字体 */
			//     break;

			default:
				return ;
		}
	}

    for (t = 0; t < csize; t++)
    {
        temp = pfont[t];                            /* 获取字符的点阵数据 */

        for (t1 = 0; t1 < 8; t1++)                  /* 一个字节8个点 */
        {
            if (temp & 0x80)                        /* 有效点,需要显示 */
            {
                lcd_draw_point(x, y, color);        /* 画点出来,要显示这个点 */
            }
            else if (mode == 0)                     /* 无效点,不显示 */
            {
                lcd_draw_point(x, y, BACK_COLOR); /* 画背景色,相当于这个点不显示(注意背景色由全局变量控制) */
            }

            temp <<= 1;                             /* 移位, 以便获取下一个位的状态 */
            y++;

            if (y >= lcddev.height)return;          /* 超区域了 */

            if ((y - y0) == size)                   /* 显示完一列了? */
            {
                y = y0; /* y坐标复位 */
                x++;    /* x坐标递增 */

                if (x >= lcddev.width) return;       /* x坐标超区域了 */
                break;
            }
        }
    }
}

/**
 * @brief       显示字符串
 * @param       x,y         : 起始坐标
 * @param       width,height: 区域大小
 * @param       size        : 选择字体 12/16/24/32
 * @param       p           : 字符串首地址
 * @param       color       : 字符串的颜色;
 * @retval      无
 */
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)
{
    uint8_t x0 = x;

	width += x;
	height += y;

	while ((*p <= '~') && (*p >= ' '))   /* 判断是不是非法字符! */
	{
		if (x >= width)
		{
			x = x0;
			y += size;
		}

		if (y >= height)
		{
			break;      /* 退出 */
		}

		lcd_show_char(x, y, *p, size, 0, color);
		x += size / 2;
		p++;
	}

}

//中文打印函数待添加


