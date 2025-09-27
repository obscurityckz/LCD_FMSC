#include ".\BSP\Touch\touch.h"
#include ".\BSP\LCD\lcd.h"          //电阻屏在使用前需要校准，需要用到划线和字符显示
#include ".\BSP\W25Q64\w25q64.h"    //用到Flash模拟EEPROM存储校正数据，还有SPI通信，驱动
#include ".\BSP\delay\delay.h"
#include ".\BSP\LCD\lcdfont.h"

_touch_dev tp_dev = {
    XPT2046_init,
    TP_Scan,
    TP_Adjust,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

/**
 * @brief       触摸IC初始化
 * @retval      是否经过校验：0，经过校验；1未经过校验
 */
uint8_t XPT2046_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    tp_dev.touchtype = 0;                   // 默认设置（电阻屏&竖屏）
    tp_dev.touchtype |= lcddev.dir & 0x01;  //根据LCD显示方向确定是横屏还是竖屏

    if(lcddev.id == 0x9341)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();

        GPIO_InitStructure.Pin = GPIO_PIN_13 | GPIO_PIN_15 | GPIO_PIN_12;   //SCK MOSI CS
        GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStructure.Pull = GPIO_NOPULL;
        GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

        GPIO_InitStructure.Pin = GPIO_PIN_14;       //MISO
        GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

        GPIO_InitStructure.Pin = GPIO_PIN_5;        //T_PEN_IRQ
        GPIO_InitStructure.Pull = GPIO_PULLUP;      //常态下上拉保持，当产生按下中断，中断标志位置0
        HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);//拉高片选，未选中

        W25Q64_Init();  //初始化Flash，用于存储校准值
        TP_ReadForDifferentDir_xy(&tp_dev.x[0], &tp_dev.y[0]); //第一次读取初始化

       if(TP_GetAdjustData() == 0)//已经校准，避免初始化重复校准
	   {
			lcd_clear(WHITE);
			lcd_show_string(40, 160, lcddev.width, lcddev.height, 16, "Screen Exit Adjust!", RED);
			HAL_Delay(1000);
			lcd_clear(WHITE);
		   
		   return 1;
	   }
       else
       {
           lcd_clear(WHITE);    
           TP_Adjust();
           TP_SaveAdjustData();
       }
    }
    return 0;
}

/**
 * @brief   SPI读写时序，根据XPT2046的原理图来写
 * @note    从触摸屏IC读取adc值
 * @param   cmd：指令
 * @retval  读取到的ADC值12bit
 */
 uint16_t XPT2046_Read_AD(uint8_t cmd)
{
    uint16_t data;

    T_CLK(0);
    T_MOSI(0);
    T_CS(0);
    for(uint8_t i = 0; i < 8; i++)
    {
        if(cmd & 0x80)      
            T_MOSI(1);     
        else            
            T_MOSI(0); 
        
        cmd <<= 1;
        T_CLK(0);           //上升沿发送数据8位
        delay_us(1);
        T_CLK(1);
    }

    delay_us(5);//转换延时
	T_CLK(0);
	delay_us(1);
    T_CLK(1);
    delay_us(1);//一个时钟清除busy位
    T_CLK(0);

    for(uint8_t i = 0; i < 16; i++)
    {
		data <<= 1;//先位移，否则最后16位数据的最高位就会溢出
		
        T_CLK(0);
        delay_us(1);
        T_CLK(1);       
                
        data |= T_MISO();  //下降沿接收数据16位，只要有高12位有效（12位AD）
    }
    T_CS(1);
    return data >> 4;
}


