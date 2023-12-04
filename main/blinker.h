#ifndef __WEATHER_STATION__BLINKER_H__
#define __WEATHER_STATION__BLINKER_H__

#define BLINK_GPIO 5

void blinker_init(void);
void vTaskBlinker(void * pvParameters);
void blinker_set_disabled(void);
void blinker_set_bt_discoverable(void);
void blinker_set_bt_connected(void);

#endif