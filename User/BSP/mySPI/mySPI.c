#include ".\BSP\mySPI\mySPI.h"

/** 
 *@brief  Initializes the SPI peripheral.软件模拟SPI，便于灵活调整，而且Flash对速度要求不高，是单片机上的Flash
 *@param  None
 *@retval None
*/
void mySPI_init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE(); // 使能GPIOB时钟

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_5 | GPIO_PIN_0; // SCK, MOSI, CS
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_4; // MISO
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); // CS high
}

/** @brief  Sends a byte and receives a byte over SPI.软件模拟SPI的读写函数
 * @note   W25Q64支持模式0，CPOL=0, CPHA=0或者模式3，CPOL=1, CPHA=1，这里采用模式0
 * @param  byte: The byte to send.
 * @retval The received byte.
 */
uint8_t mySPI_ReadWriteByte(uint8_t byte)
{
    uint8_t received_byte = 0;
    for(int i = 0; i < 8; i++)
    {
        if(byte & 0x80)             // 发送最高位
            SPI_MOSI_H();
        else
            SPI_MOSI_L();

        SPI_SCK_H();                // 时钟上升沿，数据被采样

        received_byte <<= 1;        // 接收数据左移一位
        if(SPI_MISO_READ())         // 读取MISO引脚
            received_byte |= 0x01;  // 如果为高电平，设置最低位为1

        SPI_SCK_L();                // 时钟下降沿，准备发送下一位
        byte <<= 1;                 // 待发送数据左移一位
    }
    return received_byte;
}
