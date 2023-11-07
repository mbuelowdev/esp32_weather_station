#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#define BLINK_GPIO 5

void init_blinker(void);
void set_blinker_disabled(void);
void set_blinker_bt_discoverable(void);
void set_blinker_bt_connected(void);
void vTaskBlinker(void * pvParameters);

typedef enum {
    BLINKER_DISABLED = 0,
    BLINKER_BT_DISCOVERABLE = 1,
    BLINKER_BT_CONNECTED = 2
} BLINKER_MODE;

static BLINKER_MODE blinker_mode = BLINKER_DISABLED;

/**
 * Initializes the blinker pin an starts the async blinker
 * controlling FreeRTOS-Task.
*/
void init_blinker(void)
{
    // Initialize BLINK_GPIO for our LED
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BLINK_GPIO, 0);

    // Start async Task to manage LED blinking
    TaskHandle_t taskHandle = NULL;
    xTaskCreate(vTaskBlinker, "BLINKER", 1024, NULL, 1, &taskHandle); 
}

/**
 * Blinker controlling FreeRTOS-Task to run separately from main task.
*/
void vTaskBlinker(void * pvParameters)
{
    bool led_is_on = false;

    while (1) {
        switch (blinker_mode) {
            case BLINKER_DISABLED:
                gpio_set_level(BLINK_GPIO, 0);
                break;
            case BLINKER_BT_DISCOVERABLE:
                gpio_set_level(BLINK_GPIO, led_is_on ? 0 : 1);
                led_is_on = led_is_on ? false : true;
                break;
            case BLINKER_BT_CONNECTED:
                gpio_set_level(BLINK_GPIO, 1);
                led_is_on = true;
                break;    
            default:
                break;
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

/**
 * Put blinker in BLINKER_BT_DISCOVERABLE state.
 * 
 * LED will be turned off.
*/
void set_blinker_disabled(void)
{
    blinker_mode = BLINKER_DISABLED;
}

/**
 * Put blinker in BLINKER_BT_DISCOVERABLE state.
 * 
 * LED will blink in 500ms interval.
*/
void set_blinker_bt_discoverable(void)
{
    blinker_mode = BLINKER_BT_DISCOVERABLE;
}

/**
 * Put blinker in BLINKER_BT_CONNECTED state.
 * 
 * LED will be turned on permanently.
*/
void set_blinker_bt_connected(void)
{
    blinker_mode = BLINKER_BT_CONNECTED;
}