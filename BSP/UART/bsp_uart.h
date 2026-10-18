#ifndef __BSP_UART_H
#define __BSP_UART_H

#include "main.h"
#include "semphr.h"

#define UART_RX_BUF_SIZE 128
#define UART_TX_BUF_SIZE 128

void BSP_UART_Init(void);
uint16_t BSP_UART_GetReceivedData(uint8_t **data_ptr);
void BSP_UART_SendString(const char *str);

#endif