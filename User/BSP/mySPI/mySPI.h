#ifndef _MYSPI_H
#define _MYSPI_H

#include "stdint.h"
#include "gpio.h"
#include "main.h"

#define SPI_SCK_H()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET)
#define SPI_SCK_L()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET)

#define SPI_MOSI_H()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET)
#define SPI_MOSI_L()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET)

#define SPI_MISO_READ() HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4)

#define SPI_CS_H()    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET)
#define SPI_CS_L()    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET)

void mySPI_init(void);
uint8_t mySPI_ReadWriteByte(uint8_t byte); 

#endif 
