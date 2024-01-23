#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "esp_attr.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"

#include "wifi.h"
#include "sensors.h"
#include "blinker.h"
#include "configuration.h"
#include "configuration_mode.c"
#include "pusher.h"

void app_main(void);
esp_err_t main_fetch_device_configuration(void);
bool main_is_configuration_button_pressed(void);
void main_configuration_mode_loop(void);
void main_normal_mode_loop(void);

RTC_DATA_ATTR static uint32_t boot_count = 0;
RTC_DATA_ATTR static uint8_t measurement_count = 0;
RTC_DATA_ATTR static uint32_t last_upload_timestamp = 0;
RTC_DATA_ATTR static struct sensor_data_t measurements[100];

void app_main(void)
{
    boot_count += 1;
    bool isColdBoot = boot_count == 1;

    // 1. Fetch device configuration once into RTC memory on cold boot
    if (isColdBoot) {
        main_fetch_device_configuration();
        printf(
            "Current config:\n"
            "data_sink=%s\n"
            "data_sink_push_format=%i\n"
            "measurement_rate=%i\n"
            "upload_rate=%i\n"
            "wifi_ssid=%s\n"
            "wifi_password=%s\n"
            "sleeping for %i us\n",
            configuration.data_sink,
            configuration.data_sink_push_format,
            configuration.measurement_rate,
            configuration.upload_rate,
            configuration.wifi_ssid,
            configuration.wifi_password,
            configuration.measurement_rate * 1000 * 1000
        );
        fflush(stdout);
    }

    // Intialize the blinker
    blinker_init();

    // 2. Jump into configuration mode if requested on cold boot
    if (isColdBoot && main_is_configuration_button_pressed()) {
        main_configuration_mode_loop();
    }

    // Update the system time once on cold boot
    if (isColdBoot) {
        ESP_ERROR_CHECK(connect_to_wifi());
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
        ESP_ERROR_CHECK(esp_netif_sntp_init(&config));
        ESP_ERROR_CHECK(esp_netif_sntp_sync_wait(pdMS_TO_TICKS(15000)));

        // Wait for the deregister to be processed by the AP
        disconnect_from_wifi();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    // Prepare some stuff for power saving reasons
    // TODO maybe needs to move before the sleep-call
    if (isColdBoot) {
        rtc_gpio_isolate(GPIO_NUM_4);
    }

    // Enter the normal measuring cycle
    main_normal_mode_loop();
}

esp_err_t main_fetch_device_configuration()
{
    esp_err_t err = cfg_load();
    if (err != ESP_OK) {
        printf("Error (%s) reading data from NVS!\n", esp_err_to_name(err));
        fflush(stdout);

        return err;
    }

    return ESP_OK;
}

bool main_is_configuration_button_pressed(void)
{
    //int CONFIGURATION_BUTTON_GPIO = 4;

    gpio_config_t cfg = {
        .pin_bit_mask = BIT64(4),
        .mode = GPIO_MODE_DEF_INPUT,
        .pull_up_en = false,
        .pull_down_en = true,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&cfg);
    
    return gpio_get_level(4) == 1;
}

void main_configuration_mode_loop(void)
{
    // We start a bluetooth service to read/modify the configuration
    cfgmode_start();

    // After the bluetooth service got started, we stay in that mode for 5 minutes
    vTaskDelay(300000 / portTICK_PERIOD_MS);

    // We restart after the 5 minutes
    esp_restart();
}

void main_normal_mode_loop(void)
{
    struct sensor_data_t* current_measurement = malloc(sizeof(struct sensor_data_t));
    memset(current_measurement, 0, sizeof(struct sensor_data_t));

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    current_measurement->timestamp = tv_now.tv_sec;

    sensors_init();
    sensors_read_temperature_and_humidity_outside(current_measurement);
    sensors_read_daylight(current_measurement);
    sensors_deinit();

    memcpy(&measurements[measurement_count], current_measurement, sizeof(struct sensor_data_t));
    free(current_measurement);
    measurement_count += 1;

    // Upon cold boot set the last_upload_timestamp to now
    if (last_upload_timestamp == 0) {
        last_upload_timestamp = tv_now.tv_sec;    
    }

    printf("measurement          : %i\n",   measurement_count);
    printf("tv_now.tv_sec        : %lli\n", tv_now.tv_sec);
    printf("last_upload_timestamp: %li\n",  last_upload_timestamp);
    fflush(stdout);

    // Check if enough time has past to trigger an upload
    if ((last_upload_timestamp + configuration.upload_rate) < tv_now.tv_sec) {
        ESP_ERROR_CHECK(connect_to_wifi());
        ESP_ERROR_CHECK(pusher_http_push(measurements, measurement_count));
        last_upload_timestamp = tv_now.tv_sec;

        // Since we uploaded the data we can discard it from rtc memory now
        measurement_count = 0;
        memset(&measurements, 0, sizeof(struct sensor_data_t) * 100);
    }

    disconnect_from_wifi();
    blinker_set_disabled();

    esp_sleep_enable_timer_wakeup(configuration.measurement_rate * 1000 * 1000);
    esp_deep_sleep_start();

    // 1. Initialize values in static rtc ram depending on configuration
    // TODO

    // 2. Collect sensor measurement values
    // TODO

    // 3. Persist the values in static rtc ram
    // TODO

    // 4. Send data off if threshold is reached
    // TODO

    // 5. Clear data in static rtc ram if previously sent
    // TODO

    // 6. Go into deep sleep for one interval
    // TODO
}