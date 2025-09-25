#ifndef	_LED_H
#define _LED_H

#include "gpio.h"


#define KEY1_PRES()   HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3)

void led_init(void);
void key_init(void);

extern uint8_t key;

#endif
