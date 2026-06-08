#include <stdio.h>
#include <string.h>
#include "driver/i2c.h"
#include "esp_log.h"
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
#define OLED_WIDTH              128
#define OLED_HEIGHT             64

static const char *TAG = "HEALTH_MONITOR";

// ============================================================
// TASK 1 — I2C initialization (same as before, don't change)
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
// TASK 2 — Initialize the OLED display
// You don't need to change this function
// ============================================================
static ssd1306_handle_t oled_init(void)
{
    ssd1306_handle_t ssd1306_dev = ssd1306_create(I2C_MASTER_NUM, OLED_I2C_ADDRESS);
    if (ssd1306_dev == NULL) {
        ESP_LOGE(TAG, "Failed to create SSD1306 device");
        return NULL;
    }
    esp_err_t err = ssd1306_refresh_gram(ssd1306_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display");
        return NULL;
    }
    ssd1306_clear_screen(ssd1306_dev, 0x00);
    return ssd1306_dev;
}

// ============================================================
// TASK 3 — Display vitals on the OLED screen
// This function receives fake vitals for now
// Later we will replace the fake values with real sensor data
// Fill in the TODO sections to display each vital on screen
// ============================================================
static void display_vitals(ssd1306_handle_t oled, int heart_rate, int spo2, float temperature)
{
    // Buffer to hold formatted strings
    char line[32];

    // Clear the screen before drawing
    ssd1306_clear_screen(oled, 0x00);

    // TODO: Format and display heart rate on line 1
    snprintf(line, sizeof(line), "HR:  %d BPM", heart_rate);
    ssd1306_draw_string(oled, 0, 0, (const uint8_t *)line, 16, 1);

    // TODO: Format and display SpO2 on line 2
    snprintf(line, sizeof(line), "SpO2: %d%%", spo2);
    ssd1306_draw_string(oled, 0, 16, (const uint8_t *)line, 16, 1);

    // TODO: Format and display temperature on line 3
    snprintf(line, sizeof(line), "Temp: %.1fF", temperature);
    ssd1306_draw_string(oled, 0, 32, (const uint8_t *)line, 16, 1);

    // TODO: Add a status line at the bottom
    snprintf(line, sizeof(line), "Status: OK");
    ssd1306_draw_string(oled, 0, 48, (const uint8_t *)line, 16, 1);

    // Refresh the display to show the new content
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
        ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "I2C initialized");

    // Initialize OLED
    ssd1306_handle_t oled = oled_init();
    if (oled == NULL) {
        ESP_LOGE(TAG, "OLED init failed");
        return;
    }
    ESP_LOGI(TAG, "OLED initialized");

    // TODO: Fill in fake vitals values to test the display
    int heart_rate = 70;
    int spo2 = 99;
    float temperature = 21.4;

    // Display vitals on screen
    // TODO: call display_vitals with your fake values
    display_vitals(oled, heart_rate, spo2, temperature);

    // TODO: Add a loop that updates the display every second
    // Use vTaskDelay(pdMS_TO_TICKS(1000)) for the delay
    while(1) {
        // TODO: call display_vitals here inside the loop
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}