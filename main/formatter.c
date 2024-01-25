#include "formatter.h"
#include <string.h>
#include "esp_err.h"
#include "sensors.h"

esp_err_t formatter_format_measurements_as_json(char* buffer, size_t buffer_length, struct sensor_data_t* measurements, size_t measurements_length)
{
    size_t offset = 0;

    if (measurements_length == 0) {
        return ESP_OK;
    }

    memset(buffer, 0, buffer_length);

    // Write the first part of the JSON
    sprintf(
        &buffer[offset],
        "{\"measurements\":["
    ); 

    // Write everything but the last measurement (with trailing comma)
    for (size_t i = 0; i < (measurements_length - 1); i++) {
        offset = strlen(buffer);
        sprintf(
            &buffer[offset],
            "{"
                "\"time\":%li,"
                "\"temp\":%.2f,"
                "\"humd\":%.0f,"
                "\"dayl\":%.0f,"
                "\"uv\":%d,"
                "\"batt\":{"
                    "\"volt\":%.3f,"
                    "\"chrg\":%.2f,"
                    "\"chrt\":%.2f"
                "}"
            "},", // With comma
            measurements[i].timestamp,
            measurements[i].temperature,
            measurements[i].humidity,
            measurements[i].daylight,
            measurements[i].uv,
            measurements[i].battery_voltage,
            measurements[i].battery_charge,
            measurements[i].battery_charge_rate
        );   
    }

    // Write the last measurement (without trailing comma)
    offset = strlen(buffer);
    sprintf(
        &buffer[offset],
        "{"
            "\"time\":%li,"
            "\"temp\":%.2f," // 47-57 letters
            "\"humd\":%.0f,"
            "\"dayl\":%.0f,"
            "\"uv\":%d,"
            "\"batt\":{"
                "\"volt\":%.3f,"
                "\"chrg\":%.2f,"
                "\"chrt\":%.2f"
            "}"
        "}", // Without comma
        measurements[measurements_length - 1].timestamp,
        measurements[measurements_length - 1].temperature,
        measurements[measurements_length - 1].humidity,
        measurements[measurements_length - 1].daylight,
        measurements[measurements_length - 1].uv,
        measurements[measurements_length - 1].battery_voltage,
        measurements[measurements_length - 1].battery_charge,
        measurements[measurements_length - 1].battery_charge_rate
    );

    // Write the last part of the JSON
    offset = strlen(buffer);
    sprintf(
        &buffer[offset],
        "]}"
    ); 

    return ESP_OK;
}

esp_err_t formatter_format_measurements_as_csv(char* buffer, size_t buffer_length, struct sensor_data_t* measurements, size_t measurements_length)
{
    //sprintf(buffer, "%f,%f", (sensor_data->temperature / 100.0), (sensor_data->humidity / 100.0));

    return ESP_OK;
}