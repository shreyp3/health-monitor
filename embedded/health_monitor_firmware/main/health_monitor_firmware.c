#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"

// ============================================================
// CONFIGURATION — you don't need to change these
// ============================================================
#define I2C_MASTER_SCL_IO       21          // SCL pin on ESP32
#define I2C_MASTER_SDA_IO       22          // SDA pin on ESP32
#define I2C_MASTER_NUM          I2C_NUM_0   // I2C port number
#define I2C_MASTER_FREQ_HZ      100000      // 100kHz standard speed
#define I2C_MASTER_TIMEOUT_MS   1000        // timeout in ms

static const char *TAG = "I2C_SCANNER";

// ============================================================
// TASK 1 — Initialize the I2C bus
// Fill in the i2c_config_t struct fields below
// Use the #defines above for pin numbers and frequency
// ============================================================
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,       // GPIO 22
        .scl_io_num = I2C_MASTER_SCL_IO,       // GPIO 21
        .sda_pullup_en = GPIO_PULLUP_ENABLE,   // enable internal pullup on SDA
        .scl_pullup_en = GPIO_PULLUP_ENABLE,   // enable internal pullup on SCL
        .master.clk_speed = I2C_MASTER_FREQ_HZ, // 100 kHz
    };

    // Apply the configuration to the I2C port
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) return err;

    // Install the I2C driver
    // Parameters: port, mode, rx buffer (0 for master), tx buffer (0 for master), flags
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// ============================================================
// TASK 2 — Probe a single I2C address
// This sends a start condition to the given address and checks
// if anything responds with an ACK
// You don't need to change this function
// ============================================================
static bool i2c_probe_address(uint8_t address)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    return (err == ESP_OK);
}

// ============================================================
// TASK 3 — Scan all I2C addresses and print results
// Fill in the loop below to scan addresses 1 through 127
// For each address, call i2c_probe_address()
// If it returns true, log the address using ESP_LOGI
// ============================================================
static void i2c_scanner(void)
{
    ESP_LOGI(TAG, "Starting I2C scan...");
    int devices_found = 0;

    for (uint8_t address = 1; address < 128; address++)
    {
        if (i2c_probe_address(address))
        {
            ESP_LOGI(TAG, "Found device at address: 0x%02X", address);
            devices_found++;
        }
    }

    if (devices_found == 0) {
        ESP_LOGI(TAG, "No I2C devices found");
    } else {
        ESP_LOGI(TAG, "Scan complete — %d device(s) found", devices_found);
    }
}

// ============================================================
// MAIN — do not modify this
// ============================================================
void app_main(void)
{
    // Initialize I2C
    esp_err_t err = i2c_master_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "I2C initialized successfully");

    // Run the scanner
    i2c_scanner();
}