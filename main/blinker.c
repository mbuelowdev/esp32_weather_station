#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#define BLINK_GPIO 5

void init_blinker(void);
void set_blinker_disabled(void);
void set_blinker_bt_discoverable(void);
void set_blinker_bt_connected(void);

typedef enum {
    BLINKER_DISABLED = 0,
    BLINKER_BT_DISCOVERABLE = 1,
    BLINKER_BT_CONNECTED = 2
} BLINKER_MODE;

static BLINKER_MODE blinker_mode = BLINKER_DISABLED;

// Task to be created.
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

void init_blinker(void)
{
    // Initialize BLINK_GPIO for our LED
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BLINK_GPIO, 0);

    //static uint8_t ucParameterToPass;
    TaskHandle_t taskHandle = NULL;

    // Create the task, storing the handle.  Note that the passed parameter ucParameterToPass
    // must exist for the lifetime of the task, so in this case is declared static.  If it was just an
    // an automatic stack variable it might no longer exist, or at least have been corrupted, by the time
    // the new task attempts to access it.
    xTaskCreate(vTaskBlinker, "BLINKER", 1024, NULL, 1, &taskHandle); 
}

void set_blinker_disabled(void)
{
    blinker_mode = BLINKER_DISABLED;
}

void set_blinker_bt_discoverable(void)
{
    blinker_mode = BLINKER_BT_DISCOVERABLE;
}

void set_blinker_bt_connected(void)
{
    blinker_mode = BLINKER_BT_CONNECTED;
}