#include <string.h>
#include "sensors.h"
#include "esp_err.h"

esp_err_t sensor_data_format_as_json(char* buffer, struct sensor_data_t* sensor_data, size_t sensor_data_length)
{
    sprintf(
        buffer,
        "{\"measurements\":["
    ); 

    for (size_t i = 0; i < sensor_data_length; i++) {
        sprintf(
            &buffer[strlen(buffer)],
            "{\"temperature\":%f,\"humidity\":%f}",
            (sensor_data->temperature / 100.0),
            (sensor_data->humidity / 100.0)
        );   
        // if not at end of loop, insert comma     
    }

    return ESP_OK;
}

esp_err_t sensor_data_format_as_csv(char* buffer, struct sensor_data_t* sensor_data, size_t sensor_data_length)
{
    for (size_t i = 0; i < sensor_data_length; i++) {
        
    }

    sprintf(buffer, "%f,%f", (sensor_data->temperature / 100.0), (sensor_data->humidity / 100.0));

    return ESP_OK;
}