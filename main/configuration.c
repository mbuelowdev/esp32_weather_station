#include <stdint.h>
#include <string.h>
#include "esp_attr.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "configuration.h"

#define CONFIGURATION_NVS_NAMESPACE "cfg_ns"
#define CONFIGURATION_NVS_KEY "cfg"

struct configuration_t default_configuration = {
    "http://configure/",
    0,
    60,
    600,
    "configure",
    "configure"
};

RTC_DATA_ATTR struct configuration_t configuration;

/**
 * Loads the configuration from non-volatile storage (nvs) into the static
 * rtc ram.
 * 
 * If the configuration has not been set yet, the defaults will be written
 * and loaded.
*/
esp_err_t cfg_load()
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    size_t configure_t_size = sizeof(struct configuration_t);

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
    struct configuration_t* new_configuration = malloc(configure_t_size);
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
esp_err_t cfg_write()
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    size_t configure_t_size = sizeof(struct configuration_t);

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