#define TP_READ_TIMES   5
#define TP_DISCARD_VAL  1
/**
 * @brief   读取一个物理值（x或者y），均值滤波
 * @note    连续读取TP_READ_TIMES次数据,对这些数据升序排列,
 *          然后去掉最低和最高TP_LOST_VAL个数, 取平均值
 *          设置时需满足: TP_READ_TIMES > 2*TP_LOST_VAL 的条件
 * @param   cmd：指令
 *   @arg   0XD0: 读取X轴坐标(@竖屏状态,横屏状态和Y对调.)
 *   @arg   0X90: 读取Y轴坐标(@竖屏状态,横屏状态和X对调.)
 * 
 * @retval  读取到的数据(滤波后的), ADC值(12bit)
 */
 uint16_t TP_AverageFilterRead_xy(uint8_t cmd)
{
    uint16_t buf[TP_READ_TIMES];
    uint32_t sum = 0;//初始化，否则会出现随机值

    for(uint8_t i = 0; i < TP_READ_TIMES; i++)
    {
        buf[i] = XPT2046_Read_AD(cmd);
    }

    for(uint8_t i = 0; i < TP_READ_TIMES - 1; i++)
    {
        for(uint8_t j = i + 1; j < TP_READ_TIMES; j++)
        {
            if(buf[i] > buf[j])			//升序排列
            {
                uint16_t tmp = buf[i];
                buf[i] = buf[j];
                buf[j] = tmp;
            }
        }  
    }

    for(uint8_t i = TP_DISCARD_VAL; i < (TP_READ_TIMES - TP_DISCARD_VAL); i++)//去掉最值求均值
    {
        sum += buf[i];
    }

    return (uint16_t)(sum / (TP_READ_TIMES - 2*TP_DISCARD_VAL));
}

uint8_t CMD_RDX = 0xD0;//默认为touchtype = 0，电阻屏竖屏显示效果
uint8_t CMD_RDY = 0x90;
/**
 * @brief 根据LCD显示方向更改坐标的AD读取
 * @param   *x：x坐标指针；*y：y坐标指针
 * @retval 无
 */
void TP_ReadForDifferentDir_xy(uint16_t *x, uint16_t *y)
{
    uint16_t xval, yval;

    if((tp_dev.touchtype & 0x01) == 0)//竖屏
    {
        xval = TP_AverageFilterRead_xy(CMD_RDX);
        yval = TP_AverageFilterRead_xy(CMD_RDY);
    }
    else //横屏
    {
        xval = TP_AverageFilterRead_xy(CMD_RDY);
        yval = TP_AverageFilterRead_xy(CMD_RDX);
    }
    
    *x = xval; *y = yval;
}

#define TP_RANGE_MAX    50  //连续两次采样得到的数据之差超出这个范围的数据被认为是错误数据
/**
 * @brief   在均值滤波的基础上采集两次数据，滤出超出范围的数据，因为手指的按压是面不是点，进一步提高采样精度
 * @param   *x：x坐标指针；*y：y坐标指针
 * @retval  0:成功，数据正确；1：失败,数据错误
 */
 uint8_t TP_RangeFilterRead_xy(uint16_t *x, uint16_t *y)
{
    uint16_t x1, y1;
    uint16_t x2, y2;

    TP_ReadForDifferentDir_xy(&x1, &y1);
    TP_ReadForDifferentDir_xy(&x2, &y2);

    if( 
        (((x2 <= x1) && (x1 < (x2 + TP_RANGE_MAX))) || ((x1 <= x2) && (x2 < (x1 + TP_RANGE_MAX))))  
        &&
        (((y2 <= y1) && (y1 < (y2 + TP_RANGE_MAX))) || ((y1 <= y2) && (y2 < (y1 + TP_RANGE_MAX))))
      )
    {
        *x = (x1 + x2) / 2;
        *y = (y1 + y2) / 2;
        return 0;
    }

    return 1;
}

/**
 * @brief   按键扫描
 * @param   mode:模式选择
 *          1：物理扫描（校准等特殊用途）
 *          0：屏幕坐标
 * @retval  tp_dev.sta & TP_PRES_DOWN：当前的触屏状态，是否被按下
 */
