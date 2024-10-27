#include <stdio.h>
#include "haiwei_iot.h"


#define IOT_SERVER "abe2519e7b.st1.iotda-device.cn-north-4.myhuaweicloud.com"
#define IOT_PORT  1883



void huawei_iot_start()
{
    hal_wifi_sta_connect();
}


