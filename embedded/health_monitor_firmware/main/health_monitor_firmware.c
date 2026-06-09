#include <stdio.h>
#include <string.h>
#include <math.h>
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
#define MPU6050_I2C_ADDRESS     0x68

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

// Heart rate algorithm
#define FINGER_THRESHOLD        50000
#define MOTION_LIGHT_THRESHOLD  0.05f
#define MOTION_ACTIVE_THRESHOLD 0.30f

static const char *TAG = "HEALTH_MONITOR";

// Global vitals shared between tasks
static float heartrate = 0.0f;
static float pctspo2 = 0.0f;

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

static void display_vitals(ssd1306_handle_t oled, float bpm, float spo2, float temp_f, const char *activity)
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
// HEART RATE TASK
// Uses 2nd order Butterworth bandpass filter (0.5-5Hz)
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

                // Butterworth bandpass filter on IR
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

                // Butterworth bandpass filter on RED
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

                // Peak detection on filtered signal
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
    esp_err_t err = i2c_master_init();
    if (err != ESP_OK) { ESP_LOGE(TAG, "I2C init failed"); return; }

    ssd1306_handle_t oled = oled_init();
    if (oled == NULL) { ESP_LOGE(TAG, "OLED init failed"); return; }

    err = max30102_init();
    if (err != ESP_OK) { ESP_LOGE(TAG, "MAX30102 init failed"); return; }

    err = mpu6050_init();
    if (err != ESP_OK) { ESP_LOGE(TAG, "MPU6050 init failed"); return; }

    ESP_LOGI(TAG, "All sensors initialized");

    xTaskCreate(max30102_task, "max30102_task", 4096, NULL, 5, NULL);

    float temp_f = 0.0f;
    const char *activity = "Resting";

    while (1) {
        err = mlx90614_read_temp_f(&temp_f);
        if (err != ESP_OK) temp_f = 0.0f;

        activity = mpu6050_get_activity();

        display_vitals(oled, heartrate, pctspo2, temp_f, activity);

        ESP_LOGI(TAG, "Temp: %.1fF  Activity: %s  HR: %.1f  SpO2: %.1f",
                 temp_f, activity, heartrate, pctspo2);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}