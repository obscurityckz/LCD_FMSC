#include ".\BSP\W25Q64\w25q64.h"

/* 这里使用的是LCD屏上的Flash，根据原理图的结构采用的是高电平选中Flash，低电平选中XPT2046 */
void W25Q64_Init(void)
{
    mySPI_init(); // 初始化SPI接口
}

void W25Q64_WriteEnable(void)
{
    SPI_CS_L(); // 使能片选
    mySPI_ReadWriteByte(W25Q64_Write_Enable); // 发送写使能指令
    SPI_CS_H(); // 禁止片选
}

void W25Q64_WriteDisable(void)
{
    SPI_CS_L(); // 使能片选
    mySPI_ReadWriteByte(W25Q64_Write_Disable); // 发送写禁止指令
    SPI_CS_H(); // 禁止片选
}

/**
 * @brief       等待W25Q64空闲
 * @note        通过读取状态寄存器1的忙标志位(BUSY)来判断
 * @param       无
 * @retval      status: 状态寄存器1的值,1表示忙,0表示空闲
*/

uint8_t W25Q64_WaitBusyStatus(void)
{
    uint16_t Timeout = 0xFFFF; // 超时计数器
    uint8_t status = 0;
    SPI_CS_L(); // 使能片选
    mySPI_ReadWriteByte(W25Q64_Read_Status_register_1); // 发送读取状态寄存器1指令
    while(Timeout--)
    {
        status = mySPI_ReadWriteByte(W25Q64_DUMMY_BYTE); // 读取状态寄存器值
        if((status & 0x01) == 0) // 检查忙标志位
            break; // 如果不忙，退出循环
        
        if(Timeout == 0)
        {
            printf("W25Q64 wait busy timeout!\r\n");
            break; // 超时处理
        }
    }
    SPI_CS_H(); // 禁止片选
    return status;
}

/**
 * @brief       擦除扇区
 * @note        每个扇区大小为4KB,W2Q64共128块，每块16扇区，每个扇区16页，每页最多256字节，地址为24位，寻址空间：0x000000~0x7FFFFF
 * @param       Dst_Addr: 扇区地址,范围:0~4096(对应0~4KB)
 * @retval      无
 */
void W25Q64_EraseSector(uint32_t Dst_Addr)
{
    W25Q64_WriteEnable(); // 使能写操作
    SPI_CS_L(); // 使能片选
    mySPI_ReadWriteByte(W25Q64_Sector_Erase_4KB); // 发送扇区擦除指令
    mySPI_ReadWriteByte((Dst_Addr >> 16) & 0xFF); // 发送高地址字节
    mySPI_ReadWriteByte((Dst_Addr >> 8) & 0xFF); // 发送中地址字节
    mySPI_ReadWriteByte(Dst_Addr & 0xFF); // 发送低地址字节
    SPI_CS_H(); // 禁止片选
    W25Q64_WaitBusyStatus(); // 等待擦除完成
    W25Q64_WriteDisable(); // 禁止写操作
}

/**
* @brief       擦除32KB块
* @param       Dst_Addr: 块地址,范围:0~65536(对应0~32KB)
* @retval      无
*/
void W25Q64_EraseBlock(uint32_t Dst_Addr)
{
    W25Q64_WriteEnable(); // 使能写操作
    SPI_CS_L(); // 使能片选
    mySPI_ReadWriteByte(W25Q64_Block_Erase_32KB); // 发送32KB块擦除指令
    mySPI_ReadWriteByte((Dst_Addr >> 16) & 0xFF); // 发送高地址字节
    mySPI_ReadWriteByte((Dst_Addr >> 8) & 0xFF); // 发送中地址字节
    mySPI_ReadWriteByte(Dst_Addr & 0xFF); // 发送低地址字节
    SPI_CS_H(); // 禁止片选
    W25Q64_WaitBusyStatus(); // 等待擦除完成
    W25Q64_WriteDisable(); // 禁止写操作
}

/**
 * @brief       擦除整个芯片
 * @param       无
 * @retval      无
 */
void W25Q64_EraseChip(void)
{
    W25Q64_WriteEnable(); // 使能写操作
    SPI_CS_L(); // 使能片选
    mySPI_ReadWriteByte(W25Q64_Chip_Erase); // 发送芯片擦除指令
    SPI_CS_H(); // 禁止片选
    W25Q64_WaitBusyStatus(); // 等待擦除完成
    W25Q64_WriteDisable(); // 禁止写操作
}

#define W25Q64_PAGE_SIZE 256 // 每页256字节
/**
 * @brief       写W25Q64数据,支持跨页写
 * @param       pBuffer: 数据缓冲区指针
 * @param       WriteAddr: 写入地址(24位)
 * @param       NumByteToWrite: 要写入的字节数(每个扇区最大256)
 * @retval      无
 */
