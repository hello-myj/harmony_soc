#ifndef __HAL_BLE_H__
#define __HAL_BLE_H__



enum
{
    HAL_BLE_STATUS_NONE,
    HAL_BLE_STATUS_NOTHING,
    HAL_BLE_STATUS_SCANNING,
    HAL_BLE_STATUS_CONNECTED,
};

int hal_ble_init(void);
int hal_ble_scan_start();
int hal_ble_scan_stop();
int hal_ble_connect(char *adv_name);

#endif


