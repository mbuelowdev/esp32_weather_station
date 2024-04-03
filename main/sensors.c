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
i2c_master_dev_handle_t bme280_dev_handle = NULL;
i2c_master_dev_handle_t ltr390_dev_handle = NULL;
i2c_master_dev_handle_t max17048_dev_handle = NULL;

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

    // Register BME280 (Temperature, Pressure)
    i2c_device_config_t bme280_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x76,
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(i2c_bus_handle, &bme280_dev_cfg, &bme280_dev_handle);

    // Register LTR390 (Light, UV)
    i2c_device_config_t ltr390_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x53, // 53
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(i2c_bus_handle, &ltr390_dev_cfg, &ltr390_dev_handle);

    // Register MAX17048 (Battery)
    i2c_device_config_t max17048_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x36,
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(i2c_bus_handle, &max17048_dev_cfg, &max17048_dev_handle);

    // Let the sensors power up
    vTaskDelay(1 / portTICK_PERIOD_MS);
    
    return ESP_OK;
}

esp_err_t sensors_deinit(void)
{
    i2c_master_bus_rm_device(sht30_dev_handle);
    i2c_master_bus_rm_device(bme280_dev_handle);
    i2c_master_bus_rm_device(ltr390_dev_handle);
    i2c_master_bus_rm_device(max17048_dev_handle);
    i2c_del_master_bus(i2c_bus_handle);

    return ESP_OK;
}

