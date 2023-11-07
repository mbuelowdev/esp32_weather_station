#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "wifi.c"
#include "sensors.c"
#include "configuration.c"
#include "blinker.c"

void app_main(void);
void configuration_mode_loop(void);
void normal_mode_loop(void);
void sleep_for_next_interval(void);

void app_main(void)
{
    // 1. Check if we have to boot into configuration mode
    // TODO

    // 2. Fetch device configuration from nvs
    esp_err_t err = load_configuration();
    if (err != ESP_OK) {
        printf("Error (%s) reading data from NVS!\n", esp_err_to_name(err));
        fflush(stdout);
        fflush(stdout);
        sleep_for_next_interval();

        return;
    }

    printf("Current config:\ndata_sink=%s\ndata_sink_push_format=%i\nmeasurement_rate=%i\nupload_rate=%i\nwifi_ssid=%s\nwifi_password=%s\n", configuration.data_sink, configuration.data_sink_push_format, configuration.measurement_rate, configuration.upload_rate, configuration.wifi_ssid, configuration.wifi_password);
    fflush(stdout);

    init_blinker();

    for (int i = 60; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        if (i == 45) {
            set_blinker_bt_discoverable();
        }

        if (i == 30) {
            set_blinker_bt_discoverable();
        }

        if (i == 15) {
            set_blinker_bt_connected();
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();

    // 3. Switch into normal mode
    // TODO
}

void configuration_mode_loop(void)
{
    // 1. Enable bluetooth pairing and wait for client to connect
    // TODO

    // 2. Send current configuration to client
    // TODO

    // 3. Wait and receive new configuration from client
    // TODO

    // 4. Persist new configuration
    // TODO

    // 5. Reboot
    // TODO
}

void normal_mode_loop(void)
{
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

void sleep_for_next_interval()
{

}