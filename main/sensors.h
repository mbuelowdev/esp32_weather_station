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

#define SHT30_SENSOR_ADDR                   0x44           /*!< Temperature/Humidity */
#define BME280_SENSOR_ADDR                  0x76           /*!< Pressure */
#define VEML7700_SENSOR_ADDR                0x20           /*!< Daylight */
#define VEML6070_SENSOR_ADDR                0x68           /*!< UV */

struct sensor_data_t {
    /**
     * Time of measurement as unix timestamp
    */
    uint32_t timestamp;

    /**
     * Temperature in celsius
    */
    double temperature;

    /**
     * Relative humidity in %
    */
    double humidity;

    /**
     * Daylight in Lux
    */
    double daylight;
};

esp_err_t sensors_init(void);
esp_err_t sensors_deinit(void);
esp_err_t sensors_read_temperature_and_humidity_outside(struct sensor_data_t* measurement);
esp_err_t sensors_read_daylight(struct sensor_data_t * measurement);

#endif