esp_err_t sensors_read_temperature_and_humidity_outside(struct sensor_data_t * measurement)
{
    char* LOGTAG = "I2C-SHT30";

    // Check if the sensor is on the bus.
    // If not: use dummy values and log as error
    if (ESP_OK != i2c_master_probe(i2c_bus_handle, 0x44, 5000)) {
        ESP_LOGE(LOGTAG, "Could not find sensor on bus. Using dummy values instead.");
        measurement->temperature = 0.0;
        measurement->humidity = 0.0;
        return ESP_OK;
    } else {
        ESP_LOGI(LOGTAG, "Found sensor on bus!");
    }

    // Start measuring command
    uint8_t write_buf[3] = {0x24, 0x00};
    ESP_ERROR_CHECK(i2c_master_transmit(sht30_dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    vTaskDelay(50 / portTICK_PERIOD_MS);


    uint8_t read_buf[6] = {0};
    ESP_ERROR_CHECK(i2c_master_receive(sht30_dev_handle, read_buf, sizeof(read_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));


    ESP_LOG_BUFFER_HEX(LOGTAG, read_buf, sizeof(read_buf));

    uint16_t temperature_raw = (read_buf[0] << 8) + read_buf[1];
    uint16_t humidity_raw = (read_buf[3] << 8) + read_buf[4];

    double temp_f = temperature_raw / 65534.0f;
    double humd_f = humidity_raw / 65534.0f;

    double temperature = -45.0f + 175.0f * temp_f;
    double humidity = 100.0f * humd_f;

    ESP_LOGI(LOGTAG, "Temperature: %.1f°C", temperature);
    ESP_LOGI(LOGTAG, "Humidity: %.0f%%", humidity);

    measurement->temperature = (float) temperature;
    measurement->humidity = (float) humidity;

    return ESP_OK;
}

esp_err_t sensors_read_temperature_and_pressure_inside(struct sensor_data_t* measurement)
{
    char* LOGTAG = "I2C-BME280";
    
    uint8_t BME280_REGISTER_CHIPID = 0xD0;

    // Check if the sensor is on the bus.
    // If not: use dummy values and log as error
    if (ESP_OK != i2c_master_probe(i2c_bus_handle, 0x76, 5000)) {
        ESP_LOGE(LOGTAG, "Could not find sensor on bus. Using dummy values instead.");
        measurement->temperature_inside = 0.0;
        return ESP_OK;
    } else {
        ESP_LOGI(LOGTAG, "Found sensor on bus!");
    }

    // Read the chipid
    uint8_t write_buf1[1] = {BME280_REGISTER_CHIPID};
    uint8_t read_buf1[1] = {0};
    ESP_ERROR_CHECK(i2c_master_transmit_receive(bme280_dev_handle, write_buf1, sizeof(write_buf1), read_buf1, sizeof(read_buf1), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    ESP_LOG_BUFFER_HEX(LOGTAG, read_buf1, sizeof(read_buf1));

    return ESP_OK;
}

esp_err_t sensors_read_daylight_and_uv(struct sensor_data_t * measurement)
{
    char* LOGTAG = "I2C-LTR390";

    uint8_t wbuffer[8] = {0};
    uint8_t rbuffer[8] = {0};

    uint8_t LTR390_MAIN_CTRL = 0x00;
    uint8_t LTR390_ALS_UVS_MEAS_RATE = 0x04;
    uint8_t LTR390_ALS_UVS_GAIN = 0x05;
    uint8_t LTR390_PART_ID = 0x06;
    uint8_t LTR390_MAIN_STATUS = 0x07;
    uint8_t LTR390_ALS_DATA_0 = 0x0D;
    uint8_t LTR390_ALS_DATA_1 = 0x0E;
    uint8_t LTR390_ALS_DATA_2 = 0x0F;
    uint8_t LTR390_UVS_DATA_0 = 0x10;
    uint8_t LTR390_UVS_DATA_1 = 0x11;
    uint8_t LTR390_UVS_DATA_2 = 0x12;

    // Check if the sensor is on the bus.
    // If not: use dummy values and log as error
    if (ESP_OK != i2c_master_probe(i2c_bus_handle, 0x53, 5000)) {
        ESP_LOGE(LOGTAG, "Could not find sensor on bus. Using dummy values instead.");
        measurement->daylight = 0;
        measurement->uv = 0;
        return ESP_OK;
    } else {
        ESP_LOGI(LOGTAG, "Found sensor on bus!");
    }


    
    _clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_PART_ID;
    _i2c_error_check(i2c_master_transmit_receive(ltr390_dev_handle, wbuffer, 1, rbuffer, 1, 50000 / portTICK_PERIOD_MS));






    _clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_MAIN_STATUS;
    _i2c_error_check(i2c_master_transmit_receive(ltr390_dev_handle, wbuffer, 1, rbuffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));



    // Put the sensor into enabled ALS mode
    _clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_MAIN_CTRL;
    wbuffer[1] = 0b00000010;
    _i2c_error_check(i2c_master_transmit(ltr390_dev_handle, wbuffer, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));

    // Set the measurement resolution and rate
    // resolution: 18 bits
    // rate: every 100ms
    _clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_ALS_UVS_MEAS_RATE;
    wbuffer[1] = 0b00100010;
    _i2c_error_check(i2c_master_transmit(ltr390_dev_handle, wbuffer, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));

    // Set the gain to 3
    _clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_ALS_UVS_GAIN;
    wbuffer[1] = 0b00000001;
    _i2c_error_check(i2c_master_transmit(ltr390_dev_handle, wbuffer, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));

    // Wait 100ms for the first measurement and then continiously ask if the data is ready
    // with a delay of 10ms in between
    vTaskDelay(10 / portTICK_PERIOD_MS);
    int timeout_als = 10;
    while (timeout_als > 0) {
        _clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
        wbuffer[0] = LTR390_MAIN_STATUS;
        _i2c_error_check(i2c_master_transmit_receive(ltr390_dev_handle, wbuffer, 1, rbuffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));

        if (rbuffer[0] & 0b1000) {
            break;
        }

        timeout_als -= 1;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // Read the ALS
    uint32_t als_measurement = 0;

    // Read the lower 8 bits
    _clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_ALS_DATA_0;
    _i2c_error_check(i2c_master_transmit_receive(ltr390_dev_handle, wbuffer, 1, rbuffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    als_measurement = als_measurement + (rbuffer[0]);

    // Read the middle 8 bits
    _clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_ALS_DATA_1;
    _i2c_error_check(i2c_master_transmit_receive(ltr390_dev_handle, wbuffer, 1, rbuffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    als_measurement = als_measurement + (rbuffer[0] << 8);

    // Read the upper 4 bits
    _clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_ALS_DATA_2;
    _i2c_error_check(i2c_master_transmit_receive(ltr390_dev_handle, wbuffer, 1, rbuffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    als_measurement = als_measurement + (rbuffer[0] << 12);

    float lux = (0.6f * als_measurement) / (3 * 1);
    ESP_LOGI("I2C-LTR390", "lux: %.2f", lux);
    measurement->daylight = lux;

    // Put the sensor into enabled UVS mode
    _clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_MAIN_CTRL;
    wbuffer[1] = 0b00000000;
    _i2c_error_check(i2c_master_transmit(ltr390_dev_handle, wbuffer, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));

    // Wait 100ms for the first measurement and then continiously ask if the data is ready
    // with a delay of 10ms in between
    vTaskDelay(10 / portTICK_PERIOD_MS);
    int timeout_uvs = 10;
    while (timeout_uvs > 0) {
        _clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
        wbuffer[0] = LTR390_MAIN_STATUS;
        _i2c_error_check(i2c_master_transmit_receive(ltr390_dev_handle, wbuffer, 1, rbuffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));

        if (rbuffer[0] & 0b1000) {
            break;
        }

        timeout_uvs -= 1;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // Read the UVS
    uint32_t uvs_measurement = 0;

    // Read the lower 8 bits
    _clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_UVS_DATA_0;
    _i2c_error_check(i2c_master_transmit_receive(ltr390_dev_handle, wbuffer, 1, rbuffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    uvs_measurement = uvs_measurement + (rbuffer[0]);

    // Read the middle 8 bits
    _clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_UVS_DATA_1;
    _i2c_error_check(i2c_master_transmit_receive(ltr390_dev_handle, wbuffer, 1, rbuffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    uvs_measurement = uvs_measurement + (rbuffer[0] << 8);

    // Read the upper 4 bits
    _clear_buffers(rbuffer, wbuffer, sizeof(rbuffer), sizeof(wbuffer));
    wbuffer[0] = LTR390_UVS_DATA_2;
    _i2c_error_check(i2c_master_transmit_receive(ltr390_dev_handle, wbuffer, 1, rbuffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    uvs_measurement = uvs_measurement + (rbuffer[0] << 12);

    double uvi = uvs_measurement / ((double) ((3.0f/18.0f) * (1.0f/4.0f) * 2300.0f));
    ESP_LOGI("I2C-LTR390", "uvi: %.2f", uvi);
    measurement->uv = (float) uvi;

    return ESP_OK;
}

// TODO error handling
// TODO smoothen the code
// TODO comments
esp_err_t sensors_read_battery_status(struct sensor_data_t * measurement)
{
    char* LOGTAG = "I2C-MAX17048";

    uint8_t wbuffer[8] = {0};
    uint8_t rbuffer[8] = {0};

    uint8_t MAX17048_VCELL_REG = 0x02;
    uint8_t MAX17048_SOC_REG = 0x04;
    uint8_t MAX17048_CRATE_REG = 0x16;

    // Check if the sensor is on the bus.
    // If not: use dummy values and log as error
    if (ESP_OK != i2c_master_probe(i2c_bus_handle, 0x36, 5000)) {
        ESP_LOGE(LOGTAG, "Could not find sensor on bus. Using dummy values instead.");
        measurement->battery_voltage = 0.0;
        measurement->battery_charge = 0.0f;
        measurement->battery_charge_rate = 0.0;
        return ESP_OK;
    } else {
        ESP_LOGI(LOGTAG, "Found sensor on bus!");
    }

    // Read voltage
    uint8_t write_buf1[1] = {MAX17048_VCELL_REG};
    uint8_t read_buf1[2] = {0};
    ESP_ERROR_CHECK(i2c_master_transmit_receive(max17048_dev_handle, write_buf1, sizeof(write_buf1), read_buf1, sizeof(read_buf1), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    uint16_t voltage_raw = read_buf1[1] + (read_buf1[0] << 8);
    double voltage = voltage_raw * 78.125 / 1000000.0;
    ESP_LOG_BUFFER_HEX(LOGTAG, read_buf1, sizeof(read_buf1));
    ESP_LOGI(LOGTAG, "Voltage: %0.3fV", voltage);
    measurement->battery_voltage = (float) voltage;

    // Read percent
    uint8_t write_buf2[1] = {MAX17048_SOC_REG};
    uint8_t read_buf2[2] = {0};
    ESP_ERROR_CHECK(i2c_master_transmit_receive(max17048_dev_handle, write_buf2, sizeof(write_buf2), read_buf2, sizeof(read_buf2), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    uint16_t soc_raw = read_buf2[1] + (read_buf2[0] << 8);
    double charge = soc_raw / 256.0;
    ESP_LOG_BUFFER_HEX(LOGTAG, read_buf2, sizeof(read_buf2));
    ESP_LOGI(LOGTAG, "Charge: %0.1f%%", charge);
    measurement->battery_charge = (float) charge;

    // Read charge rate
    uint8_t write_buf3[1] = {MAX17048_CRATE_REG};
    uint8_t read_buf3[2] = {0};
    ESP_ERROR_CHECK(i2c_master_transmit_receive(max17048_dev_handle, write_buf3, sizeof(write_buf3), read_buf3, sizeof(read_buf3), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    uint16_t crate_raw = read_buf3[1] + (read_buf3[0] << 8);
    double charge_rate = ((int16_t) crate_raw) * 0.208;
    ESP_LOG_BUFFER_HEX(LOGTAG, read_buf3, sizeof(read_buf3));
    ESP_LOGI(LOGTAG, "Charge rate: %0.1f%%/h", charge_rate);
    measurement->battery_charge_rate = (float) charge_rate;

    return ESP_OK;
}

void _clear_buffers(uint8_t* buffer1, uint8_t* buffer2, size_t buffer1_size, size_t buffer2_size) {
    memset(&buffer1, 0, buffer1_size);
    memset(&buffer2, 0, buffer2_size);
}

void _i2c_error_check(uint8_t esp_result_code) {

    if (esp_result_code != ESP_OK) {
        ESP_LOGE("I2C", "Error: %i", esp_result_code);
        sensors_deinit();
        ESP_ERROR_CHECK(esp_result_code);
    }
}