#ifndef __WEATHER_STATION__FORMATTER_H__
#define __WEATHER_STATION__FORMATTER_H__

#include "esp_err.h"
#include "sensors.h"

esp_err_t formatter_format_measurements_as_json(char* buffer, size_t buffer_length, struct sensor_data_t* measurements, size_t measurements_length);
esp_err_t formatter_format_measurements_as_csv(char* buffer, size_t buffer_length, struct sensor_data_t* measurements, size_t measurements_length);

#endif