void W25Q64_PageWrite(uint32_t WriteAddr, uint8_t *pBuffer, uint16_t NumByteToWrite)
{
    uint16_t page_remain = 0;

    while(NumByteToWrite > 0)
    {
        page_remain = W25Q64_PAGE_SIZE - (WriteAddr % W25Q64_PAGE_SIZE); // 计算当前页剩余空间

        uint16_t write_size = (NumByteToWrite < page_remain) ? NumByteToWrite : page_remain; // 本次写入大小

        W25Q64_WriteEnable(); // 使能写操作
        SPI_CS_L(); // 使能片选
        mySPI_ReadWriteByte(W25Q64_Page_Program); // 发送页编程指令
        mySPI_ReadWriteByte((WriteAddr >> 16) & 0xFF); // 发送高地址字节
        mySPI_ReadWriteByte((WriteAddr >> 8) & 0xFF); // 发送中地址字节
        mySPI_ReadWriteByte(WriteAddr & 0xFF); // 发送低地址字节
        for(uint16_t i = 0; i < write_size; i++)
        {
            mySPI_ReadWriteByte(pBuffer[i]); // 写入数据
        }
        SPI_CS_H(); // 禁止片选
        W25Q64_WaitBusyStatus(); // 等待写入完成

        //更新地址和计数,准备写下一页
        WriteAddr += write_size;
        pBuffer += write_size;
        NumByteToWrite -= write_size;
    }
    W25Q64_WriteDisable(); // 禁止写操作
}

/**
 * @brief       读W25Q64数据
 * @param       pBuffer: 数据缓冲区指针
 * @param       ReadAddr: 读取地址(24位)
 * @param       NumByteToRead: 要读取的字节数(最大256)
 * @retval      无
 */
void W25Q64_ReadData(uint32_t ReadAddr, uint8_t* pBuffer, uint16_t NumByteToRead)
{
    SPI_CS_L(); // 使能片选
    mySPI_ReadWriteByte(W25Q64_Read_Data); // 发送读取数据指令
    mySPI_ReadWriteByte((ReadAddr >> 16) & 0xFF); // 发送高地址字节
    mySPI_ReadWriteByte((ReadAddr >> 8) & 0xFF);  // 发送中地址字节
    mySPI_ReadWriteByte(ReadAddr & 0xFF);         // 发送低地址字节
    for(uint16_t i = 0; i < NumByteToRead; i++)
    {
        pBuffer[i] = mySPI_ReadWriteByte(W25Q64_DUMMY_BYTE); // 读取数据
    }
    SPI_CS_H(); // 禁止片选
}

/**
 * @brief       读取W25Q64的JEDEC ID
 * @param       ID: 存储ID的变量指针
 * @retval      无
 */
void W25Q64_ReadID(uint32_t* ID)
{
    uint8_t temp[3] = {0};
    SPI_CS_L(); // 使能片选
    mySPI_ReadWriteByte(W25Q64_JEDEC_ID); // 发送JEDEC ID读取指令
    temp[0] = mySPI_ReadWriteByte(W25Q64_DUMMY_BYTE); // 读取制造商ID
    temp[1] = mySPI_ReadWriteByte(W25Q64_DUMMY_BYTE); // 读取存储器类型
    temp[2] = mySPI_ReadWriteByte(W25Q64_DUMMY_BYTE); // 读取容量
    SPI_CS_H(); // 禁止片选
    *ID = (temp[0] << 16) | (temp[1] << 8) | temp[2]; // 组合成32位ID
}

/**
 * @brief       解除写保护
 * @param       无
 * @retval      无
 */
void W25Q64_WriteRegUnprotected(void)
{
    W25Q64_WriteEnable(); // 使能写操作

    SPI_CS_L(); // 使能片选
    mySPI_ReadWriteByte(W25Q64_Write_Status_register); // 发送写状态寄存器指令
    mySPI_ReadWriteByte(0x00); // 写入状态寄存器1
    mySPI_ReadWriteByte(0x00); // 写入状态寄存器2
    SPI_CS_H(); // 禁止片选

    W25Q64_WaitBusyStatus(); // 等待写入完成
    //W25Q64_WriteDisable(); // 禁止写操作
}


uint8_t W25Q64_ReadReg(uint8_t reg)
{
	uint8_t sta;

	SPI_CS_L();
	if(reg == 1)
	{
		mySPI_ReadWriteByte(W25Q64_Read_Status_register_1);
	}
	else if(reg == 2)
	{
		mySPI_ReadWriteByte(W25Q64_Read_Status_register_2);
	}
	sta = mySPI_ReadWriteByte(W25Q64_DUMMY_BYTE);
	SPI_CS_H();
	
	return sta;
}
