#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "hal_ble.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "ezos.h"

#define GATTC_TAG "BLE"

static esp_ble_ext_scan_params_t ext_scan_params = {
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE,
    .cfg_mask = ESP_BLE_GAP_EXT_SCAN_CFG_UNCODE_MASK | ESP_BLE_GAP_EXT_SCAN_CFG_CODE_MASK,
    .uncoded_cfg = {BLE_SCAN_TYPE_ACTIVE, 40, 40},
    .coded_cfg = {BLE_SCAN_TYPE_ACTIVE, 40, 40},
};

const esp_ble_gap_conn_params_t phy_1m_conn_params = {
    .scan_interval = 0x40,
    .scan_window = 0x40,
    .interval_min = 320,
    .interval_max = 320,
    .latency = 0,
    .supervision_timeout = 600,
    .min_ce_len = 0,
    .max_ce_len = 0,
};
const esp_ble_gap_conn_params_t phy_2m_conn_params = {
    .scan_interval = 0x40,
    .scan_window = 0x40,
    .interval_min = 320,
    .interval_max = 320,
    .latency = 0,
    .supervision_timeout = 600,
    .min_ce_len = 0,
    .max_ce_len = 0,
};
const esp_ble_gap_conn_params_t phy_coded_conn_params = {
    .scan_interval = 0x40,
    .scan_window = 0x40,
    .interval_min = 320, // 306-> 362Kbps
    .interval_max = 320,
    .latency = 0,
    .supervision_timeout = 600,
    .min_ce_len = 0,
    .max_ce_len = 0,
};

static uint16_t g_gattc_if = ESP_GATT_IF_NONE;

#define HAL_MASTER_CONNECT_MAX 2

#define REMOTE_SERVICE_UUID 0x00FF
static esp_bt_uuid_t g_remote_filter_service_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = REMOTE_SERVICE_UUID,
    },
};

typedef struct
{
    char adv_name[32];
    uint8_t isfull;
    uint8_t isConnect;
    esp_bd_addr_t record_addr;
    uint16_t conn_id;
    uint8_t con_status;
} ble_connect_remote_dev_t;

ble_connect_remote_dev_t g_remote_tabs[HAL_MASTER_CONNECT_MAX] = {0};

