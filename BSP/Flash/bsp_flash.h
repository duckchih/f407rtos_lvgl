#ifndef __BSP_FLASH_H
#define __BSP_FLASH_H

#include "main.h"

#define FLASH_SAVE_ADDR  0x080E0000 // 使用第 11 扇区 (1MB 容量，每扇区 128KB)
#define FLASH_SAVE_SECTOR FLASH_SECTOR_11

uint32_t BSP_Flash_ReadWord(uint32_t addr);
void BSP_Flash_WriteWord(uint32_t addr, uint32_t data);

#endif