static  uint8_t TP_Scan(uint8_t mode)
{
    if(T_Pen_IRQ() == 0)//按下检测
    {
        if(mode)        //扫描模式：用于校准，即在TP_Scan中一直处于该阶段
        {
            TP_RangeFilterRead_xy(&tp_dev.x[0], &tp_dev.y[0]);
        }
        else if(TP_RangeFilterRead_xy(&tp_dev.x[0], &tp_dev.y[0]) == 0)//已经完成校准，坐标读取正确，读取物理坐标，转换为屏幕尺寸
        {
            //x轴   物理坐标转换成逻辑坐标（LCD尺寸上的x坐标值）
            tp_dev.x[0] = (signed short)(tp_dev.x[0] - tp_dev.xc) / tp_dev.xfac + lcddev.width / 2;   //点斜式转换，已知数据是（物理x坐标的中心值，LCD屏幕x坐标的中心值）
            //y轴   物理坐标转换成逻辑坐标（LCD尺寸上的x坐标值）
            tp_dev.y[0] = (signed short)(tp_dev.y[0] - tp_dev.yc) / tp_dev.yfac + lcddev.height / 2; //点斜式转换，已知数据是（物理x坐标的中心值，LCD屏幕x坐标的中心值）
            // tp_dev.x[0]=tp_dev.xfac*tp_dev.x[0]+tp_dev.xoff;//将结果转换为屏幕坐标
			// tp_dev.y[0]=tp_dev.yfac*tp_dev.y[0]+tp_dev.yoff;
        }

        if((tp_dev.sta & TP_PRES_DOWN) == 0)                //如果之前未被按下
        {
            tp_dev.sta = (TP_PRES_DOWN | TP_CATCH_PRES);   //如果之前未被按下，此时按下后置标志位为按下且瞬间按下
            tp_dev.x[TOUCH_MAX - 1] = tp_dev.x[0];          //记录当前的坐标值，用于下次处理用
            tp_dev.y[TOUCH_MAX - 1] = tp_dev.y[0];
        }
    }
    else                //如果未被按下
    {
        if(tp_dev.sta & TP_PRES_DOWN)       //之前是按下的后松开
        {
            tp_dev.sta &= ~(TP_PRES_DOWN);  //清除按键按下标志位
        }
        else                                //如果之前就没被按下
        {
            tp_dev.x[0] = 0xffff;
            tp_dev.y[0] = 0xffff;
            tp_dev.x[TOUCH_MAX - 1] = 0;
            tp_dev.y[TOUCH_MAX - 1] = 0;
        }
    }

    return tp_dev.sta & TP_PRES_DOWN;       
}

//四点校准法相关函数
/********************************************************************************************************************************************************/

//在指定位置显示一个字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//size:字体大小 12/16/24
//mode:叠加方式(1)还是非叠加方式(0)
void LCD_ShowChar(uint16_t x,uint16_t y,uint8_t num,uint8_t size,uint8_t mode)
{  							  
    uint8_t temp,t1,t;
	uint16_t y0=y;
	uint8_t csize=(size/8+((size%8)?1:0))*(size/2);		//得到字体一个字符对应点阵集所占的字节数	
	//设置窗口		   
	num=num-' ';//得到偏移后的值
	for(t=0;t<csize;t++)
	{   
		if(size==12)temp=asc2_1206_vertical[num][t]; 	 	//调用1206字体
		else if(size==16)temp=asc2_1608_vertical[num][t];	//调用1608字体
		else if(size==24)temp=asc2_2412_vertical[num][t];	//调用2412字体
		else return;								//没有的字库
		for(t1=0;t1<8;t1++)
		{			    
			if(temp&0x80)lcd_draw_point(x,y,POINT_COLOR);
			else if(mode==0)lcd_draw_point(x,y,BACK_COLOR);
			temp<<=1;
			y++;
			if(y>=lcddev.height)return;		//超区域了
			if((y-y0)==size)
			{
				y=y0;
				x++;
				if(x>=lcddev.width)return;	//超区域了
				break;
			}
		}  	 
	}  	    	   	 	  
}

//m^n函数
//返回值:m^n次方.
uint32_t LCD_Pow(uint8_t m,uint8_t n)
{
	uint32_t result=1;	 
	while(n--)result*=m;    
	return result;
}

//显示数字,高位为0,则不显示
//x,y :起点坐标	 
//len :数字的位数
//size:字体大小
//color:颜色 
//num:数值(0~4294967295);	 
void LCD_ShowNum(uint16_t x,uint16_t y,uint32_t num,uint8_t len,uint8_t size)
{         	
	uint8_t t,temp;
	uint8_t enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/LCD_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				LCD_ShowChar(x+(size/2)*t,y,' ',size,0);
				continue;
			}else enshow=1; 
		 	 
		}
	 	LCD_ShowChar(x+(size/2)*t,y,temp+'0',size,0); 
	}
}

