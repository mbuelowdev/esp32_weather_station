
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "blinker.h"
#include "configuration.h"

#define BT_LOG_TAG                  "BT_STACK"

#define BLUETOOTH_ADV_NAME          "Weather Station MK1"
#define ESP_APP_ID                  0x42
#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define SVC_INST_ID                 0

#define CHAR_VALUE_LENGTH_MAX       500
#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

enum
{
    IDX_SVC_CONFIGURATION,

    IDX_CHAR_DATA_SINK,
    IDX_CHAR_DATA_SINK_VALUE,

    IDX_CHAR_DATA_SINK_PUSH_FORMAT,
    IDX_CHAR_DATA_SINK_PUSH_FORMAT_VALUE,

    IDX_CHAR_MEASURMENT_RATE,
    IDX_CHAR_MEASURMENT_RATE_VALUE,

    IDX_CHAR_UPLOAD_RATE,
    IDX_CHAR_UPLOAD_RATE_VALUE,

    IDX_CHAR_WIFI_SSID,
    IDX_CHAR_WIFI_SSID_VALUE,

    IDX_CHAR_WIFI_PASSWORD,
    IDX_CHAR_WIFI_PASSWORD_VALUE,

    IDX_CHAR_WIFI_SUBTRACT_MEASURING_TIME,
    IDX_CHAR_WIFI_SUBTRACT_MEASURING_TIME_VALUE,

    IDX_CHAR_FLUSH,
    IDX_CHAR_FLUSH_VALUE,

    TABLE_MAX,
};

static uint8_t adv_config_done       = 0;

uint16_t handle_table[TABLE_MAX];

typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;
static prepare_type_env_t prepare_write_env;

esp_err_t cfgmode_start(void);

void bt_prepare_write_event(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
void bt_exec_write_event(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

static void bt_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void bt_gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void bt_gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static uint8_t service_uuid[16] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

/* The length of adv data must be less than 31 bytes */
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval        = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance          = 0x00,
    .manufacturer_len    = 0,    //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //test_manufacturer,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(service_uuid),
    .p_service_uuid      = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp        = true,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x0006,
    .max_interval        = 0x0010,
    .appearance          = 0x00,
    .manufacturer_len    = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //&test_manufacturer[0],
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(service_uuid),
    .p_service_uuid      = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

/* Service */
static const uint16_t GATTS_SERVICE_UUID_TEST                   = 0x00FF;
static const uint16_t GATTS_CHAR_UUID_DATA_SINK                 = 0xFF01;
static const uint16_t GATTS_CHAR_UUID_DATA_SINK_PUSH_FORMAT     = 0xFF02;
static const uint16_t GATTS_CHAR_UUID_MEASUREMENT_RATE          = 0xFF03;
static const uint16_t GATTS_CHAR_UUID_UPLOAD_RATE               = 0xFF04;
static const uint16_t GATTS_CHAR_UUID_WIFI_SSID                 = 0xFF05;
static const uint16_t GATTS_CHAR_UUID_WIFI_PASSWORD             = 0xFF06;
static const uint16_t GATTS_CHAR_UUID_SUBTRACT_MEASURING_TIME   = 0xFF07;
static const uint16_t GATTS_CHAR_UUID_FLUSH                     = 0xFFFF;

static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
//static const uint8_t char_prop_read                = ESP_GATT_CHAR_PROP_BIT_READ;
//static const uint8_t char_prop_write               = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_write          = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
//static const uint8_t char_prop_read_write_notify   = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;


/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_db[TABLE_MAX] =
{
    // Service Declaration
    [IDX_SVC_CONFIGURATION]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID_TEST), (uint8_t *)&GATTS_SERVICE_UUID_TEST}},

    /* Data Sink */
    /* Characteristic Declaration */
    [IDX_CHAR_DATA_SINK]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
    /* Characteristic Value */
    [IDX_CHAR_DATA_SINK_VALUE] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_DATA_SINK, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      CHAR_VALUE_LENGTH_MAX, 0, NULL}},

    /* Data Sink Push Format */
    /* Characteristic Declaration */
    [IDX_CHAR_DATA_SINK_PUSH_FORMAT]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
    /* Characteristic Value */
    [IDX_CHAR_DATA_SINK_PUSH_FORMAT_VALUE] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_DATA_SINK_PUSH_FORMAT, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      CHAR_VALUE_LENGTH_MAX, 0, NULL}},

    /* Measurement Rate */
    /* Characteristic Declaration */
    [IDX_CHAR_MEASURMENT_RATE]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
    /* Characteristic Value */
    [IDX_CHAR_MEASURMENT_RATE_VALUE] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_MEASUREMENT_RATE, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      CHAR_VALUE_LENGTH_MAX, 0, NULL}},

    /* Upload Rate */
    /* Characteristic Declaration */
    [IDX_CHAR_UPLOAD_RATE]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
    /* Characteristic Value */
    [IDX_CHAR_UPLOAD_RATE_VALUE] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_UPLOAD_RATE, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      CHAR_VALUE_LENGTH_MAX, 0, NULL}},

    /* Wifi SSID */
    /* Characteristic Declaration */
    [IDX_CHAR_WIFI_SSID]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
    /* Characteristic Value */
    [IDX_CHAR_WIFI_SSID_VALUE] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_WIFI_SSID, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      CHAR_VALUE_LENGTH_MAX, 0, NULL}},

    /* Wifi Password */
    /* Characteristic Declaration */
    [IDX_CHAR_WIFI_PASSWORD]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
    /* Characteristic Value */
    [IDX_CHAR_WIFI_PASSWORD_VALUE] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_WIFI_PASSWORD, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      CHAR_VALUE_LENGTH_MAX, 0, NULL}},

    /* Subtract Measuring Time */
    /* Characteristic Declaration */
    [IDX_CHAR_WIFI_SUBTRACT_MEASURING_TIME]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
    /* Characteristic Value */
    [IDX_CHAR_WIFI_SUBTRACT_MEASURING_TIME_VALUE] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_SUBTRACT_MEASURING_TIME, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      CHAR_VALUE_LENGTH_MAX, 0, NULL}},

    /* Flush Characteristic (used to save configuration on write) */
    /* Characteristic Declaration */
    [IDX_CHAR_FLUSH]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
    /* Characteristic Value */
    [IDX_CHAR_FLUSH_VALUE] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_FLUSH, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      CHAR_VALUE_LENGTH_MAX, 0, NULL}}
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst heart_rate_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = bt_gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

