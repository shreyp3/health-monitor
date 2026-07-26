#include <stdio.h>
#include <string.h>
#include <math.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ssd1306.h"

// ============================================================
// CONFIGURATION
// ============================================================
#define I2C_MASTER_SCL_IO       21  // SCL binds to pin 21
#define I2C_MASTER_SDA_IO       22  // SDA binds to pin 22
#define I2C_MASTER_NUM          I2C_NUM_0
#define I2C_MASTER_FREQ_HZ      100000
#define I2C_MASTER_TIMEOUT_MS   1000

#define OLED_I2C_ADDRESS        0x3C    // I2C Address of OLED screen
#define MAX30102_I2C_ADDRESS    0x57    // I2C Address of heart rate monitor
#define MLX90614_I2C_ADDRESS    0x5A    // I2C Address of skin temp monitor
#define MPU6050_I2C_ADDRESS     0x68    // I2C Address of accelerometer

// MAX30102 registers
#define MAX30102_REG_INTR_ENABLE_1  0x02
#define MAX30102_REG_FIFO_WR_PTR    0x04
#define MAX30102_REG_OVF_COUNTER    0x05
#define MAX30102_REG_FIFO_RD_PTR    0x06
#define MAX30102_REG_FIFO_DATA      0x07
#define MAX30102_REG_FIFO_CONFIG    0x08
#define MAX30102_REG_MODE_CONFIG    0x09
#define MAX30102_REG_SPO2_CONFIG    0x0A
#define MAX30102_REG_LED1_PA        0x0C
#define MAX30102_REG_LED2_PA        0x0D

// MLX90614 registers
#define MLX90614_REG_TOBJ1          0x07

// MPU6050 registers
#define MPU6050_REG_PWR_MGMT_1      0x6B
#define MPU6050_REG_ACCEL_CONFIG    0x1C
#define MPU6050_REG_ACCEL_XOUT_H    0x3B

// Heart rate thresholds
#define FINGER_THRESHOLD        50000
#define MOTION_LIGHT_THRESHOLD  0.05f
#define MOTION_ACTIVE_THRESHOLD 0.30f

// ============================================================
// BLE CONFIGURATION
// Custom UUIDs for our health monitor service
// These uniquely identify our device to the app
// ============================================================
#define HEALTH_MONITOR_SERVICE_UUID     0x00FF
#define HEALTH_MONITOR_CHAR_UUID        0xFF01
#define DEVICE_NAME                     "HealthMonitor"

#define GATTS_APP_ID                    0
#define GATTS_NUM_HANDLE                4

static const char *TAG = "HEALTH_MONITOR";

// Global vitals
static float heartrate = 0.0f;
static float pctspo2 = 0.0f;

// BLE connection state
static bool ble_connected = false;
static uint16_t ble_conn_id = 0;
static uint16_t gatts_if_global = 0;
static uint16_t char_handle = 0;
static uint16_t descr_handle = 0;
static bool notifications_enabled = false;

// ============================================================
// I2C INIT
// ============================================================
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,    // The 
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) return err;
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// ============================================================
// MAX30102 HELPERS
// ============================================================
static esp_err_t max30102_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_master_write_to_device(I2C_MASTER_NUM, MAX30102_I2C_ADDRESS,
                                       data, sizeof(data),
                                       pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

static esp_err_t max30102_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, MAX30102_I2C_ADDRESS,
                                         &reg, 1, data, len,
                                         pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

static esp_err_t max30102_init(void)
{
    esp_err_t err;
    err = max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x40);
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(100));
    err = max30102_write_reg(MAX30102_REG_FIFO_CONFIG, (0x2 << 5) | 0x10);
    if (err != ESP_OK) return err;
    err = max30102_write_reg(MAX30102_REG_FIFO_WR_PTR, 0x00);
    if (err != ESP_OK) return err;
    err = max30102_write_reg(MAX30102_REG_OVF_COUNTER, 0x00);
    if (err != ESP_OK) return err;
    err = max30102_write_reg(MAX30102_REG_FIFO_RD_PTR, 0x00);
    if (err != ESP_OK) return err;
    err = max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x03);
    if (err != ESP_OK) return err;
    err = max30102_write_reg(MAX30102_REG_SPO2_CONFIG, (0x3 << 5) | (0x3 << 2) | 0x3);
    if (err != ESP_OK) return err;
    err = max30102_write_reg(MAX30102_REG_LED1_PA, 0xA0);
    if (err != ESP_OK) return err;
    err = max30102_write_reg(MAX30102_REG_LED2_PA, 0xD0);
    if (err != ESP_OK) return err;
    ESP_LOGI(TAG, "MAX30102 initialized");
    return ESP_OK;
}