//显示字符串
//x,y:起点坐标
//width,height:区域大小  
//size:字体大小
//*p:字符串起始地址		  
void LCD_ShowString(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint8_t size,uint8_t *p)
{         
	uint8_t x0=x;
	width+=x;
	height+=y;
    while((*p<='~')&&(*p>=' '))//判断是不是非法字符!
    {       
        if(x>=width){x=x0;y+=size;}
        if(y>=height)break;//退出
        LCD_ShowChar(x,y,*p,size,0);
        x+=size/2;
        p++;
    }  
}

//提示校准结果(各个参数)
void TP_Adj_Info_Show(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t x3,uint16_t y3,uint16_t fac)
{	  
	POINT_COLOR=RED;
	LCD_ShowString(40,160,lcddev.width,lcddev.height,16,"x1:");
 	LCD_ShowString(40+80,160,lcddev.width,lcddev.height,16,"y1:");
 	LCD_ShowString(40,180,lcddev.width,lcddev.height,16,"x2:");
 	LCD_ShowString(40+80,180,lcddev.width,lcddev.height,16,"y2:");
	LCD_ShowString(40,200,lcddev.width,lcddev.height,16,"x3:");
 	LCD_ShowString(40+80,200,lcddev.width,lcddev.height,16,"y3:");
	LCD_ShowString(40,220,lcddev.width,lcddev.height,16,"x4:");
 	LCD_ShowString(40+80,220,lcddev.width,lcddev.height,16,"y4:");  
 	LCD_ShowString(40,240,lcddev.width,lcddev.height,16,"fac is:");     
	LCD_ShowNum(40+24,160,x0,4,16);		//显示数值
	LCD_ShowNum(40+24+80,160,y0,4,16);	//显示数值
	LCD_ShowNum(40+24,180,x1,4,16);		//显示数值
	LCD_ShowNum(40+24+80,180,y1,4,16);	//显示数值
	LCD_ShowNum(40+24,200,x2,4,16);		//显示数值
	LCD_ShowNum(40+24+80,200,y2,4,16);	//显示数值
	LCD_ShowNum(40+24,220,x3,4,16);		//显示数值
	LCD_ShowNum(40+24+80,220,y3,4,16);	//显示数值
 	LCD_ShowNum(40+56,240,fac,3,16); 	//显示数值,该数值必须在95~105范围之内.
}
/******************************************************************************************************************************************************/

#define POINT_NUM   5   //五点校准是5，四点校准是4
/**
 * @brief   电阻屏校准，目前只是进行竖屏的校准
 * @note    使用五点校准法，得到x，y轴的比例因子xfac,yfac以及物理中心坐标值(xc, yc)这四个参数
 *          其中：物理坐标：触摸IC采集的坐标值，范围0~4095；
 *                逻辑坐标：LCD屏幕的尺寸坐标，范围240*320。
 * @param   无
 * @retval  无
 */
