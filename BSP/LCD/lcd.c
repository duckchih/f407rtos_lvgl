#include "lcd.h"
#include "FreeRTOS.h"
#include "task.h"

/* 封装底层引脚控制，使用 CubeMX 生成的宏，方便以后换引脚 */
#define LCD_CS_LOW()    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)
#define LCD_CS_HIGH()   HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)

#define LCD_DC_LOW()    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET)
#define LCD_DC_HIGH()   HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET)

#define LCD_RST_LOW()   HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET)
#define LCD_RST_HIGH()  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET)

#define LCD_PWR_LOW()   HAL_GPIO_WritePin(LCD_PWR_GPIO_Port, LCD_PWR_Pin, GPIO_PIN_RESET)
#define LCD_PWR_HIGH()  HAL_GPIO_WritePin(LCD_PWR_GPIO_Port, LCD_PWR_Pin, GPIO_PIN_SET)

/* 底层写命令和写数据 */
void LCD_WriteCommand(uint8_t cmd)
{
    LCD_CS_LOW();
    LCD_DC_LOW(); // DC=0 表示命令
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}

void LCD_WriteData8(uint8_t data)
{
    LCD_CS_LOW();
    LCD_DC_HIGH(); // DC=1 表示数据
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}

void LCD_WriteData16(uint16_t data)
{
    uint8_t buf[2];
    buf[0] = data >> 8;
    buf[1] = data & 0xFF;
    LCD_CS_LOW();
    LCD_DC_HIGH();
    HAL_SPI_Transmit(&hspi1, buf, 2, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}

/* 设置显示区域 (ST7789 支持 172x320) */
void LCD_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    x1 += 34;
    x2 += 34;
    LCD_WriteCommand(0x2A); // 列地址设置
    LCD_WriteData16(x1);
    LCD_WriteData16(x2);

    LCD_WriteCommand(0x2B); // 行地址设置
    LCD_WriteData16(y1);
    LCD_WriteData16(y2);

    LCD_WriteCommand(0x2C); // 写显存
}

/* 屏幕初始化序列 */
void LCD_Init(void)
{
    // 1. 屏幕供电使能 (给模块内的 LDO 供电)
    LCD_PWR_LOW();
    vTaskDelay(pdMS_TO_TICKS(50));

    // 2. 硬件复位
    LCD_RST_HIGH();
    vTaskDelay(pdMS_TO_TICKS(10));
    LCD_RST_LOW();
    vTaskDelay(pdMS_TO_TICKS(20));
    LCD_RST_HIGH();
    vTaskDelay(pdMS_TO_TICKS(120));

    // 3. ST7789V3 初始化序列
    LCD_WriteCommand(0x11); // Sleep Out
    vTaskDelay(pdMS_TO_TICKS(120));

    LCD_WriteCommand(0x36); // Memory Data Access Control
    LCD_WriteData8(0xC0);   // 0x00 是竖屏，如果是横屏可以改 0x60

    LCD_WriteCommand(0x3A); // Interface Pixel Format
    LCD_WriteData8(0x55);   // 16 bits/pixel (RGB565)

    LCD_WriteCommand(0x21); // Display Inversion ON (IPS 屏通常需要开启)

    LCD_WriteCommand(0x13); // Normal Display Mode ON

    // 设置 RGB 控制参数
    LCD_WriteCommand(0xB2);
    LCD_WriteData8(0x0C); LCD_WriteData8(0x0C);
    LCD_WriteData8(0x00); LCD_WriteData8(0x33);
    LCD_WriteData8(0x33);

    // 电源控制
    LCD_WriteCommand(0xB7);
    LCD_WriteData8(0x35);

    LCD_WriteCommand(0xBB);
    LCD_WriteData8(0x19);

    LCD_WriteCommand(0xC0);
    LCD_WriteData8(0x2C);

    LCD_WriteCommand(0xC2);
    LCD_WriteData8(0x01);

    LCD_WriteCommand(0xC3);
    LCD_WriteData8(0x12);

    LCD_WriteCommand(0xC4);
    LCD_WriteData8(0x20);

    LCD_WriteCommand(0xC6);
    LCD_WriteData8(0x0F);

    LCD_WriteCommand(0xD0);
    LCD_WriteData8(0xA4);
    LCD_WriteData8(0xA1);

    // 伽马校正
    LCD_WriteCommand(0xE0);
    LCD_WriteData8(0xD0); LCD_WriteData8(0x04);
    LCD_WriteData8(0x0D); LCD_WriteData8(0x11);
    LCD_WriteData8(0x13); LCD_WriteData8(0x2B);
    LCD_WriteData8(0x3F); LCD_WriteData8(0x54);
    LCD_WriteData8(0x4C); LCD_WriteData8(0x18);
    LCD_WriteData8(0x0D); LCD_WriteData8(0x0B);
    LCD_WriteData8(0x1F); LCD_WriteData8(0x23);

    LCD_WriteCommand(0xE1);
    LCD_WriteData8(0xD0); LCD_WriteData8(0x04);
    LCD_WriteData8(0x0C); LCD_WriteData8(0x11);
    LCD_WriteData8(0x13); LCD_WriteData8(0x2C);
    LCD_WriteData8(0x3F); LCD_WriteData8(0x44);
    LCD_WriteData8(0x51); LCD_WriteData8(0x2F);
    LCD_WriteData8(0x1F); LCD_WriteData8(0x1F);
    LCD_WriteData8(0x20); LCD_WriteData8(0x23);

    LCD_WriteCommand(0x29); // Display ON

    // 4. 清屏为白色
    LCD_Fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, WHITE);
}

/* 填充区域 */
void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color)
{
    uint32_t size = (xend - xsta + 1) * (yend - ysta + 1);
    if (size == 0) return;

    LCD_SetWindow(xsta, ysta, xend, yend);
    
    LCD_CS_LOW();
    LCD_DC_HIGH(); // 进入写显存模式，连续发数据

    uint8_t buf[2];
    buf[0] = color >> 8;
    buf[1] = color & 0xFF;

    // 为了速度，可以按块发送，这里直接循环发送
    for (uint32_t i = 0; i < size; i++) {
        HAL_SPI_Transmit(&hspi1, buf, 2, HAL_MAX_DELAY);
    }
    LCD_CS_HIGH();
}
