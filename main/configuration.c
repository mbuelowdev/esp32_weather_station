#include <stdint.h>
#include <string.h>
#include <esp_attr.h>
#include <nvs_flash.h>
#include <nvs.h>

#define CONFIGURATION_NVS_NAMESPACE "cfg_ns"
#define CONFIGURATION_NVS_KEY "cfg"

typedef struct {
    /**
     * Where to push the measurement data. Supports: HTTP, HTTPS.
     * 
     * Default: "http://configuration/" 
    */
    char data_sink[256];

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

} configuration_t;

configuration_t default_configuration = {
    "http://configure/",
    0,
    60,
    600,
    "configure",
    "configure"
};

/**
 * The configuration in use
*/
RTC_DATA_ATTR configuration_t configuration;

/**
 * Loads the configuration from non-volatile storage (nvs) into the static
 * rtc ram.
 * 
 * If the configuration has not been set yet, the defaults will be written
 * and loaded.
*/
esp_err_t load_configuration()
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    size_t configure_t_size = sizeof(configuration_t);

    // Initialize the nvs library
    err = nvs_flash_init();
    if (err != ESP_OK && err != ESP_ERR_NVS_NO_FREE_PAGES && err != ESP_ERR_NVS_NEW_VERSION_FOUND) return err;

    // The nvs partition was truncated and needs to be erased.
    // We retry nvs_flash_init.
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        err = nvs_flash_erase();
        if (err != ESP_OK) return err;

        err = nvs_flash_init();
        if (err != ESP_OK) return err;
    }

    // Open nvs
    err = nvs_open(CONFIGURATION_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;

    // Check if the configuration already exists inside nvs
    size_t nvs_blob_size = 0;
    err = nvs_get_blob(nvs_handle, CONFIGURATION_NVS_KEY, NULL, &nvs_blob_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
    
    // If nothing resides inside nvs yet, then we write the default values to nvs
    if (nvs_blob_size == 0) {
        printf("Writing default configuration to nvs...");

        err = nvs_set_blob(nvs_handle, CONFIGURATION_NVS_KEY, &default_configuration, configure_t_size);
        if (err != ESP_OK) return err;
    }

    // Read persisted configuration
    configuration_t* new_configuration = malloc(configure_t_size);
    err = nvs_get_blob(nvs_handle, CONFIGURATION_NVS_KEY, new_configuration, &configure_t_size);
    if (err != ESP_OK) {
        free(new_configuration);
        return err;
    }

    configuration = *new_configuration;

    free(new_configuration);

    // Commit
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) return err;

    // Close
    nvs_close(nvs_handle);
    return ESP_OK;
}

/**
 * Writes the configuration to non-volatile storage (nvs)
*/
esp_err_t write_configuration()
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    size_t configure_t_size = sizeof(configuration_t);

    // Open nvs
    err = nvs_open(CONFIGURATION_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;

    // Write current configuration
    err = nvs_set_blob(nvs_handle, CONFIGURATION_NVS_KEY, &configuration, configure_t_size);
    if (err != ESP_OK) return err;

    // Commit
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) return err;

    // Close
    nvs_close(nvs_handle);
    return ESP_OK;
}
