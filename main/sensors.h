#ifndef __WEATHER_STATION__SENSORS_H__
#define __WEATHER_STATION__SENSORS_H__

#include "esp_err.h"

struct sensor_data_t {
    /**
     * Time of measurement as unix timestamp
    */
    uint32_t timestamp;

    /**
     * Temperature in Celsius * 100
    */
    int32_t temperature;

    /**
     * Humidity in xyz * 100
    */
    int32_t humidity;
};

esp_err_t sensor_data_format_as_json(char* buffer, struct sensor_data_t* sensor_data, size_t sensor_data_length);
esp_err_t sensor_data_format_as_csv(char* buffer, struct sensor_data_t* sensor_data, size_t sensor_data_length);

#endif