// ============================================================
// MLX90614
// ============================================================
static esp_err_t mlx90614_read_temp_f(float *temp_f)
{
    uint8_t reg = MLX90614_REG_TOBJ1;
    uint8_t buf[3];
    esp_err_t err = i2c_master_write_read_device(
        I2C_MASTER_NUM, MLX90614_I2C_ADDRESS,
        &reg, 1, buf, 3,
        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    if (err != ESP_OK) return err;
    uint16_t raw = ((uint16_t)buf[1] << 8) | buf[0];
    if (raw & 0x8000) return ESP_FAIL;
    float kelvin = raw * 0.02f;
    float celsius = kelvin - 273.15f;
    *temp_f = (celsius * 9.0f / 5.0f) + 32.0f;
    return ESP_OK;
}

// ============================================================
// MPU6050
// ============================================================
static esp_err_t mpu6050_init(void)
{
    uint8_t wake_cmd[2] = {MPU6050_REG_PWR_MGMT_1, 0x00};
    esp_err_t err = i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_I2C_ADDRESS,
                                                wake_cmd, 2,
                                                pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    if (err != ESP_OK) return err;
    uint8_t accel_cmd[2] = {MPU6050_REG_ACCEL_CONFIG, 0x00};
    return i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_I2C_ADDRESS,
                                       accel_cmd, 2,
                                       pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

static const char* mpu6050_get_activity(void)
{
    uint8_t reg = MPU6050_REG_ACCEL_XOUT_H;
    uint8_t buf[6];
    esp_err_t err = i2c_master_write_read_device(
        I2C_MASTER_NUM, MPU6050_I2C_ADDRESS,
        &reg, 1, buf, 6,
        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    if (err != ESP_OK) return "Error";
    int16_t ax = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t ay = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t az = (int16_t)((buf[4] << 8) | buf[5]);
    float gx = ax / 16384.0f;
    float gy = ay / 16384.0f;
    float gz = az / 16384.0f;
    float magnitude = sqrtf(gx*gx + gy*gy + gz*gz);
    float deviation = fabsf(magnitude - 1.0f);
    if (deviation < MOTION_LIGHT_THRESHOLD) return "Resting";
    if (deviation < MOTION_ACTIVE_THRESHOLD) return "Light";
    return "Active";
}

// ============================================================
// OLED
// ============================================================
static ssd1306_handle_t oled_init(void)
{
    ssd1306_handle_t oled = ssd1306_create(I2C_MASTER_NUM, OLED_I2C_ADDRESS);
    if (oled == NULL) return NULL;
    ssd1306_refresh_gram(oled);
    ssd1306_clear_screen(oled, 0x00);
    return oled;
}

static void display_vitals(ssd1306_handle_t oled, float bpm, float spo2,
                            float temp_f, const char *activity)
{
    char line[32];
    ssd1306_clear_screen(oled, 0x00);

    if (bpm > 0) {
        snprintf(line, sizeof(line), "HR:  %.0f BPM", bpm);
    } else {
        snprintf(line, sizeof(line), "HR:  --");
    }
    ssd1306_draw_string(oled, 0, 0, (const uint8_t *)line, 16, 1);

    if (spo2 > 0) {
        snprintf(line, sizeof(line), "O2:  %.0f%%", spo2);
    } else {
        snprintf(line, sizeof(line), "O2:  --");
    }
    ssd1306_draw_string(oled, 0, 16, (const uint8_t *)line, 16, 1);

    if (temp_f > 0) {
        snprintf(line, sizeof(line), "Tmp: %.1fF", temp_f);
    } else {
        snprintf(line, sizeof(line), "Tmp: --");
    }
    ssd1306_draw_string(oled, 0, 32, (const uint8_t *)line, 16, 1);

    snprintf(line, sizeof(line), "Mov: %s", activity);
    ssd1306_draw_string(oled, 0, 48, (const uint8_t *)line, 16, 1);

    ssd1306_refresh_gram(oled);
}

// ============================================================
// BLE ADVERTISING DATA
// This is what the phone sees when scanning for devices
// ============================================================
static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = false,
    .min_interval        = 0x0006,
    .max_interval        = 0x0010,
    .appearance          = 0x00,
    .manufacturer_len    = 0,
    .p_manufacturer_data = NULL,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = 0,
    .p_service_uuid      = NULL,
    .flag                = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// ============================================================
// GATT SERVICE DEFINITION
// Defines the structure of our BLE service
// ============================================================
static const uint16_t primary_service_uuid     = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_notify          = ESP_GATT_CHAR_PROP_BIT_NOTIFY |
                                                  ESP_GATT_CHAR_PROP_BIT_READ;
static const uint16_t health_service_uuid      = HEALTH_MONITOR_SERVICE_UUID;
static const uint16_t health_char_uuid         = HEALTH_MONITOR_CHAR_UUID;
static const uint8_t health_char_value[1]      = {0};
static uint8_t health_cccd[2]                  = {0x00, 0x00};

static const esp_gatts_attr_db_t gatt_db[] = {
    // Service declaration
    [0] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid,
         ESP_GATT_PERM_READ,
         sizeof(uint16_t), sizeof(health_service_uuid),
         (uint8_t *)&health_service_uuid}
    },
    // Characteristic declaration
    [1] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
         ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(uint8_t),
         (uint8_t *)&char_prop_notify}
    },
    // Characteristic value
    [2] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&health_char_uuid,
         ESP_GATT_PERM_READ,
         128, sizeof(health_char_value),
         (uint8_t *)health_char_value}
    },
    // Client Characteristic Configuration Descriptor (CCCD)
    // This is what the app writes to to enable notifications
    [3] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid,
         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
         sizeof(uint16_t), sizeof(health_cccd),
         (uint8_t *)health_cccd}
    },
};

