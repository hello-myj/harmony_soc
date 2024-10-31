#ifndef __HAL_BLE_H__
#define __HAL_BLE_H__



enum
{
    HAL_BLE_STATUS_DISCONNECTED=0,
    HAL_BLE_STATUS_CONNECTING,
    HAL_BLE_STATUS_CONNECTED,
};

int hal_ble_init(void);
int hal_ble_set_connect_devname(char *adv_name);
int hal_ble_del_connect_devname(char *adv_name);


#endif


