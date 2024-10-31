#ifndef __HAL_UART_H__
#define __HAL_UART_H__
#include <stdint.h>

#define MAX_UART_LEN 128

typedef struct
{
    char data[MAX_UART_LEN];
    uint32_t len;
} hal_uart_msg_t;

int hal_uart_init(void);
int hal_uart_send(uint8_t *buf, uint32_t len);
int hal_uart_recv(uint8_t *buf, uint32_t buf_len, uint32_t timeout);

hal_uart_msg_t *hal_uart_msg_new(uint8_t *data, uint32_t len);
void hal_uart_msg_del(hal_uart_msg_t *msg);

#endif