static void TP_Adjust(void)
{
    uint16_t xy[POINT_NUM][2];              //五个坐标，四个边缘坐标用于算比例因子，第五个坐标用来点斜式求线性方程
     double px, py;                   //x，y的比例因子
     short sx1, sx2, sy1, sy2;       //4组物理坐标的差值
	// uint32_t tem1, tem2;				//四点校准
    // uint16_t d1,d2;
	// double fac;
	
    uint16_t outtime = 0;           //计时，超时5s没有按下就退出校准，以上一次校准数据为准
    uint8_t cnt = 0;                //五个校准点计数

    tp_dev.sta = 0;                     //清除按下标志
    lcd_clear(WHITE);                   //清屏
    lcd_draw_touch_cross(20, 20, RED);  //画点1，左上角
    lcd_show_string(40, 40, 160, 100, 16,
    "Please use the stylus click the cross on the screen.The cross will always move until the screen adjustment is completed.",
    RED);                               //显示提示信息
    
    while(1)
    {
        tp_dev.scan(1);         //轮询扫描校准

        if((tp_dev.sta & 0xc000) == TP_CATCH_PRES)  //检测到一次快速按键按下松开
        {
			tp_dev.sta &= ~(TP_CATCH_PRES);
			outtime = 0;
			
            xy[cnt][0] = tp_dev.x[0];       //保存x物理值
            xy[cnt][1] = tp_dev.y[0];       //保存y物理值
            cnt++;

            switch (cnt)
            {
                case 1:
                    lcd_draw_touch_cross(20, 20, WHITE);
                    lcd_draw_touch_cross(lcddev.width - 20, 20, RED);//画点2，右上角
                    break;
                
                case 2:
                    lcd_draw_touch_cross(lcddev.width - 20, 20, WHITE);
                    lcd_draw_touch_cross(20, lcddev.height - 20, RED);//画点3，左下角
                    break;

                case 3:
                    lcd_draw_touch_cross(20, lcddev.height - 20, WHITE);
                    lcd_draw_touch_cross(lcddev.width - 20, lcddev.height - 20, RED);//画点4，右下角
                    break;

                case 4:
                    lcd_clear(WHITE);
                    lcd_draw_touch_cross(lcddev.width / 2, lcddev.height / 2, RED);//画点5，中心点
                    break;
                //对边相等（四点校准法）
					// tem1=abs(xy[0][0]-xy[1][0]);//x1-x2
					// tem2=abs(xy[0][1]-xy[1][1]);//y1-y2
					// tem1*=tem1;
					// tem2*=tem2;
					// d1=sqrt(tem1+tem2);//得到1,2的距离
					
					// tem1=abs(xy[2][0]-xy[3][0]);//x3-x4
					// tem2=abs(xy[2][1]-xy[3][1]);//y3-y4
					// tem1*=tem1;
					// tem2*=tem2;
					// d2=sqrt(tem1+tem2);//得到3,4的距离
					// fac=(float)d1/d2;
					// if(fac<0.95||fac>1.05||d1==0||d2==0)//不合格
					// {
					// 	cnt=0;
				    // 	lcd_draw_touch_cross(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
  	 				// 	lcd_draw_touch_cross(20,20,RED);								//画点1
					// 	TP_Adj_Info_Show(xy[0][0],xy[0][1],xy[1][0],xy[1][1],xy[2][0],xy[2][1],xy[3][0],xy[3][1],fac*100);//显示数据   
					// 	continue;
					// }
					// tem1=abs(xy[0][0]-xy[2][0]);//x1-x3
					// tem2=abs(xy[0][1]-xy[2][1]);//y1-y3
					// tem1*=tem1;
					// tem2*=tem2;
					// d1=sqrt(tem1+tem2);//得到1,3的距离
					
					// tem1=abs(xy[1][0]-xy[3][0]);//x2-x4
					// tem2=abs(xy[1][1]-xy[3][1]);//y2-y4
					// tem1*=tem1;
					// tem2*=tem2;
					// d2=sqrt(tem1+tem2);//得到2,4的距离
					// fac=(float)d1/d2;
					// if(fac<0.95||fac>1.05)//不合格
					// {
					// 	cnt=0;
				    // 	lcd_draw_touch_cross(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
  	 				// 	lcd_draw_touch_cross(20,20,RED);								//画点1
					// 	TP_Adj_Info_Show(xy[0][0],xy[0][1],xy[1][0],xy[1][1],xy[2][0],xy[2][1],xy[3][0],xy[3][1],fac*100);//显示数据   
					// 	continue;
					// }//正确了
					
					// //对角线相等
					// tem1=abs(xy[1][0]-xy[2][0]);//x1-x3
					// tem2=abs(xy[1][1]-xy[2][1]);//y1-y3
					// tem1*=tem1;
					// tem2*=tem2;
					// d1=sqrt(tem1+tem2);//得到1,4的距离
	
					// tem1=abs(xy[0][0]-xy[3][0]);//x2-x4
					// tem2=abs(xy[0][1]-xy[3][1]);//y2-y4
					// tem1*=tem1;
					// tem2*=tem2;
					// d2=sqrt(tem1+tem2);//得到2,3的距离
					// fac=(float)d1/d2;
					// if(fac<0.95||fac>1.05)//不合格
					// {
					// 	cnt=0;
				    // 	lcd_draw_touch_cross(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
  	 				// 	lcd_draw_touch_cross(20,20,RED);								//画点1
					// 	TP_Adj_Info_Show(xy[0][0],xy[0][1],xy[1][0],xy[1][1],xy[2][0],xy[2][1],xy[3][0],xy[3][1],fac*100);//显示数据   
					// 	continue;
					// }//正确了
					// //计算结果
					// tp_dev.xfac=(float)(lcddev.width-40)/(xy[1][0]-xy[0][0]);//得到xfac		 
					// tp_dev.xoff=(lcddev.width-tp_dev.xfac*(xy[1][0]+xy[0][0]))/2;//得到xoff
						  
					// tp_dev.yfac=(float)(lcddev.height-40)/(xy[2][1]-xy[0][1]);//得到yfac
					// tp_dev.yoff=(lcddev.height-tp_dev.yfac*(xy[2][1]+xy[0][1]))/2;//得到yoff  
					// if(abs(tp_dev.xfac)>2||abs(tp_dev.yfac)>2)//触屏和预设的相反了.
					// {
					// 	cnt=0;
				    // 	lcd_draw_touch_cross(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
  	 				// 	lcd_draw_touch_cross(20,20,RED);								//画点1
					// 	LCD_ShowString(40,26,lcddev.width,lcddev.height,16,"TP Need readjust!");
					// 	tp_dev.touchtype=!tp_dev.touchtype;//修改触屏类型.
					// 	if(tp_dev.touchtype)//X,Y方向与屏幕相反
					// 	{
					// 		CMD_RDX=0X90;
					// 		CMD_RDY=0XD0;	 
					// 	}else				   //X,Y方向与屏幕相同
					// 	{
					// 		CMD_RDX=0XD0;
					// 		CMD_RDY=0X90;	 
					// 	}			    
					// 	continue;
					// }		
					// POINT_COLOR=BLUE;
					// lcd_clear(WHITE);//清屏
					// LCD_ShowString(35,110,lcddev.width,lcddev.height,16,"Touch Screen Adjust OK!");//校正完成
					// HAL_Delay(1000);
					// TP_SaveAdjustData();  
					// lcd_clear(WHITE);//清屏   
					// return;//校正完成

               case 5:
                    sx1 = xy[1][0] - xy[0][0];      //第2个点与第1个点的x轴物理值做差（ADC采样值做差）
                    sx2 = xy[3][0] - xy[2][0];      //第4个点与第3个点的x轴物理值做差（ADC采样值做差）
                    sy1 = xy[2][1] - xy[0][1];      //第3个点与第1个点的y轴坐标值做差（ADC采样值做差）
                    sy2 = xy[3][1] - xy[1][1];      //第4个点与第2个点的y轴坐标值做差（ADC才颜值做差）

                    px = (float) sx1 / sx2;
                    py = (float) sy1 / sy2;

                    if(px < 0) px = -px;
                    if(py < 0) py = -py;

                    if(px < 0.95 || px > 1.05 || py < 0.95 || py > 1.05 || 
                       abs(sx1) > 4095 || abs(sx2) > 4095 || abs(sy1) > 4095 || abs(sy2) > 4095 ||
                       abs(sx1) == 0 || abs(sx2) == 0 || abs(sy1) == 0 || abs(sy2) == 0
                    )
                    {
                        cnt = 0;
                        lcd_draw_touch_cross(lcddev.width / 2, lcddev.height / 2, WHITE);   //清除点5
                        lcd_draw_touch_cross(20, 20, RED);//重画点1
                        TP_ShowAdjustInformation(xy, px, py);    //显示当前信息，方便找问题
                        continue;
                    }

                    tp_dev.xfac = (float)(sx1 + sx2) / (2 * (lcddev.width - 40));   //x方向转换因子，求平均后再按比例转换
                    tp_dev.yfac = (float)(sy1 + sy2) / (2 * (lcddev.height - 40));  //y方向转换因子

                    tp_dev.xc = xy[POINT_NUM - 1][0];   //中心点物理坐标（ADC采样值）
                    tp_dev.yc = xy[POINT_NUM - 1][1];

                    lcd_clear(WHITE);   //校准完成提示后清屏
                    lcd_show_string(35, 110, lcddev.width, lcddev.height, 16, "Touch Screen Adjust OK!", BLUE);
                    HAL_Delay(1000);
                    lcd_clear(WHITE);

                    TP_SaveAdjustData();//保存校准信息
                    return;
            }
        }

        HAL_Delay(10);
        outtime++;

        if(outtime > 1000)//超时后以上次校准信息为准
        {
            TP_GetAdjustData();
            break;
        }
    } 
}

