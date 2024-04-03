#ifndef __WEATHER_STATION__SENSORS_H__
#define __WEATHER_STATION__SENSORS_H__

#include "esp_err.h"
#include "driver/gpio.h"

#define I2C_MASTER_SCL_IO                   GPIO_NUM_22    /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO                   GPIO_NUM_21    /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM                      0              /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ                  100000         /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE           0              /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE           0              /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS               1000

struct sensor_data_t {
    /**
     * Time of measurement as unix timestamp
    */
    uint32_t timestamp;

    /**
     * Temperature in celsius
    */
    float temperature;

    /**
     * Temperature in celsius
    */
    float temperature_inside;

    /**
     * Relative humidity in %
    */
    float humidity;

    /**
     * Pressure in 
    */
    float pressure;

    /**
     * Daylight in Lux
    */
    uint32_t daylight;

    /**
     * UVA intensity in μW/cm²
    */
    uint16_t uv;

    /**
     * Battery voltage in V
    */
    float battery_voltage;

    /**
     * Battery charge in %
    */
    float battery_charge;

    /**
     * Battery charge/discharge rate in %/h
    */
    float battery_charge_rate;
};

esp_err_t sensors_init(void);
esp_err_t sensors_deinit(void);
esp_err_t sensors_read_temperature_and_humidity_outside(struct sensor_data_t* measurement);
esp_err_t sensors_read_temperature_and_pressure_inside(struct sensor_data_t* measurement);
esp_err_t sensors_read_daylight_and_uv(struct sensor_data_t * measurement);
esp_err_t sensors_read_battery_status(struct sensor_data_t * measurement);

void clear_buffers(uint8_t* buffer1, uint8_t* buffer2, size_t buffer1_size, size_t buffer2_size);

#endif