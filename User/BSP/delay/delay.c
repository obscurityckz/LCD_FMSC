#include ".\BSP\delay\delay.h"

//在不影响Hal_Delay的情况下实现us级延时
void delay_us(uint32_t nus)
{
    if(nus <= 0)
        return ;

    uint32_t load_before = SysTick->LOAD;    //获取系统默认设置的重装值，保存数据为了恢复之前的计时时间
    uint32_t tmp;
    uint32_t ticks = nus * 168;              //系统主频168MHZ，经过HAL_Init分频后的系统节拍是168
    
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;//关闭滴答定时器中断
//    SysTick->LOAD = nus * (load_before + 1) / 1000 - 1;
	SysTick->LOAD = ticks;
    SysTick->VAL = 0; 

    do{
        tmp = SysTick->CTRL;
    }while ((tmp & 0x01) && !(tmp & (1 << 16)));//当滴答定时器的第0位寄存器（定时器使能位）为真且第17位寄存器（递减计时到0时置1标志位，读取自动清零）
    
    //恢复
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;  //关闭定时器
    SysTick->LOAD = load_before;                //重设装载值
    SysTick->VAL  = 0;                          //定时器清零
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;  //开启中断
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;   //使能定时器
}