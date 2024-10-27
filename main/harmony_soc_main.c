#include <string.h>
#include <stdio.h>

#include "hal_platform.h"
#include "hal_wifi.h"
#include "hal_uart.h"
#include "hal_ble.h"
#include "ezos.h"

#include "app_handler.h"

#define PROJECT_NAME "openharmony_esp32_master"
#define PROJECT_VERSION "V1.0.0"
#define PROJECT_AUTHOR "MjGame"
#define PROJECT_BUILD_TIME __TIME__

static void app_info_printf()
{
    printf("\r\n*******************************\r\n");
    printf("project : %s\r\n", PROJECT_NAME);
    printf("version : %s\r\n", PROJECT_VERSION);
    printf("author : %s\r\n", PROJECT_AUTHOR);
    printf("compiling time : %s\r\n", PROJECT_BUILD_TIME);
    printf("\r\n*******************************\r\n");
}

// init...
// wifi connect....
// mqtt huaiwei iot...
// ble connect...
// 启动成功
// 跳到主界面

void app_main(void)
{
    hal_platform_init();

    app_info_printf();

    hal_uart_init();

    hal_wifi_sta_init();

    hal_ble_init();

    app_proc_start();
}