/**
 * @brief 获取校准参数，避免重复校准
 * @param 无
 * @retval 0：数据已校准；
 *         1：获取失败，需要重新校准
 */
uint8_t TP_GetAdjustData(void)
{
    uint8_t *p = (uint8_t *)&tp_dev.xfac;
    uint8_t flag = 0;
    /* 由于我们是直接指向tp_dev.xfac地址进行保存的, 读取的时候,将读取出来的数据
     * 写入指向tp_dev.xfac的首地址, 就可以还原写入进去的值, 而不需要理会具体的数
     * 据类型. 此方法适用于各种数据(包括结构体)的保存/读取(包括结构体).
     */
    W25Q64_ReadData(256, p, 12);
    W25Q64_ReadData(256 + 12, &flag, 1);

    if(flag == 0xA0)//写入位检验是否校准
        return 0;
    else
        return 1;
}

/**
 * @brief 保存校准数据
 */
void TP_SaveAdjustData(void)
{
    uint8_t *p = (uint8_t *)&tp_dev.xfac;
    uint8_t flag = 0xA0;
    /* p指向tp_dev.xfac的地址, p+4则是tp_dev.yfac的地址
     * p+8则是tp_dev.xoff的地址,p+10,则是tp_dev.yoff的地址
     * 总共占用12个字节(4个参数)
     * p+12用于存放标记电阻触摸屏是否校准的数据(0X0A)
     * 往p[12]写入0X0A. 标记已经校准过.
     */
    W25Q64_PageWrite(256, p, 12);
    W25Q64_PageWrite(256 + 12, &flag, 1);
}

