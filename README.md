# ESP32 Standalone WiFi Weather Station

A **solar, battery-powered, ESP32-DevKitC based, wifi** weather station. Written in C.

## Sensors

1. Temperature
2. Humidity
3. Pressure
5. Daylight
6. UV
7. Rain (l/mm2)
8. Wind force
9. Wind direction
10. Battery percentage

### Temperature, Humidity

Temperature and humidity will be measured by an SHT30 ([Product](https://www.adafruit.com/product/4099)|[Datasheet](https://www.sensirion.com/media/documents/213E6A3B/63A5A569/Datasheet_SHT3x_DIS.pdf)).

- The sensor resides outside of the weather station
- The sensor supports I2C (addresses: 0x44, 0x45)
- Has to be mounted under a rain shield and also upside down
- Has to be protected from direct sunlight
- Draws 600-1500uA while measuring, 2uA while idle
- Operates between -40° to +125° Celsius

### Pressure

Pressure will be measured by an BME280 ([Product](https://www.adafruit.com/product/2652)|[Datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf)|[Library](https://github.com/boschsensortec/BME280_driver)).

- The sensor resides inside the weather station
- The sensor supports I2C (addresses: 0x76, 0x77)
- Needs to be in an area with airflow (Consider mounting it in a sepereate area from main electronics)
- Draws 4uA while active and <1uA while in sleep mode
- Operates between -40° to +85° Celsius

### Daylight

Daylight will be measured by an LTR390 ([Product](https://www.adafruit.com/product/4831)|[Datasheet](https://optoelectronics.liteon.com/upload/download/DS86-2015-0004/LTR-390UV-01_Final_%20DS_V1.4.PDF)).

- The sensor resides inside the weather station
- The sensor supports I2C (addresses: 0x53)
- Has to be located next to a small window
- Draws 110uA while active and 1uA while in standby
- Operates between -25° to +85° Celsius

### UV

UV will be measured by an LTR390 ([Product](https://www.adafruit.com/product/4831)|[Datasheet](https://optoelectronics.liteon.com/upload/download/DS86-2015-0004/LTR-390UV-01_Final_%20DS_V1.4.PDF)).

- The sensor resides inside the weather station
- The sensor supports I2C (addresses: 0x53)
- Has to be located next to a small window
- Draws 110uA while active and 1uA while in standby
- Operates between -40° to +85° Celsius

TODO:

- Currently only supports UV-A and no UV-B
- What to use for the window that does not block uv? Glass, plastic?

### Rain

TBD

### Wind force

TBD

### Wind direction

TBD

### Battery percentage

Battery percentage will be measured by an MAX17048 ([Product](https://www.adafruit.com/product/5580)|[Datasheet](https://cdn-learn.adafruit.com/assets/assets/000/114/607/original/MAX17048-MAX17049.pdf)).

- The sensor resides inside the weather station
- The sensor supports I2C (addresses: 0x36)
- Draws 23uA while active, 4uA while hibernating
- Operates between -40° to +85° Celsius

## Special Use-Cases

1. Die Wetterstation kann über einen Schalter am Gehäuse in den "Nur Laden"-Modus gebracht werden. (Entkoppeln des ESP32 vom Strom)
2. Die Wetterstation kann über einen Knopf in den Konfigurations-Modus versetzt werden. (Konfigurieren via Bluetooth und App)
3. Benutzer kann über App folgende Einstellungen tätigen:
    1. WLAN SSID und Passwort hinterlegen
    2. Abtastrate festlegen (alle 10s, 20s, 30s, usw.)
    3. Uploadrate festlegen (alle 5min, 15min, 30min, 60min, usw.)
    4. Anzeigen der maximalen Lebensdauer bei voller Batterie und aktueller Konfiguration (Unterscheidung Sommer/Winter)


## Program

### Modes

1. Normal-Mode
2. Configuration-Mode

#### Normal-Mode

In `Normal-Mode` your weather station will periodically take measurements, persist them and send them off.

#### Configuration-Mode

The weather station has a `Configuration-Mode` in which different configuration parameters/options can be set. To launch into configuration mode simply hold down the `Configuration-Button` while performing a cold boot (reconnecting power source). Release the `Configuration-Button` after 4-5 seconds or after the `Configuration-LED` starts blinking. You now have a 60 second window to open up the weather station app to pair with your weather station. Upon successful pairing, you can edit the persistent configuration of your weather station. Click on apply to change the configuration and reboot your weather station.

To apply a new configuration you have to repeat the whole pairing process again. Changing configurations while the weather station is working is not possible due to power saving measures. Bluetooth is expensive power wise.

The following values can be configured:

- **Data Sink**
*String, URL*
Where to push the measurement data. Supports: HTTP, HTTPS.
- **Data Sink Push Format**
*Int*
The format in which the data should be pushed to the data sink. Supports JSON (0), CSV (1).
- **Measurement Rate**
*Int, Seconds*
The interval in which measurements should be taken. Default: 60.
- **Upload Rate**
*Int, Seconds*
The interval in which data should be pushed to the data sink. Default: 600.
- **WiFi SSID**
*String*
The name of the wifi network to connect to.
- **WiFi Password**
*String*
The password for the wifi network.

## Power Consumption

- Measuring (M) takes 5 seconds and draws 500mA
- Sending (S) takes 15 seconds and draws 500mA
- Idling draws 1mA
- The battery provides 6600mAh
- The battery takes 7,75h to charge without a consumer and with maximum charging current (900mA)

### Example 1

Measuring every 15 seconds and sending every 5 minutes. Ignoring the fact, that a measurement would take 20 seconds instead of 15.

In one hour we have:
- 240 measurements (3600s/15s), 1200s of measuring (240x5s)
- 12 sendings (60min/5min), 180s of sending (12x15s)
- 2220s of idling (3600s - 1200s - 180s)

That results in (rounded up):
- 167mAh for measuring (33% of 500mAh)
- 25mAh for sending (5% of 500mAh)
- 1mAh for idling (62% of 1mAh)

For this example the weather station would draw **193mAh** per hour. With a fully charged battery and without solar power, the weather station can operate for 34 hours.

### Example 2

Measuring every 30 seconds and sending every 10 minutes. Ignoring the fact, that a measurement would take 35 seconds instead of 30.

In one hour we have:
- 120 measurements (3600s/30s), 600s of measuring (120x5s)
- 6 sendings (60min/10min), 90s of sending (6x15s)
- 2910s of idling (3600s - 600s - 90s)

That results in (rounded up):
- 85mAh for measuring (17% of 500mAh)
- 15mAh for sending (3% of 500mAh)
- 1mAh for idling (81% of 1mAh)

For this example the weather station would draw **101mAh** per hour. With a fully charged battery and without solar power, the weather station can operate for 65 hours.

### Example 3

Measuring every 60 seconds and sending every 10 minutes. Ignoring the fact, that a measurement would take 65 seconds instead of 60.

In one hour we have:
- 60 measurements (3600s/60s), 300s of measuring (60x5s)
- 6 sendings (60min/10min), 90s of sending (6x15s)
- 3210s of idling (3600s - 300s - 90s)

That results in (rounded up):
- 45mAh for measuring (9% of 500mAh)
- 15mAh for sending (3% of 500mAh)
- 1mAh for idling (89% of 1mAh)

For this example the weather station would draw **61mAh** per hour. With a fully charged battery and without solar power, the weather station can operate for 108 hours.

## Schutz vor den Elementen

- Wir brauchen ein besonderes Coating/Sealant womit wir die ganze Elektronik mit Außnahme der Sensoren bedecken. [Siehe hier](https://resources.pcb.cadence.com/blog/2020-common-methods-for-waterproofing-electronics-materials)
- Das Gehäuse sollte einem Stevenson Screen ähneln. [Siehe hier](https://duckduckgo.com/?q=small+stevenson+screen&t=ffab&atb=v326-1&iar=images&iax=images&ia=images)

## Umsetzung bestimmter Funktionalitäten

### Persistieren der Wetterdaten

Die Wetterdaten werden im RTC static RAM (RTC_DATA_ATTR, [Siehe hier](https://github.com/G6EJD/ESP32_RTC_RAM/blob/master/ESP32_BME280_RTCRAM_Datalogger.ino)) persistiert. Der RAM überlebt den Deep-Sleep vom ESP32. Alternativ könnten wir auch auf den internen Flash-Speicher schreiben. Das führt allerdings zu einem hohen Verschleiß ([Siehe hier](https://stackoverflow.com/questions/63780850/esp32-best-way-to-store-data-frequently)). 

Um eine Variable im RTC static RAM zu halten brauchen wir sie nur mit einem Attribut zu versehen.

```c
typedef struct {
  float temperature;
  int   humidity;
  float pressure;  
} readings;

RTC_DATA_ATTR readings Readings[100];
```

Aus der Abtastrate und der Uploadrate, berechnen wir uns, wie oft wir die Sensorwerte auslesen können. Sobald dieses Maximum erreicht wurde, laden wir die Daten zum Server hoch.