esp_err_t cfgmode_start(void)
{
    esp_err_t ret;

    /* Initialize NVS. */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(BT_LOG_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(BT_LOG_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg);
    if (ret) {
        ESP_LOGE(BT_LOG_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(BT_LOG_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_ble_gatts_register_callback(bt_gatts_event_handler);
    if (ret){
        ESP_LOGE(BT_LOG_TAG, "gatts register error, error code = %x", ret);
        return ret;
    }

    ret = esp_ble_gap_register_callback(bt_gap_event_handler);
    if (ret){
        ESP_LOGE(BT_LOG_TAG, "gap register error, error code = %x", ret);
        return ret;
    }

    ret = esp_ble_gatts_app_register(ESP_APP_ID);
    if (ret){
        ESP_LOGE(BT_LOG_TAG, "gatts app register error, error code = %x", ret);
        return ret;
    }

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(BT_LOG_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }

    return ESP_OK;
}

void bt_prepare_write_event(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(BT_LOG_TAG, "prepare write, handle = %d, value len = %d", param->write.handle, param->write.len);
    esp_gatt_status_t status = ESP_GATT_OK;
    if (prepare_write_env->prepare_buf == NULL) {
        prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
        prepare_write_env->prepare_len = 0;
        if (prepare_write_env->prepare_buf == NULL) {
            ESP_LOGE(BT_LOG_TAG, "%s, Gatt_server prep no mem", __func__);
            status = ESP_GATT_NO_RESOURCES;
        }
    } else {
        if(param->write.offset > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_OFFSET;
        } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_ATTR_LEN;
        }
    }
    /*send response when param->write.need_rsp is true */
    if (param->write.need_rsp){
        esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
        if (gatt_rsp != NULL){
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK){
               ESP_LOGE(BT_LOG_TAG, "Send response error");
            }
            free(gatt_rsp);
        }else{
            ESP_LOGE(BT_LOG_TAG, "%s, malloc failed", __func__);
            status = ESP_GATT_NO_RESOURCES;
        }
    }
    if (status != ESP_GATT_OK){
        return;
    }
    memcpy(prepare_write_env->prepare_buf + param->write.offset,
           param->write.value,
           param->write.len);
    prepare_write_env->prepare_len += param->write.len;

}

void bt_exec_write_event(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param) {
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC && prepare_write_env->prepare_buf){
        esp_log_buffer_hex(BT_LOG_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }else{
        ESP_LOGI(BT_LOG_TAG,"ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void bt_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            /* advertising start complete event to indicate advertising start successfully or failed */
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(BT_LOG_TAG, "advertising start failed");
            }else{
                ESP_LOGI(BT_LOG_TAG, "advertising start successfully");
                blinker_set_bt_discoverable();
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(BT_LOG_TAG, "Advertising stop failed");
            }
            else {
                ESP_LOGI(BT_LOG_TAG, "Stop adv successfully");
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(BT_LOG_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
            break;
        default:
            break;
    }
}

static void bt_gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            heart_rate_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        } else {
            ESP_LOGE(BT_LOG_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == heart_rate_profile_tab[idx].gatts_if) {
                if (heart_rate_profile_tab[idx].gatts_cb) {
                    heart_rate_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

static void bt_gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:{
            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(BLUETOOTH_ADV_NAME);
            if (set_dev_name_ret){
                ESP_LOGE(BT_LOG_TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }
            
            //config adv data
            esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
            if (ret){
                ESP_LOGE(BT_LOG_TAG, "config adv data failed, error code = %x", ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;

            //config scan response data
            ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
            if (ret){
                ESP_LOGE(BT_LOG_TAG, "config scan response data failed, error code = %x", ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;

            esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, TABLE_MAX, SVC_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(BT_LOG_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
        }
       	    break;
        case ESP_GATTS_READ_EVT:
            ESP_LOGI(BT_LOG_TAG, "ESP_GATTS_READ_EVT");

            if (!param->read.need_rsp) {
                ESP_LOGI(BT_LOG_TAG, "Read handled by stack");
                break;
            }

            esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
            if (gatt_rsp != NULL) {
                gatt_rsp->attr_value.handle = param->read.handle;
                gatt_rsp->attr_value.offset = param->read.offset;
                gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            } else {
                ESP_LOGE(BT_LOG_TAG, "%s, malloc failed", __func__);
                break;
            }

            if (param->read.handle == handle_table[IDX_CHAR_DATA_SINK_VALUE]) {
                gatt_rsp->attr_value.len = strlen(configuration.data_sink);
                memcpy(gatt_rsp->attr_value.value, configuration.data_sink, gatt_rsp->attr_value.len);
            } else if (param->read.handle == handle_table[IDX_CHAR_DATA_SINK_PUSH_FORMAT_VALUE]) {
                gatt_rsp->attr_value.len = 1;
                uint8_t bytes[1] = {configuration.data_sink_push_format};
                memcpy(gatt_rsp->attr_value.value, &bytes, gatt_rsp->attr_value.len);
            } else if (param->read.handle == handle_table[IDX_CHAR_MEASURMENT_RATE_VALUE]) {
                gatt_rsp->attr_value.len = 2;
                uint8_t bytes[2] = {(configuration.measurement_rate >> 8) & 0xFF, configuration.measurement_rate & 0xFF};
                memcpy(gatt_rsp->attr_value.value, &bytes, gatt_rsp->attr_value.len);
            } else if (param->read.handle == handle_table[IDX_CHAR_UPLOAD_RATE_VALUE]) {
                gatt_rsp->attr_value.len = 2;
                uint8_t bytes[2] = {(configuration.upload_rate >> 8) & 0xFF, configuration.upload_rate & 0xFF};
                memcpy(gatt_rsp->attr_value.value, &bytes, gatt_rsp->attr_value.len);
            } else if (param->read.handle == handle_table[IDX_CHAR_WIFI_SSID_VALUE]) {
                gatt_rsp->attr_value.len = strlen(configuration.wifi_ssid);
                memcpy(gatt_rsp->attr_value.value, configuration.wifi_ssid, gatt_rsp->attr_value.len);
            } else if (param->read.handle == handle_table[IDX_CHAR_WIFI_PASSWORD_VALUE]) {
                gatt_rsp->attr_value.len = strlen(configuration.wifi_password);
                memcpy(gatt_rsp->attr_value.value, configuration.wifi_password, gatt_rsp->attr_value.len);
            } else if (param->read.handle == handle_table[IDX_CHAR_WIFI_SUBTRACT_MEASURING_TIME_VALUE]) {
                gatt_rsp->attr_value.len = 1;
                uint8_t bytes[1] = {configuration.subtract_measuring_time ? 1 : 0};
                memcpy(gatt_rsp->attr_value.value, &bytes, gatt_rsp->attr_value.len);
            }  else if (param->read.handle == handle_table[IDX_CHAR_FLUSH_VALUE]) {
                gatt_rsp->attr_value.len = 0;
            }

            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, gatt_rsp);
            if (response_err != ESP_OK) {
                ESP_LOGE(BT_LOG_TAG, "Send response error");
            }

            if (gatt_rsp != NULL) {
                free(gatt_rsp);
            }

       	    break;
        case ESP_GATTS_WRITE_EVT:
            if (!param->write.is_prep){
                // the data length of gattc write  must be less than CHAR_VALUE_LENGTH_MAX.
                ESP_LOGI(BT_LOG_TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :", param->write.handle, param->write.len);
                esp_log_buffer_hex(BT_LOG_TAG, param->write.value, param->write.len);

            if (param->write.handle == handle_table[IDX_CHAR_DATA_SINK_VALUE]) {
                strncpy(configuration.data_sink, (char*) param->write.value, param->write.len);
            } else if (param->write.handle == handle_table[IDX_CHAR_DATA_SINK_PUSH_FORMAT_VALUE]) {
                configuration.data_sink_push_format = param->write.value[0];
            } else if (param->write.handle == handle_table[IDX_CHAR_MEASURMENT_RATE_VALUE]) {
                configuration.measurement_rate = (param->write.value[0] << 8) + param->write.value[1];
            } else if (param->write.handle == handle_table[IDX_CHAR_UPLOAD_RATE_VALUE]) {
                configuration.upload_rate = (param->write.value[0] << 8) + param->write.value[1];
            } else if (param->write.handle == handle_table[IDX_CHAR_WIFI_SSID_VALUE]) {
                strncpy(configuration.wifi_ssid, (char*) param->write.value, param->write.len);
            } else if (param->write.handle == handle_table[IDX_CHAR_WIFI_PASSWORD_VALUE]) {
                strncpy(configuration.wifi_password, (char*) param->write.value, param->write.len);
            } else if (param->write.handle == handle_table[IDX_CHAR_WIFI_SUBTRACT_MEASURING_TIME_VALUE]) {
                configuration.subtract_measuring_time = param->write.value[0] > 0;
            } else if (param->write.handle == handle_table[IDX_CHAR_FLUSH_VALUE]) {
                cfg_write();
            }

                /* send response when param->write.need_rsp is true*/
                if (param->write.need_rsp){
                    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                }
            }else{
                /* handle prepare write */
                bt_prepare_write_event(gatts_if, &prepare_write_env, param);
            }
      	    break;
        case ESP_GATTS_EXEC_WRITE_EVT:
            // the length of gattc prepare write data must be less than CHAR_VALUE_LENGTH_MAX.
            ESP_LOGI(BT_LOG_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
            bt_exec_write_event(&prepare_write_env, param);
            break;
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(BT_LOG_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            break;
        case ESP_GATTS_CONF_EVT:
            ESP_LOGI(BT_LOG_TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(BT_LOG_TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(BT_LOG_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            esp_log_buffer_hex(BT_LOG_TAG, param->connect.remote_bda, 6);
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the iOS system, please refer to Apple official documents about the BLE connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);
            blinker_set_bt_connected();
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(BT_LOG_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
            esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(BT_LOG_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle != TABLE_MAX){
                ESP_LOGE(BT_LOG_TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to TABLE_MAX(%d)", param->add_attr_tab.num_handle, TABLE_MAX);
            }
            else {
                ESP_LOGI(BT_LOG_TAG, "create attribute table successfully, the number handle = %d",param->add_attr_tab.num_handle);
                memcpy(handle_table, param->add_attr_tab.handles, sizeof(handle_table));
                esp_ble_gatts_start_service(handle_table[IDX_SVC_CONFIGURATION]);
            }
            break;
        }
        case ESP_GATTS_STOP_EVT:
        case ESP_GATTS_OPEN_EVT:
        case ESP_GATTS_CANCEL_OPEN_EVT:
        case ESP_GATTS_CLOSE_EVT:
        case ESP_GATTS_LISTEN_EVT:
        case ESP_GATTS_CONGEST_EVT:
        case ESP_GATTS_UNREG_EVT:
        case ESP_GATTS_DELETE_EVT:
        default:
            break;
    }
}
