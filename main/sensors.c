#include <string.h>
#include <stdio.h>
#include "sensors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_err.h"

i2c_master_bus_handle_t i2c_bus_handle;
i2c_master_dev_handle_t sht30_dev_handle = NULL;
i2c_master_dev_handle_t veml7700_dev_handle = NULL;

esp_err_t sensors_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = i2c_master_port,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle));

    // Register SHT30 (Temperature, Humidity)
    i2c_device_config_t sht30_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x44,
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(i2c_bus_handle, &sht30_dev_cfg, &sht30_dev_handle);

    // Register VEML7700 (Daylight)
    i2c_device_config_t veml7700_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x10,
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(i2c_bus_handle, &veml7700_dev_cfg, &veml7700_dev_handle);

    return ESP_OK;
}

esp_err_t sensors_deinit(void)
{
    i2c_master_bus_rm_device(sht30_dev_handle);
    i2c_del_master_bus(i2c_bus_handle);

    return ESP_OK;
}

esp_err_t sensors_read_temperature_and_humidity_outside(struct sensor_data_t * measurement)
{
    // 1. Power the sensor
    // powerup();
    vTaskDelay(1 / portTICK_PERIOD_MS);

    // Start measuring command
    uint8_t write_buf[3] = {0x24, 0x00};
    esp_err_t err = i2c_master_transmit(sht30_dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    vTaskDelay(50 / portTICK_PERIOD_MS);

    if (err != ESP_OK) {
        ESP_LOGE("I2C-SHT30", "error1: %i", err);
        return err;
    }

    uint8_t read_buf[6] = {0};
    err = i2c_master_receive(sht30_dev_handle, read_buf, sizeof(read_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    if (err != ESP_OK) {
        ESP_LOGE("I2C-SHT30", "error2: %i", err);
        return err;
    }

    ESP_LOG_BUFFER_HEX("I2C-SHT30", read_buf, sizeof(read_buf));

    uint16_t temperature_raw = (read_buf[0] << 8) + read_buf[1];
    uint16_t humidity_raw = (read_buf[3] << 8) + read_buf[4];

    double temp_f = temperature_raw / 65534.0f;
    double humd_f = humidity_raw / 65534.0f;

    double temperature = -45.0f + 175.0f * temp_f;
    double humidity = 100.0f * humd_f;

    ESP_LOGI("I2C-SHT30", "Temperature: %.1f°C", temperature);
    ESP_LOGI("I2C-SHT30", "Humidity: %.0f%%", humidity);

    measurement->temperature = temperature;
    measurement->humidity = humidity;

    return ESP_OK;
}

esp_err_t sensors_read_daylight(struct sensor_data_t * measurement)
{
    esp_err_t err = ESP_OK;

    // 1. Power the sensor
    // powerup();
    vTaskDelay(1 / portTICK_PERIOD_MS);

    uint8_t ALS_CONF_0_CODE = 0x00;
    uint16_t ALS_CONF_0 = 0;
    ALS_CONF_0 = ALS_CONF_0 | (0b000 << 15); // Reserved
    ALS_CONF_0 = ALS_CONF_0 | (0b10 << 12);  // ALS_GAIN
    ALS_CONF_0 = ALS_CONF_0 | (0b0 << 10);   // Reserved
    ALS_CONF_0 = ALS_CONF_0 | (0b1000 << 9); // ALS_IT
    ALS_CONF_0 = ALS_CONF_0 | (0b00 << 5);   // ALS_PERS
    ALS_CONF_0 = ALS_CONF_0 | (0b00 << 3);   // Reserved
    ALS_CONF_0 = ALS_CONF_0 | (0b0 << 1);    // ALS_INT_EN
    ALS_CONF_0 = ALS_CONF_0 | (0b0 << 0);    // ALS_SD

    uint8_t POWER_SAVING_CODE = 0x03;
    uint16_t POWER_SAVING = 0;
    POWER_SAVING = POWER_SAVING | (0b11 << 2);   // PSM
    POWER_SAVING = POWER_SAVING | (0b0 << 0);    // PSM_EN

    // Powerup the sensor and configure it
    uint8_t write_buf1[3] = {ALS_CONF_0_CODE, (ALS_CONF_0 & 0xFF), ((ALS_CONF_0 >> 8) & 0xFF)};
    err = i2c_master_transmit(veml7700_dev_handle, write_buf1, sizeof(write_buf1), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    if (err != ESP_OK) {
        ESP_LOGE("I2C-VEML7700", "error-c-1: %i", err);
        return err;
    }

    // Disable power saving mode
    uint8_t write_buf2[3] = {POWER_SAVING_CODE, (POWER_SAVING & 0xFF), ((POWER_SAVING >> 8) & 0xFF)};
    err = i2c_master_transmit(veml7700_dev_handle, write_buf2, sizeof(write_buf2), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    if (err != ESP_OK) {
        ESP_LOGE("I2C-VEML7700", "error-c-2: %i", err);
        return err;
    }

    // Wait for the first measurement
    vTaskDelay(1000 / portTICK_PERIOD_MS);


    // Read the measurement
    uint8_t READ_ALS_CODE = 0x04;
    uint8_t write_buf3[1] = {READ_ALS_CODE};
    uint8_t read_buf[2] = {0};
    err = i2c_master_transmit_receive(veml7700_dev_handle, write_buf3, sizeof(write_buf3), read_buf, sizeof(read_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    if (err != ESP_OK) {
        ESP_LOGE("I2C-VEML7700", "error-r-1: %i", err);
        return err;
    }

    // Save the measurement
    ESP_LOG_BUFFER_HEX("I2C-VEML7700", read_buf, sizeof(read_buf));
    uint16_t raw_lux = read_buf[0] + (read_buf[1] << 8);
    double lux = (raw_lux * 8.0f) / 5.0f;
    ESP_LOGI("I2C-VEML7700", "Lux: %.2f", lux);
    measurement->daylight = lux;

    // Shutdown the sensor to save power
    uint16_t ALS_CONF_0_SHUTDOWN = ALS_CONF_0 | (0b1 << 0);
    uint8_t write_buf4[3] = {ALS_CONF_0_CODE, (ALS_CONF_0_SHUTDOWN & 0xFF), ((ALS_CONF_0_SHUTDOWN >> 8) & 0xFF)};
    err = i2c_master_transmit(veml7700_dev_handle, write_buf4, sizeof(write_buf4), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (err != ESP_OK) {
        ESP_LOGE("I2C-VEML7700", "error-sd-1: %i", err);
        return err;
    }

    return ESP_OK;
}