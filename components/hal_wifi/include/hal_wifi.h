#ifndef __HAL_WIFI_H__
#define __HAL_WIFI_H__
#include <stdbool.h>

int hal_wifi_sta_init(void);
int hal_wifi_sta_connect(char *ssid,char *passwd);
bool hal_wifi_sta_connect_status();

#endif
