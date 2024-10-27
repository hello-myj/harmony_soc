#include "tlv_protocol.h"

#include "ezos.h"

#include "hal_uart.h"

static void data_proc_task(void *pvParameters)
{

    while (1)
    {
        // ezos_printf("task1 run\r\n");
        // printf("task2 run\r\n");

      //  hal_uart_send((uint8_t *)"hello world\r\n",strlen("hello world\r\n"));
        uint8_t buf[128]={0};
        uint16_t len= hal_uart_recv(buf,sizeof(buf),0xffffffffUL);
        hal_uart_send(buf,len);
        ezos_delayms(100);
    }
}

int app_proc_start()
{

    ezos_thread_params_t tmp_param = {0};

    tmp_param.user_arg = NULL;
    tmp_param.priority = 16;
    tmp_param.thread_name = NULL;
    tmp_param.stack_size = 2048;

    ezos_thread_create(data_proc_task, &tmp_param);

    return 0;
}