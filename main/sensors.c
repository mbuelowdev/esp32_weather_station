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
//i2c_master_dev_handle_t veml7700_dev_handle = NULL;
//i2c_master_dev_handle_t veml6070_0x38_dev_handle = NULL;
//i2c_master_dev_handle_t veml6070_0x39_dev_handle = NULL;
//i2c_master_dev_handle_t veml6070_0x0C_dev_handle = NULL;
i2c_master_dev_handle_t ltr390_dev_handle = NULL;
i2c_master_dev_handle_t max17048_dev_handle = NULL;
//i2c_master_dev_handle_t bme280_dev_handle = NULL;

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
 /*
    // Register VEML7700 (Daylight)
    i2c_device_config_t veml7700_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x10,
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(i2c_bus_handle, &veml7700_dev_cfg, &veml7700_dev_handle);

    // Register VEML6070 (UVA intensity) (Address used for writing to cmd and reading lsb data)
    i2c_device_config_t veml6070_0x38_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x38,
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(i2c_bus_handle, &veml6070_0x38_dev_cfg, &veml6070_0x38_dev_handle);

    // Register VEML6070 (UVA intensity) (Address used for reading msb data)
    i2c_device_config_t veml6070_0x39_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x39,
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(i2c_bus_handle, &veml6070_0x39_dev_cfg, &veml6070_0x39_dev_handle);

    // Register VEML6070 (UVA intensity) (Address used for clearing ara register)
    i2c_device_config_t veml6070_0x0C_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x0C,
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(i2c_bus_handle, &veml6070_0x0C_dev_cfg, &veml6070_0x0C_dev_handle);
*/
    // Register LTR390 (Light, UV)
    i2c_device_config_t ltr390_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x53, // 53
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(i2c_bus_handle, &ltr390_dev_cfg, &ltr390_dev_handle);

    // Register MAX17048 (battery)
    i2c_device_config_t max17048_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x36,
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(i2c_bus_handle, &max17048_dev_cfg, &max17048_dev_handle);
/*
    // Register BME280 (Temperature and Pressure)
    i2c_device_config_t bme280_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x76,
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(i2c_bus_handle, &bme280_dev_cfg, &bme280_dev_handle);
*/
    
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
    // Power the sensor
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

    measurement->temperature = (float) temperature;
    measurement->humidity = (float) humidity;

    return ESP_OK;
}
/*
esp_err_t sensors_read_temperature_and_pressure_inside(struct sensor_data_t* measurement)
{
    esp_err_t err;

    uint8_t BME280_REGISTER_CHIPID = 0xD0;

    // Read the chipid
    uint8_t write_buf1[1] = {BME280_REGISTER_CHIPID};
    uint8_t read_buf1[1] = {0};
    err = i2c_master_transmit_receive(bme280_dev_handle, write_buf1, sizeof(write_buf1), read_buf1, sizeof(read_buf1), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    if (err != ESP_OK) {
        ESP_LOGE("I2C-BME280", "error-1: %i", err);
        return err;
    }

    ESP_LOG_BUFFER_HEX("I2C-BME280", read_buf1, sizeof(read_buf1));

    return ESP_OK;
}

esp_err_t sensors_read_daylight(struct sensor_data_t * measurement)
{
    esp_err_t err = ESP_OK;

    // Power the sensor
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
    measurement->daylight = (uint32_t) lux;

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

esp_err_t sensors_read_uv(struct sensor_data_t * measurement)
{
    esp_err_t err = ESP_OK;

    // Power the sensor
    vTaskDelay(150 / portTICK_PERIOD_MS);

    // Clear ara register
    uint8_t read_buf1[1] = {0};
    err = i2c_master_receive(veml6070_0x0C_dev_handle, read_buf1, sizeof(read_buf1), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    if (err != ESP_OK) {
        ESP_LOGE("I2C-VEML6070", "error-r-1: %i", err);
        return err;
    }

    uint8_t VEML_CONFIG = 0;
    VEML_CONFIG = VEML_CONFIG | (0b00 << 7);  // Reserved
    VEML_CONFIG = VEML_CONFIG | (0b0 << 5);   // ACK
    VEML_CONFIG = VEML_CONFIG | (0b0 << 4);   // ACK_THD
    VEML_CONFIG = VEML_CONFIG | (0b10 << 3);  // IT
    VEML_CONFIG = VEML_CONFIG | (0b1 << 1);   // Reserved
    VEML_CONFIG = VEML_CONFIG | (0b0 << 0);   // SD
    
    // Write the device configuration
    uint8_t write_buf1[1] = {VEML_CONFIG};
    err = i2c_master_transmit(veml6070_0x38_dev_handle, write_buf1, sizeof(write_buf1), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    if (err != ESP_OK) {
        ESP_LOGE("I2C-VEML6070", "error-r-2: %i", err);
        return err;
    }

    // Wait at least one IT (integration time)
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Read the measurement (lsb)
    uint8_t read_buf2[1] = {0};
    err = i2c_master_receive(veml6070_0x38_dev_handle, read_buf2, sizeof(read_buf2), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    if (err != ESP_OK) {
        ESP_LOGE("I2C-VEML6070", "error-r-3: %i", err);
        return err;
    }

    // Read the measurement (msb)
    uint8_t read_buf3[1] = {0};
    err = i2c_master_receive(veml6070_0x39_dev_handle, read_buf3, sizeof(read_buf3), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    if (err != ESP_OK) {
        ESP_LOGE("I2C-VEML6070", "error-r-4: %i", err);
        return err;
    }

    // Save the measurement
    ESP_LOG_BUFFER_HEX("I2C-VEML6070", read_buf2, sizeof(read_buf2));
    ESP_LOG_BUFFER_HEX("I2C-VEML6070", read_buf3, sizeof(read_buf3));
    uint16_t raw_uv = read_buf2[0] + (read_buf3[0] << 8);
    ESP_LOGI("I2C-VEML6070", "UV: %d", raw_uv);
    measurement->uv = raw_uv;

    // Shutdown the sensor to save power
    uint8_t VEML_CONFIG_SHUTDOWN = VEML_CONFIG | (0b1 << 0);
    uint8_t write_buf2[1] = {VEML_CONFIG_SHUTDOWN};
    err = i2c_master_transmit(veml6070_0x38_dev_handle, write_buf2, sizeof(write_buf2), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    if (err != ESP_OK) {
        ESP_LOGE("I2C-VEML6070", "error-r-5: %i", err);
        return err;
    }

    return ESP_OK;
}
*/
esp_err_t sensors_read_daylight_and_uv(struct sensor_data_t * measurement)
{
    esp_err_t err;
    //uint8_t wbuffer[8] = {0};
    //uint8_t rbuffer[8] = {0};
    uint8_t wbuffer[1] = {0};
    uint8_t rbuffer[1] = {0};

    uint8_t LTR390_MAIN_CTRL = 0x00;
    uint8_t LTR390_ALS_UVS_MEAS_RATE = 0x04;
    uint8_t LTR390_ALS_UVS_GAIN = 0x05;
    uint8_t LTR390_PART_ID = 0x06;
    uint8_t LTR390_MAIN_STATUS = 0x07;
    /*uint8_t LTR390_ALS_DATA_0 = 0x0D;
    uint8_t LTR390_ALS_DATA_1 = 0x0E;
    uint8_t LTR390_ALS_DATA_2 = 0x0F;
    uint8_t LTR390_UVS_DATA_0 = 0x10;
    uint8_t LTR390_UVS_DATA_1 = 0x11;
    uint8_t LTR390_UVS_DATA_2 = 0x12; */

    err = i2c_master_probe(i2c_bus_handle, 0x53, 5000);
    if (err != ESP_OK) {
        ESP_LOGE("I2C-LTR390", "error-probe: %i", err);
        return ESP_OK;
    } else {
        ESP_LOGI("I2C-LTR390", "PROBE OK");
    }


    //sensors_deinit();
    //sensors_init();

    //return ESP_OK;


    vTaskDelay(1000 / portTICK_PERIOD_MS);
    err = i2c_master_probe(i2c_bus_handle, 0x53, 5000);
    if (err != ESP_OK) {
        ESP_LOGE("I2C-LTR390", "error-probe: %i", err);
        return ESP_OK;
    } else {
        ESP_LOGI("I2C-LTR390", "PROBE OK");
    }



    ESP_LOGI("I2C-LTR390", "Wait...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);


    ESP_LOGI("I2C-LTR390", "Probe");
    err = i2c_master_probe(i2c_bus_handle, 0x53, 5000);
    if (err != ESP_OK) {
        ESP_LOGE("I2C-LTR390", "error-probe: %i", err);
        return ESP_OK;
    } else {
        ESP_LOGI("I2C-LTR390", "PROBE 5 OK");
        ESP_LOGI("I2C-LTR390", "PROBE 5 OK");
    }

    
    uint8_t test_bufferA[1] = {LTR390_PART_ID};
    uint8_t test_bufferB[1] = {0};
    ESP_LOG_BUFFER_HEX("I2C-LTR390", test_bufferA, sizeof(test_bufferA));
    err = i2c_master_transmit_receive(ltr390_dev_handle, test_bufferA, sizeof(test_bufferA), test_bufferB, sizeof(test_bufferB), 50000 / portTICK_PERIOD_MS);
    ESP_LOGE("I2C-LTR390", "error-partid-get: %i", err);
    ESP_LOG_BUFFER_HEX("I2C-LTR390", test_bufferB, sizeof(test_bufferB));






    return err;
    
    // sensor requires 5ms after powerup to boot into idle mode


/*
    ESP_LOGE("I2C-LTR390", "CLEAR BUFFERS 1");
    clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    ESP_LOGE("I2C-LTR390", "CLEAR BUFFERS 2");
    wbuffer[0] = LTR390_PART_ID;
    err = i2c_master_transmit_receive(ltr390_dev_handle, wbuffer, 1, rbuffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    ESP_LOGE("I2C-LTR390", "errorA: %i", err);
    ESP_LOG_BUFFER_HEX("I2C-LTR390", rbuffer, sizeof(rbuffer));




    clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_MAIN_STATUS;
    err = i2c_master_transmit_receive(ltr390_dev_handle, wbuffer, 1, rbuffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    ESP_LOGE("I2C-LTR390", "error0: %i", err);
    ESP_LOG_BUFFER_HEX("I2C-LTR390", rbuffer, sizeof(rbuffer));



    // Put the sensor into enabled ALS mode
    clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_MAIN_CTRL;
    wbuffer[1] = 0b00000010;
    err = i2c_master_transmit(ltr390_dev_handle, wbuffer, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    ESP_LOGE("I2C-LTR390", "error1: %i", err);

    // Set the measurement resolution and rate
    // resolution: 18 bits
    // rate: every 100ms
    clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_ALS_UVS_MEAS_RATE;
    wbuffer[1] = 0b00100010;
    err = i2c_master_transmit(ltr390_dev_handle, wbuffer, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    ESP_LOGE("I2C-LTR390", "error2: %i", err);

    // Set the gain to 3
    clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_ALS_UVS_GAIN;
    wbuffer[1] = 0b00000001;
    err = i2c_master_transmit(ltr390_dev_handle, wbuffer, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    ESP_LOGE("I2C-LTR390", "error3: %i", err);

    // Wait 100ms for the first measurement and then continiously ask if the data is ready
    // with a delay of 10ms in between
    vTaskDelay(100 / portTICK_PERIOD_MS);
    int timeout = 10;
    while (timeout > 0) {
        clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
        wbuffer[0] = LTR390_MAIN_STATUS;
        err = i2c_master_transmit_receive(ltr390_dev_handle, wbuffer, 1, rbuffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
        ESP_LOGE("I2C-LTR390", "error4: %i", err);
        ESP_LOG_BUFFER_HEX("I2C-LTR390", rbuffer, sizeof(rbuffer));

        if (rbuffer[0] & 0b1000) {
            break;
        }

        timeout -= 1;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // Read the ALS
    uint32_t als_measurement = 0;

    // Read the lower 8 bits
    clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_ALS_DATA_0;
    err = i2c_master_transmit_receive(ltr390_dev_handle, wbuffer, 1, rbuffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    ESP_LOGE("I2C-LTR390", "error5: %i", err);
    als_measurement = als_measurement + (rbuffer[0]);

    // Read the middle 8 bits
    clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_ALS_DATA_0;
    err = i2c_master_transmit_receive(ltr390_dev_handle, wbuffer, 1, rbuffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    ESP_LOGE("I2C-LTR390", "error6: %i", err);
    als_measurement = als_measurement + (rbuffer[0] << 8);

    // Read the upper 4 bits
    clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_ALS_DATA_0;
    err = i2c_master_transmit_receive(ltr390_dev_handle, wbuffer, 1, rbuffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    ESP_LOGE("I2C-LTR390", "error7: %i", err);
    als_measurement = als_measurement + (rbuffer[0] << 12);

    ESP_LOGI("I2C-LTR390", "als_measurement: %li", als_measurement);

    measurement->daylight = 0;
    measurement->uv = 0;

    return ESP_OK;*/
}

// TODO error handling
// TODO smoothen the code
// TODO comments
esp_err_t sensors_read_battery_status(struct sensor_data_t * measurement)
{
    esp_err_t err;

    uint8_t MAX17048_VCELL_REG = 0x02;
    uint8_t MAX17048_SOC_REG = 0x04;
    uint8_t MAX17048_CRATE_REG = 0x16;

    // Power the sensor
    vTaskDelay(1 / portTICK_PERIOD_MS);

    // Read voltage
    uint8_t write_buf1[1] = {MAX17048_VCELL_REG};
    uint8_t read_buf1[2] = {0};
    err = i2c_master_transmit_receive(max17048_dev_handle, write_buf1, sizeof(write_buf1), read_buf1, sizeof(read_buf1), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    uint16_t voltage_raw = read_buf1[1] + (read_buf1[0] << 8);
    double voltage = voltage_raw * 78.125 / 1000000.0;
    ESP_LOG_BUFFER_HEX("I2C-MAX17048", read_buf1, sizeof(read_buf1));
    ESP_LOGI("I2C-MAX17048", "Voltage: %0.3fV", voltage);
    measurement->battery_voltage = (float) voltage;

    // Read percent
    uint8_t write_buf2[1] = {MAX17048_SOC_REG};
    uint8_t read_buf2[2] = {0};
    err = i2c_master_transmit_receive(max17048_dev_handle, write_buf2, sizeof(write_buf2), read_buf2, sizeof(read_buf2), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    uint16_t soc_raw = read_buf2[1] + (read_buf2[0] << 8);
    double charge = soc_raw / 256.0;
    ESP_LOG_BUFFER_HEX("I2C-MAX17048", read_buf2, sizeof(read_buf2));
    ESP_LOGI("I2C-MAX17048", "Charge: %0.1f%%", charge);
    measurement->battery_charge = (float) charge;

    // Read charge rate
    uint8_t write_buf3[1] = {MAX17048_CRATE_REG};
    uint8_t read_buf3[2] = {0};
    err = i2c_master_transmit_receive(max17048_dev_handle, write_buf3, sizeof(write_buf3), read_buf3, sizeof(read_buf3), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    uint16_t crate_raw = read_buf3[1] + (read_buf3[0] << 8);
    double charge_rate = ((int16_t) crate_raw) * 0.208;
    ESP_LOG_BUFFER_HEX("I2C-MAX17048", read_buf3, sizeof(read_buf3));
    ESP_LOGI("I2C-MAX17048", "Charge rate: %0.1f%%/h", charge_rate);
    measurement->battery_charge_rate = (float) charge_rate;

    return ESP_OK;
}

void clear_buffers(uint8_t* buffer1, uint8_t* buffer2, size_t buffer1_size, size_t buffer2_size) {
    memset(&buffer1, 0, buffer1_size);
    memset(&buffer2, 0, buffer2_size);
}