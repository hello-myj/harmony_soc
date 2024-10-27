#include "app_handler.h"
#include <stdlib.h>
#include <stdint.h>

int uart_wifi_param_get(app_wifi_param_t *param)
{
    if (param == NULL)
    {
        return -1;
    }

    //   return uart_read_param(param);

    return 0;
}

int uart_tlv_send(uint8_t *buf, int len)
{
     return 0;
}

int uart_tlv_recv(uint8_t *buf, int len)
{
     return 0;
}