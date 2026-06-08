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

// MAX30102 Register addresses — these come from the datasheet
#define MAX30102_REG_INTR_STATUS_1  0x00
#define MAX30102_REG_INTR_ENABLE_1  0x02
#define MAX30102_REG_FIFO_WR_PTR    0x04
#define MAX30102_REG_FIFO_RD_PTR    0x06
#define MAX30102_REG_FIFO_DATA      0x07
#define MAX30102_REG_MODE_CONFIG    0x09
#define MAX30102_REG_SPO2_CONFIG    0x0A
#define MAX30102_REG_LED1_PA        0x0C
#define MAX30102_REG_LED2_PA        0x0D

static const char *TAG = "HEALTH_MONITOR";

// ============================================================
// I2C INIT — same as before
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
// MAX30102 HELPER — write a single byte to a register
// You don't need to change this
// ============================================================
static esp_err_t max30102_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_master_write_to_device(I2C_MASTER_NUM, MAX30102_I2C_ADDRESS,
                                       data, sizeof(data),
                                       pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

// ============================================================
// MAX30102 HELPER — read bytes from a register
// You don't need to change this
// ============================================================
static esp_err_t max30102_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, MAX30102_I2C_ADDRESS,
                                         &reg, 1, data, len,
                                         pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

// ============================================================
// TASK 1 — Initialize the MAX30102 sensor
// Write the correct values to each register using max30102_write_reg
// The comments explain what each register does
// ============================================================
static esp_err_t max30102_init(void)
{
    esp_err_t err;

    // Reset the sensor first — write 0x40 to MODE_CONFIG register
    err = max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x40);
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(100)); // wait for reset

    // Enable FIFO almost full interrupt — write 0xC0 to INTR_ENABLE_1
    err = max30102_write_reg(MAX30102_REG_INTR_ENABLE_1, 0xC0);
    if (err != ESP_OK) return err;

    // Reset FIFO pointers — write 0x00 to FIFO_WR_PTR and FIFO_RD_PTR
    err = max30102_write_reg(MAX30102_REG_FIFO_WR_PTR, 0x00);
    if (err != ESP_OK) return err;
    err = max30102_write_reg(MAX30102_REG_FIFO_RD_PTR, 0x00);
    if (err != ESP_OK) return err;

    // Set SpO2 mode — write 0x03 to MODE_CONFIG
    // 0x03 = SpO2 mode (enables both RED and IR LEDs)
    err = max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x03);
    if (err != ESP_OK) return err;

    // Set SpO2 config — write 0x27 to SPO2_CONFIG
    // 0x27 = 100 samples/sec, 411us pulse width, 4096 ADC range
    err = max30102_write_reg(MAX30102_REG_SPO2_CONFIG, 0x27);
    if (err != ESP_OK) return err;

    // Set LED brightness — write 0x24 to LED1_PA and LED2_PA
    // 0x24 = ~7mA LED current, good for fingertip measurement
    err = max30102_write_reg(MAX30102_REG_LED1_PA, 0x24);
    if (err != ESP_OK) return err;
    err = max30102_write_reg(MAX30102_REG_LED2_PA, 0x24);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "MAX30102 initialized");
    return ESP_OK;
}

// ============================================================
// TASK 2 — Read one sample from the FIFO buffer
// Each sample contains 3 bytes RED + 3 bytes IR = 6 bytes total
// You don't need to change this function
// ============================================================
static esp_err_t max30102_read_fifo(uint32_t *red, uint32_t *ir)
{
    uint8_t buf[6];
    esp_err_t err = max30102_read_reg(MAX30102_REG_FIFO_DATA, buf, 6);
    if (err != ESP_OK) return err;

    // Each value is 18 bits packed into 3 bytes, mask off top 6 bits
    *red = ((uint32_t)(buf[0] & 0x03) << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    *ir  = ((uint32_t)(buf[3] & 0x03) << 16) | ((uint32_t)buf[4] << 8) | buf[5];

    return ESP_OK;
}

// ============================================================
// OLED INIT — same as before
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
// DISPLAY — show raw RED and IR values on OLED
// ============================================================
static void display_raw(ssd1306_handle_t oled, uint32_t red, uint32_t ir)
{
    char line[32];
    ssd1306_clear_screen(oled, 0x00);

    snprintf(line, sizeof(line), "RED: %lu", red);
    ssd1306_draw_string(oled, 0, 0, (const uint8_t *)line, 16, 1);

    snprintf(line, sizeof(line), "IR:  %lu", ir);
    ssd1306_draw_string(oled, 0, 16, (const uint8_t *)line, 16, 1);

    snprintf(line, sizeof(line), "Place finger");
    ssd1306_draw_string(oled, 0, 32, (const uint8_t *)line, 16, 1);

    snprintf(line, sizeof(line), "on sensor...");
    ssd1306_draw_string(oled, 0, 48, (const uint8_t *)line, 16, 1);

    ssd1306_refresh_gram(oled);
}

// ============================================================
// MAIN
// ============================================================
void app_main(void)
{
    // Initialize I2C
    esp_err_t err = i2c_master_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed");
        return;
    }

    // Initialize OLED
    ssd1306_handle_t oled = oled_init();
    if (oled == NULL) {
        ESP_LOGE(TAG, "OLED init failed");
        return;
    }

    // Initialize MAX30102
    err = max30102_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MAX30102 init failed");
        return;
    }

    // Main loop — read raw values and display them
    uint32_t red = 0, ir = 0;
    while (1) {
        err = max30102_read_fifo(&red, &ir);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "RED: %lu  IR: %lu", red, ir);
            display_raw(oled, red, ir);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}