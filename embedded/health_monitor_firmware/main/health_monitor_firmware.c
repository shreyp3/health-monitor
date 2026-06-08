#include <stdio.h>
#include <string.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ssd1306.h"

// ============================================================
// CONFIGURATION
// ============================================================
#define I2C_MASTER_SCL_IO       21
#define I2C_MASTER_SDA_IO       22
#define I2C_MASTER_NUM          I2C_NUM_0
#define I2C_MASTER_FREQ_HZ      100000
#define I2C_MASTER_TIMEOUT_MS   1000

#define OLED_I2C_ADDRESS        0x3C
#define MAX30102_I2C_ADDRESS    0x57
#define MLX90614_I2C_ADDRESS    0x5A

// MAX30102 Register addresses
#define MAX30102_REG_INTR_STATUS_1  0x00
#define MAX30102_REG_INTR_ENABLE_1  0x02
#define MAX30102_REG_FIFO_WR_PTR    0x04
#define MAX30102_REG_OVF_COUNTER    0x05
#define MAX30102_REG_FIFO_RD_PTR    0x06
#define MAX30102_REG_FIFO_DATA      0x07
#define MAX30102_REG_MODE_CONFIG    0x09
#define MAX30102_REG_SPO2_CONFIG    0x0A
#define MAX30102_REG_LED1_PA        0x0C
#define MAX30102_REG_LED2_PA        0x0D

// MLX90614 Register addresses
#define MLX90614_REG_TA             0x06  // Ambient temperature
#define MLX90614_REG_TOBJ1          0x07  // Object temperature (skin)

#define SAMPLE_BUFFER_SIZE  100
#define SAMPLE_RATE_MS      10
#define FINGER_THRESHOLD    50000

static const char *TAG = "HEALTH_MONITOR";

// ============================================================
// I2C INIT
// ============================================================
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
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
    err = max30102_write_reg(MAX30102_REG_INTR_ENABLE_1, 0xC0);
    if (err != ESP_OK) return err;
    err = max30102_write_reg(MAX30102_REG_FIFO_WR_PTR, 0x00);
    if (err != ESP_OK) return err;
    err = max30102_write_reg(MAX30102_REG_OVF_COUNTER, 0x00);
    if (err != ESP_OK) return err;
    err = max30102_write_reg(MAX30102_REG_FIFO_RD_PTR, 0x00);
    if (err != ESP_OK) return err;
    err = max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x03);
    if (err != ESP_OK) return err;
    err = max30102_write_reg(MAX30102_REG_SPO2_CONFIG, 0x27);
    if (err != ESP_OK) return err;
    err = max30102_write_reg(MAX30102_REG_LED1_PA, 0xFF);
    if (err != ESP_OK) return err;
    err = max30102_write_reg(MAX30102_REG_LED2_PA, 0xFF);
    if (err != ESP_OK) return err;
    ESP_LOGI(TAG, "MAX30102 initialized");
    return ESP_OK;
}

// ============================================================
// MLX90614 — read object temperature in Fahrenheit
// The MLX90614 uses SMBus protocol which is I2C compatible
// Each register returns a 16-bit raw value
// Temperature in Kelvin = raw * 0.02
// Temperature in Celsius = Kelvin - 273.15
// Temperature in Fahrenheit = Celsius * 9/5 + 32
// ============================================================
static esp_err_t mlx90614_read_temp_f(float *temp_f)
{
    uint8_t reg = MLX90614_REG_TOBJ1;
    uint8_t buf[3];

    // MLX90614 returns 3 bytes: data low, data high, PEC (packet error check)
    esp_err_t err = i2c_master_write_read_device(
        I2C_MASTER_NUM, MLX90614_I2C_ADDRESS,
        &reg, 1, buf, 3,
        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS)
    );
    if (err != ESP_OK) return err;

    // Combine low and high bytes into raw 16-bit value
    uint16_t raw = ((uint16_t)buf[1] << 8) | buf[0];

    // Check error flag — bit 15 set means sensor error
    if (raw & 0x8000) return ESP_FAIL;

    // Convert raw to Kelvin, then Celsius, then Fahrenheit
    float kelvin = raw * 0.02f;
    float celsius = kelvin - 273.15f;
    *temp_f = (celsius * 9.0f / 5.0f) + 32.0f;

    return ESP_OK;
}

// ============================================================
// OLED INIT
// ============================================================
static ssd1306_handle_t oled_init(void)
{
    ssd1306_handle_t oled = ssd1306_create(I2C_MASTER_NUM, OLED_I2C_ADDRESS);
    if (oled == NULL) return NULL;
    ssd1306_refresh_gram(oled);
    ssd1306_clear_screen(oled, 0x00);
    return oled;
}

// ============================================================
// DISPLAY — shows HR standby and live skin temperature
// ============================================================
static void display_vitals(ssd1306_handle_t oled, float temp_f)
{
    char line[32];
    ssd1306_clear_screen(oled, 0x00);

    // Heart rate — standby for now
    snprintf(line, sizeof(line), "HR: Standby");
    ssd1306_draw_string(oled, 0, 0, (const uint8_t *)line, 16, 1);

    // Skin temperature
    if (temp_f > 0) {
        snprintf(line, sizeof(line), "Temp: %.1fF", temp_f);
    } else {
        snprintf(line, sizeof(line), "Temp: --");
    }
    ssd1306_draw_string(oled, 0, 16, (const uint8_t *)line, 16, 1);

    // Status line
    if (temp_f > 0) {
        snprintf(line, sizeof(line), "Reading OK");
        ssd1306_draw_string(oled, 0, 32, (const uint8_t *)line, 16, 1);
    } else {
        snprintf(line, sizeof(line), "No reading");
        ssd1306_draw_string(oled, 0, 32, (const uint8_t *)line, 16, 1);
    }

    ssd1306_refresh_gram(oled);
}

// ============================================================
// MAIN
// ============================================================
void app_main(void)
{
    esp_err_t err = i2c_master_init();
    if (err != ESP_OK) { ESP_LOGE(TAG, "I2C init failed"); return; }

    ssd1306_handle_t oled = oled_init();
    if (oled == NULL) { ESP_LOGE(TAG, "OLED init failed"); return; }

    err = max30102_init();
    if (err != ESP_OK) { ESP_LOGE(TAG, "MAX30102 init failed"); return; }

    ESP_LOGI(TAG, "All sensors initialized");

    float temp_f = 0.0f;

    while (1) {
        // Read skin temperature from MLX90614
        err = mlx90614_read_temp_f(&temp_f);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Skin Temp: %.1f F", temp_f);
        } else {
            ESP_LOGE(TAG, "MLX90614 read failed");
            temp_f = 0.0f;
        }

        display_vitals(oled, temp_f);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}