static void ble_scan_start_switch(uint8_t OnOff);
static int hal_ble_set_status(char *dev_name, uint8_t status);
static int hal_ble_set_status_by_addr(esp_bd_addr_t addr, uint8_t status);
static int hal_ble_get_status(char *dev_name, uint8_t *status);
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_EXT_ADV_REPORT_EVT:
    {
        uint8_t *adv_name = NULL;
        uint8_t adv_name_len = 0;

        adv_name = esp_ble_resolve_adv_data_by_type(param->ext_adv_report.params.adv_data,
                                                    param->ext_adv_report.params.adv_data_len,
                                                    ESP_BLE_AD_TYPE_NAME_CMPL,
                                                    &adv_name_len);

        for (int i = 0; i < HAL_MASTER_CONNECT_MAX; i++)
        {
            if (g_remote_tabs[i].isfull == 1)
            {
                if (strlen(g_remote_tabs[i].adv_name) == adv_name_len && strncmp((char *)adv_name, g_remote_tabs[i].adv_name, adv_name_len) == 0)
                {
                    esp_ble_gap_prefer_ext_connect_params_set(param->ext_adv_report.params.addr,
                                                              ESP_BLE_GAP_PHY_1M_PREF_MASK | ESP_BLE_GAP_PHY_2M_PREF_MASK | ESP_BLE_GAP_PHY_CODED_PREF_MASK,
                                                              &phy_1m_conn_params, &phy_2m_conn_params, &phy_coded_conn_params);
                    memcpy(&g_remote_tabs[i].record_addr, &param->ext_adv_report.params.addr, sizeof(esp_bd_addr_t));
                    ESP_LOGI(GATTC_TAG, "find adv device %s\r\n", g_remote_tabs[i].adv_name);
                    uint8_t status = 0;
                    int ret = hal_ble_get_status(g_remote_tabs[i].adv_name, &status);
                    if (ret != 0)
                    {
                        break;
                    }

                    if (status == HAL_BLE_STATUS_CONNECTED)
                    {
                        break;
                    }

                    ret = hal_ble_set_status(g_remote_tabs[i].adv_name, HAL_BLE_STATUS_CONNECTING);
                    if (ret != 0)
                    {
                        ESP_LOGE(GATTC_TAG, "set status error ");
                        break;
                    }
                    ble_scan_start_switch(0);

                    esp_ble_gattc_aux_open(g_gattc_if,
                                           param->ext_adv_report.params.addr,
                                           param->ext_adv_report.params.addr_type, true);
                    break;
                }
            }
        }
    }
    break;
    case ESP_GAP_BLE_EXT_SCAN_START_COMPLETE_EVT:
        if (param->ext_scan_start.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GATTC_TAG, "Extended scanning start failed, status %x", param->ext_scan_start.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "Extended scanning start successfully");
        break;
    case ESP_GAP_BLE_EXT_SCAN_STOP_COMPLETE_EVT:
        if (param->ext_scan_stop.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GATTC_TAG, "Scanning stop failed, status %x", param->ext_scan_stop.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "Scanning stop successfully");
        break;
    default:
        break;
    }
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;
    switch (event)
    {
    case ESP_GATTC_REG_EVT:
        g_gattc_if = gattc_if;
        esp_ble_gap_config_local_privacy(true);
        break;

    case ESP_GATTC_CONNECT_EVT:
        ESP_LOGI(GATTC_TAG, "Connected, conn_id %d, remote " ESP_BD_ADDR_STR "", p_data->connect.conn_id,
                 ESP_BD_ADDR_HEX(p_data->connect.remote_bda));
        break;
    case ESP_GATTC_OPEN_EVT:
        if (param->open.status != ESP_GATT_OK)
        {
            ESP_LOGE(GATTC_TAG, "Open failed, status %x", p_data->open.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "Open successfully, MTU %d", p_data->open.mtu);
        // gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->open.conn_id;

        // memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->open.remote_bda, sizeof(esp_bd_addr_t));

        hal_ble_set_status_by_addr(p_data->open.remote_bda, HAL_BLE_STATUS_CONNECTED);

        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req(gattc_if, p_data->open.conn_id);
        if (mtu_ret)
        {
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        ESP_LOGI(GATTC_TAG, "MTU exchange, status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        break;
    case ESP_GATTC_DIS_SRVC_CMPL_EVT:
        if (param->dis_srvc_cmpl.status != ESP_GATT_OK)
        {
            ESP_LOGE(GATTC_TAG, "Service discover failed, status %d", param->dis_srvc_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "Service discover complete, conn_id %d", param->dis_srvc_cmpl.conn_id);
        esp_ble_gattc_search_service(g_gattc_if, param->cfg_mtu.conn_id, &g_remote_filter_service_uuid);
        break;
    case ESP_GATTC_SEARCH_RES_EVT:
    {
        ESP_LOGI(GATTC_TAG, "Service search result, conn_id %x, is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        // if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID)
        // {
        //     ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
        //     get_service = true;
        //     gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
        //     gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
        // }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
        if (p_data->search_cmpl.status != ESP_GATT_OK)
        {
            ESP_LOGE(GATTC_TAG, "Service search failed, status %x", p_data->search_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "Service search complete");
        if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE)
        {
            ESP_LOGI(GATTC_TAG, "Get service information from remote device");
        }
        else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH)
        {
            ESP_LOGI(GATTC_TAG, "Get service information from flash");
        }
        else
        {
            ESP_LOGI(GATTC_TAG, "unknown service source");
        }
        break;

    case ESP_GATTC_DISCONNECT_EVT:
        ESP_LOGI(GATTC_TAG, "Disconnected, remote " ESP_BD_ADDR_STR ", reason 0x%02x",
                 ESP_BD_ADDR_HEX(p_data->disconnect.remote_bda), p_data->disconnect.reason);

        hal_ble_set_status_by_addr(p_data->disconnect.remote_bda, HAL_BLE_STATUS_DISCONNECTED);
        break;
    default:
        break;
    }
}

int hal_ble_set_connect_devname(char *adv_name)
{
    if (adv_name == NULL)
        return -1;

    int adv_len = strlen(adv_name);

    for (int i = 0; i < HAL_MASTER_CONNECT_MAX; i++)
    {
        // 先判断是否重复
        if (g_remote_tabs[i].isfull == 1)
        {
            if (strcmp(adv_name, g_remote_tabs[i].adv_name) == 0)
            {
                return -1;
            }
        }
    }

    for (int i = 0; i < HAL_MASTER_CONNECT_MAX; i++)
    {
        if (g_remote_tabs[i].isfull == 0)
        {
            memcpy(g_remote_tabs[i].adv_name, adv_name, adv_len);
            g_remote_tabs[i].isfull = 1;
            return 0;
        }
    }
    return -1;
}

int hal_ble_del_connect_devname(char *adv_name)
{
    if (adv_name == NULL)
        return -1;
    for (int i = 0; i < HAL_MASTER_CONNECT_MAX; i++)
    {
        if (g_remote_tabs[i].isfull)
        {
            if (strcmp(adv_name, g_remote_tabs[i].adv_name) == 0)
            {
                g_remote_tabs[i].isfull = 0;
                memset(g_remote_tabs[i].adv_name, 0, 32);
                return 0;
            }
        }
    }
    return -1;
}

static int hal_ble_set_status(char *dev_name, uint8_t status)
{
    if (dev_name == NULL)
        return -1;

    for (int i = 0; i < HAL_MASTER_CONNECT_MAX; i++)
    {
        if (g_remote_tabs[i].isfull == 1)
        {
            if (strcmp(dev_name, g_remote_tabs[i].adv_name) == 0)
            {

                g_remote_tabs[i].con_status = status;
                ESP_LOGW(GATTC_TAG, "set status %d", status);

                return 0;
            }
        }
    }

    return -1;
}

static int hal_ble_get_status(char *dev_name, uint8_t *status)
{
    if (dev_name == NULL)
        return -1;

    for (int i = 0; i < HAL_MASTER_CONNECT_MAX; i++)
    {
        if (g_remote_tabs[i].isfull == 1)
        {
            if (strcmp(dev_name, g_remote_tabs[i].adv_name) == 0)
            {
                *status = g_remote_tabs[i].con_status;
                ESP_LOGW(GATTC_TAG, "get status %d", *status);

                return 0;
            }
        }
    }

    return -1;
}

static int hal_ble_set_status_by_addr(esp_bd_addr_t addr, uint8_t status)
{

    for (int i = 0; i < HAL_MASTER_CONNECT_MAX; i++)
    {
        // 先判断是否重复
        if (g_remote_tabs[i].isfull == 1)
        {
            if (memcmp(addr, g_remote_tabs[i].record_addr, 6) == 0)
            {

                g_remote_tabs[i].con_status = status;
                ESP_LOGW(GATTC_TAG, "set status %d", status);
                return 0;
            }
        }
    }

    return -1;
}

static void ble_scan_start_switch(uint8_t OnOff)
{
    static uint8_t scan_start_flag = 0;

    if (OnOff)
    {
        if (scan_start_flag == 0)
        {
            esp_ble_gap_start_ext_scan(0, 0);
            scan_start_flag = 1;
        }
    }
    else
    {
        if (scan_start_flag == 1)
        {
            esp_ble_gap_stop_ext_scan();
            scan_start_flag = 0;
        }
    }
    ESP_LOGI(GATTC_TAG, "scan_start_flag %d", scan_start_flag);
}

void hal_ble_func_check_timer_cb(void *arg)
{
    uint8_t scan_flag = 0;
    for (int i = 0; i < HAL_MASTER_CONNECT_MAX; i++)
    {
        if (g_remote_tabs[i].isfull)
        {
            if (g_remote_tabs[i].con_status == HAL_BLE_STATUS_CONNECTING)
            {
                scan_flag = 0;
                break;
            }
            else if (g_remote_tabs[i].con_status == HAL_BLE_STATUS_DISCONNECTED)
            {
                scan_flag = 1;
            }
        }
    }

    ble_scan_start_switch(scan_flag);
}

int hal_ble_init(void)
{
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret)
    {
        ESP_LOGE(GATTC_TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
        return -1;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret)
    {
        ESP_LOGE(GATTC_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return -1;
    }

    ret = esp_bluedroid_init();
    if (ret)
    {
        ESP_LOGE(GATTC_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return -1;
    }

    ret = esp_bluedroid_enable();
    if (ret)
    {
        ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return -1;
    }
    // register the  callback function to the gap module
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret)
    {
        ESP_LOGE(GATTC_TAG, "%s gap register failed, error code = %x", __func__, ret);
        return -1;
    }

    // register the callback function to the gattc module
    ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    if (ret)
    {
        ESP_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x", __func__, ret);
        return -1;
    }

    ret = esp_ble_gattc_app_register(0);
    if (ret)
    {
        ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x", __func__, ret);
    }
    ret = esp_ble_gatt_set_local_mtu(500);
    if (ret)
    {
        ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", ret);
    }

    ret = esp_ble_gap_set_ext_scan_params(&ext_scan_params);
    if (ret)
    {
        ESP_LOGE(GATTC_TAG, "Set extend scan params error, error code = %x", ret);
    }

    ;

    ezos_timer_start(ezos_timer_create(hal_ble_func_check_timer_cb, 0, 1), 100);

    return 0;
}
