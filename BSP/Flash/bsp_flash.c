#include "bsp_flash.h"

// 读 Flash（直接读地址即可）
uint32_t BSP_Flash_ReadWord(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}

// 写 Flash（先擦除，再写入）
void BSP_Flash_WriteWord(uint32_t addr, uint32_t data)
{
    HAL_FLASH_Unlock(); // 1. 解锁

    // 2. 配置擦除（这里用阻塞式擦除，擦除很慢，千万别在中断里干）
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError = 0;
    
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3; // 2.7V - 3.6V
    EraseInitStruct.Sector = FLASH_SAVE_SECTOR;
    EraseInitStruct.NbSectors = 1;
    
    HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);

    // 3. 写入数据
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, data);

    HAL_FLASH_Lock(); // 4. 锁住
}