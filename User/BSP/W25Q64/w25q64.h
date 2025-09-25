#ifndef _W25Q64_H
#define _W25Q64_H

#include ".\BSP\mySPI\mySPI.h"
#include "stdint.h"
#include "stdio.h"

void W25Q64_Init(void);
void W25Q64_EraseSector(uint32_t Dst_Addr);
void W25Q64_EraseBlock(uint32_t Dst_Addr);
void W25Q64_EraseChip(void);
void W25Q64_PageWrite(uint32_t WriteAddr, uint8_t *pBuffer, uint16_t NumByteToWrite);
void W25Q64_ReadData(uint32_t ReadAddr, uint8_t* pBuffer, uint16_t NumByteToRead);
void W25Q64_ReadID(uint32_t* ID);
void W25Q64_WriteRegUnprotected(void);
uint8_t W25Q64_ReadReg(uint8_t reg);


//W25Q64指令表1
#define W25Q64_Write_Enable						          0x06
#define W25Q64_Write_Disable                              0x04
#define W25Q64_Read_Status_register_1				      0x05
#define W25Q64_Read_Status_register_2				      0x35
#define W25Q64_Write_Status_register				      0x01
#define W25Q64_Page_Program							      0x02      //每次最多只能写256字节
#define W25Q64_Quad_Page_Program				          0x32
#define W25Q64_Block_Erase_64KB						      0xD8
#define W25Q64_Block_Erase_32KB						      0x52
#define W25Q64_Sector_Erase_4KB						      0x20
#define W25Q64_Chip_Erase							      0xC7
#define W25Q64_Erase_Suspend					          0x75
#define W25Q64_Erase_Resume							      0x7A
#define W25Q64_Power_down							      0xB9
#define W25Q64_High_Performance_Mode				      0xA3
#define W25Q64_Continuous_Read_Mode_Reset			      0xFF
#define W25Q64_Release_Power_Down_HPM_Device_ID		      0xAB
#define W25Q64_Manufacturer_Device_ID				      0x90
#define W25Q64_Read_Uuique_ID						      0x4B
#define W25Q64_JEDEC_ID								      0x9F

//W25Q64指令集表2(读指令)
#define W25Q64_Read_Data						          0x03
#define W25Q64_Fast_Read						          0x0B
#define W25Q64_Fast_Read_Dual_Output				      0x3B
#define W25Q64_Fast_Read_Dual_IO					      0xBB
#define W25Q64_Fast_Read_Quad_Output				      0x6B
#define W25Q64_Fast_Read_Quad_IO					      0xEB
#define W25Q64_Octal_Word_Read_Quad_IO				      0xE3

#define W25Q64_DUMMY_BYTE						          0xFF

#endif 
