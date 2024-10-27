#include <stdio.h>
#include <string.h>
#include "hal_uart.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "freertos/queue.h"

#define ECHO_TEST_TXD (3)
#define ECHO_TEST_RXD (4)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM (1)
#define ECHO_UART_BAUD_RATE (115200)
#define ECHO_TASK_STACK_SIZE (3072)

#define BUF_SIZE (1024)

static QueueHandle_t g_uart_xQueue;

static void uart_task(void *arg)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *)malloc(BUF_SIZE);

    while (1)
    {
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            hal_uart_msg_t *msg = hal_uart_msg_new(data, len);
            if (msg != NULL)
                xQueueSend(g_uart_xQueue, &msg, 10 / portTICK_PERIOD_MS);
        }

        // Write data back to the UART
        // uart_write_bytes(ECHO_UART_PORT_NUM, (const char *)data, len);
        // if (len)
        // {
        //     data[len] = '\0';
        //     ESP_LOGI(TAG, "Recv str: %s", (char *)data);
        // }
    }
}

hal_uart_msg_t *hal_uart_msg_new(uint8_t *data, uint32_t len)
{
    uint32_t cur_len = len > MAX_UART_LEN ? MAX_UART_LEN : len;
    hal_uart_msg_t *msg = (hal_uart_msg_t *)malloc(sizeof(hal_uart_msg_t));
    if (msg == NULL)
        return NULL;
    msg->len = cur_len;
    memcpy(msg->data, data, cur_len);

    return msg;
}

void hal_uart_msg_del(hal_uart_msg_t *msg)
{
    free(msg);
}

int hal_uart_init(void)
{
    xTaskCreate(uart_task, "uart_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
    g_uart_xQueue = xQueueCreate(10, sizeof(hal_uart_msg_t *));

    return 0;
}

int hal_uart_send(uint8_t *buf, uint32_t len)
{
    return uart_write_bytes(ECHO_UART_PORT_NUM, buf, len);
}

int hal_uart_recv(uint8_t *buf, uint32_t buf_len, uint32_t timeout)
{

    hal_uart_msg_t *msg = NULL;
    if (xQueueReceive(g_uart_xQueue, &msg, timeout) == pdTRUE)
    {
        uint32_t cur_len = buf_len < msg->len ? buf_len : msg->len;
        if (buf != NULL)
        {
            memcpy(buf, msg->data, cur_len);
        }
        hal_uart_msg_del(msg);
        return cur_len;
    }
    return 0;
}