// ============================================================
// GAP EVENT HANDLER
// Handles advertising and connection events
// ============================================================
static void gap_event_handler(esp_gap_ble_cb_event_t event,
                               esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "BLE advertising started");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        ESP_LOGI(TAG, "BLE advertising stopped");
        break;
    default:
        break;
    }
}

// ============================================================
// GATTS EVENT HANDLER
// Handles GATT server events — connections, reads, writes
// ============================================================
static void gatts_event_handler(esp_gatts_cb_event_t event,
                                  esp_gatt_if_t gatts_if,
                                  esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        // Registration complete — set device name and configure advertising
        esp_ble_gap_set_device_name(DEVICE_NAME);
        esp_ble_gap_config_adv_data(&adv_data);
        esp_ble_gatts_create_attr_tab(gatt_db, gatts_if,
                                       sizeof(gatt_db)/sizeof(gatt_db[0]),
                                       GATTS_APP_ID);
        gatts_if_global = gatts_if;
        break;

    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        // Attribute table created — start the service
        if (param->add_attr_tab.status == ESP_GATT_OK) {
            char_handle  = param->add_attr_tab.handles[2];
            descr_handle = param->add_attr_tab.handles[3];
            esp_ble_gatts_start_service(param->add_attr_tab.handles[0]);
            ESP_LOGI(TAG, "GATT service started");
        }
        break;

    case ESP_GATTS_CONNECT_EVT:
        ble_connected = true;
        ble_conn_id = param->connect.conn_id;
        ESP_LOGI(TAG, "BLE client connected");
        // Stop advertising when connected
        esp_ble_gap_stop_advertising();
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        ble_connected = false;
        notifications_enabled = false;
        ESP_LOGI(TAG, "BLE client disconnected — restarting advertising");
        esp_ble_gap_start_advertising(&adv_params);
        break;

    case ESP_GATTS_WRITE_EVT:
        // App wrote to CCCD to enable/disable notifications
        if (param->write.handle == descr_handle) {
            uint16_t cccd_val = param->write.value[0] |
                                 (param->write.value[1] << 8);
            notifications_enabled = (cccd_val == 0x0001);
            ESP_LOGI(TAG, "Notifications %s",
                     notifications_enabled ? "enabled" : "disabled");
        }
        break;

    default:
        break;
    }
}

