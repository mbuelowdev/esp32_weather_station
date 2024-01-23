#ifndef __WEATHER_STATION__PUSHER_H__
#define __WEATHER_STATION__PUSHER_H__

#include "esp_err.h"
#include "sensors.h"
#include "pusher.c"

esp_err_t pusher_http_push(struct sensor_data_t* measurements, size_t measurements_length);

#endif