/**
 * @brief 显示校准信息
 * @param xy[POINT_NUM][2]:采样的5组坐标数据
 *        px：x轴的比例因子
 *        py：y轴的比例因子
 * @retval 无
 */
static void TP_ShowAdjustInformation(uint16_t xy[POINT_NUM][2], float px, float py)
{
    uint8_t i;
    char sbuf[20];
    for(i = 0; i < POINT_NUM; i++)
    {
        sprintf(sbuf, "x%d: %d", i + 1, xy[i][0]);
        lcd_show_string(40, 160 + (i * 20), lcddev.width, lcddev.height, 16, sbuf, RED);
        sprintf(sbuf, "y%d: %d", i + 1, xy[i][1]);
        lcd_show_string(40 + 80, 160 + (i * 20), lcddev.width, lcddev.height, 16, sbuf, RED);
    }
    
    lcd_fill(40, 160 + (i * 20), lcddev.width - 1, 16, WHITE);
    memset(sbuf, 0, sizeof(sbuf));
    sprintf(sbuf, "px:%0.2f", px);
//    sbuf[7] = '\0';
    lcd_show_string(40, 160 + (i * 20), lcddev.width, lcddev.height, 16, sbuf, RED);
    sprintf(sbuf, "py:%0.2f", py);
//    sbuf[7] = '\0';
    lcd_show_string(40 + 80, 160 + (i * 20), lcddev.width, lcddev.height, 16, sbuf, RED);
}

/*用于电阻屏校准用的函数，与LCD有关*/
/**********************************************************************************************/

/**
 * @brief	校准用的触摸点，形状为十字架+中心圆
 * @param	x：x坐标
 * @param   y：y坐标
 * @param   color：划线颜色
 * @retval	无
 */
static void lcd_draw_touch_cross(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_draw_circle(x , y, 6, color);               //画中心圆
	lcd_draw_line(x - 12, y, x+13, y, color);       //横线
	lcd_draw_line(x, y - 12, x, y + 13, color);     //竖线
    lcd_draw_point(x + 1, y + 1, color);            //描粗圆心
    lcd_draw_point(x - 1, y + 1, color);
    lcd_draw_point(x + 1, y - 1, color);
    lcd_draw_point(x - 1, y - 1, color);
}

/**
 * @brief 画一个大点，显示触摸轨迹
 */
void tp_draw_big_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_draw_point(x, y, color);
    lcd_draw_point(x + 1, y, color);
    lcd_draw_point(x, y + 1, color);
    lcd_draw_point(x + 1, y + 1, color);
}
/**********************************************************************************************/