// ============================================================
// SEND VITALS OVER BLE
// Packages all vitals as JSON and sends as BLE notification
// ============================================================
static void ble_send_vitals(float hr, float spo2, float temp, const char *motion)
{
    if (!ble_connected || !notifications_enabled) return;

    char json[128];
    snprintf(json, sizeof(json),
             "{\"hr\":%.0f,\"spo2\":%.0f,\"temp\":%.1f,\"motion\":\"%s\"}",
             hr, spo2, temp, motion);

    esp_ble_gatts_send_indicate(gatts_if_global, ble_conn_id,
                                 char_handle,
                                 strlen(json), (uint8_t *)json,
                                 false);

    ESP_LOGI(TAG, "BLE sent: %s", json);
}

// ============================================================
// HEART RATE TASK
// ============================================================
static void max30102_task(void *pvParameters)
{
    float firxv[5] = {0}, firyv[5] = {0};
    float fredxv[5] = {0}, fredyv[5] = {0};
    float hrarray[5] = {0};
    float spo2array[5] = {0};
    int hrarraycnt = 0;
    float meastime = 0.0f;
    float lastmeastime = 0.0f;
    int tcnt = 0;

    while (1) {
        uint8_t wptr = 0, rptr = 0;
        max30102_read_reg(MAX30102_REG_FIFO_WR_PTR, &wptr, 1);
        max30102_read_reg(MAX30102_REG_FIFO_RD_PTR, &rptr, 1);
        int samp = ((32 + wptr) - rptr) % 32;

        if (samp > 0) {
            uint8_t regdata[6 * 32];
            max30102_read_reg(MAX30102_REG_FIFO_DATA, regdata, 6 * samp);

            for (int cnt = 0; cnt < samp; cnt++) {
                meastime = 0.01f * tcnt++;

                uint32_t red_raw = (256*256*(regdata[6*cnt+0] & 0x03) +
                                    256*regdata[6*cnt+1] +
                                    regdata[6*cnt+2]);
                uint32_t ir_raw  = (256*256*(regdata[6*cnt+3] & 0x03) +
                                    256*regdata[6*cnt+4] +
                                    regdata[6*cnt+5]);

                if (ir_raw < FINGER_THRESHOLD) {
                    heartrate = 0.0f;
                    pctspo2 = 0.0f;
                    continue;
                }

                firxv[0] = firxv[1]; firxv[1] = firxv[2];
                firxv[2] = firxv[3]; firxv[3] = firxv[4];
                firxv[4] = (float)ir_raw / 3.48311f;

                firyv[0] = firyv[1]; firyv[1] = firyv[2];
                firyv[2] = firyv[3]; firyv[3] = firyv[4];
                firyv[4] = (firxv[0] + firxv[4]) - 2.0f * firxv[2]
                           + (-0.1718123813f * firyv[0])
                           + ( 0.3686645260f * firyv[1])
                           + (-1.1718123813f * firyv[2])
                           + ( 1.9738037992f * firyv[3]);

                fredxv[0] = fredxv[1]; fredxv[1] = fredxv[2];
                fredxv[2] = fredxv[3]; fredxv[3] = fredxv[4];
                fredxv[4] = (float)red_raw / 3.48311f;

                fredyv[0] = fredyv[1]; fredyv[1] = fredyv[2];
                fredyv[2] = fredyv[3]; fredyv[3] = fredyv[4];
                fredyv[4] = (fredxv[0] + fredxv[4]) - 2.0f * fredxv[2]
                            + (-0.1718123813f * fredyv[0])
                            + ( 0.3686645260f * fredyv[1])
                            + (-1.1718123813f * fredyv[2])
                            + ( 1.9738037992f * fredyv[3]);

                if (-1.0f * firyv[4] >= 100.0f &&
                    -1.0f * firyv[2] > -1.0f * firyv[0] &&
                    -1.0f * firyv[2] > -1.0f * firyv[4] &&
                    meastime - lastmeastime > 0.5f) {

                    hrarray[hrarraycnt % 5] = 60.0f / (meastime - lastmeastime);
                    float spo2 = 110.0f - 25.0f * ((fredyv[4] / fredxv[4]) /
                                                     (firyv[4] / firxv[4]));
                    if (spo2 > 100.0f) spo2 = 99.9f;
                    spo2array[hrarraycnt % 5] = spo2;
                    lastmeastime = meastime;
                    hrarraycnt++;

                    float hr_sum = 0, spo2_sum = 0;
                    for (int i = 0; i < 5; i++) {
                        hr_sum += hrarray[i];
                        spo2_sum += spo2array[i];
                    }
                    heartrate = hr_sum / 5.0f;
                    pctspo2 = spo2_sum / 5.0f;

                    if (heartrate < 40.0f || heartrate > 150.0f) heartrate = 0.0f;
                    if (pctspo2 < 50.0f || pctspo2 > 101.0f) pctspo2 = 0.0f;

                    ESP_LOGI(TAG, "HR: %.1f  SpO2: %.1f", heartrate, pctspo2);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ============================================================
// MAIN
// ============================================================
void app_main(void)
{
    // Initialize NVS — required for BLE
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_err_t err = i2c_master_init();
    if (err != ESP_OK) { ESP_LOGE(TAG, "I2C init failed"); return; }

    ssd1306_handle_t oled = oled_init();
    if (oled == NULL) { ESP_LOGE(TAG, "OLED init failed"); return; }

    err = max30102_init();
    if (err != ESP_OK) { ESP_LOGE(TAG, "MAX30102 init failed"); return; }

    err = mpu6050_init();
    if (err != ESP_OK) { ESP_LOGE(TAG, "MPU6050 init failed"); return; }

    // Initialize BLE
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(GATTS_APP_ID));

    esp_ble_gap_set_prefer_conn_params(
        (esp_bd_addr_t){0}, // applies to all connections
        0x18,  // min interval 30ms
        0x28,  // max interval 50ms
        0,     // latency
        400    // timeout 4 seconds
    );

    ESP_LOGI(TAG, "All systems initialized");

    xTaskCreate(max30102_task, "max30102_task", 4096, NULL, 5, NULL);

    float temp_f = 0.0f;
    const char *activity = "Resting";

    while (1) {
        err = mlx90614_read_temp_f(&temp_f);
        if (err != ESP_OK) temp_f = 0.0f;

        activity = mpu6050_get_activity();

        display_vitals(oled, heartrate, pctspo2, temp_f, activity);

        // Send vitals over BLE
        ble_send_vitals(heartrate, pctspo2, temp_f, activity);

        ESP_LOGI(TAG, "Temp: %.1fF  Activity: %s  HR: %.1f  SpO2: %.1f",
                 temp_f, activity, heartrate, pctspo2);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}