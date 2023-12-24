#ifndef __WEATHER_STATION__CONFIGURATION_H__
#define __WEATHER_STATION__CONFIGURATION_H__

#include <stdint.h>
#include <string.h>
#include <esp_attr.h>

esp_err_t cfg_load(void);
esp_err_t cfg_write(void);

struct configuration_t {
    /**
     * Where to push the measurement data. Supports: HTTP, HTTPS.
     * 
     * Default: "http://configuration/" 
    */
    char data_sink[300];

    /**
     * The format in which the data should be pushed to the data sink. Supports
     * JSON (0), CSV (1).
     * 
     * Default: 0
    */
    uint8_t data_sink_push_format;

    /**
     * The interval in which measurements should be taken.
     * 
     * Default: 60.
    */
    uint16_t measurement_rate;

    /**
     * The interval in which data should be pushed to the data sink.
     * 
     * Default: 600.
    */
    uint16_t upload_rate;

    /**
     * The name of the wifi network to connect to.
     * 
     * Default: "configuration"
    */
    char wifi_ssid[64];

    /**
     * The password for the wifi network.
     * 
     * Default: "configuration"
    */
    char wifi_password[64];
};

/**
 * The configuration in use
*/
extern RTC_DATA_ATTR struct configuration_t configuration;

#endif