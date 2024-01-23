#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_sntp.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "http_parser.h"
#include "sdkconfig.h"

#include "sensors.h"
#include "configuration.h"
#include "formatter.h"

#define SERVER_URL_MAX_SZ 256

static const char *LOG_TAG = "PUSHER";

esp_err_t pusher_http_push(struct sensor_data_t* measurements, size_t measurements_length)
{
    char* url = &configuration.data_sink[0];
    char* http_request = (char*) malloc(10240);
    char* http_host = (char*) malloc(128);
    char* http_path = (char*) malloc(512);
    esp_tls_t *tls = NULL;

    memset(http_request, 0, 10240);
    memset(http_host, 0, 128);
    memset(http_path, 0, 512);

    esp_err_t esp_ret = ESP_OK;

    esp_tls_cfg_t cfg = {
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    printf("url: %s", url);
    fflush(stdout);

    // Parse the url to extract HOST, PATH, QUERY and FRAGMENT
    struct http_parser_url* url_parse_result = (struct http_parser_url*) malloc(sizeof(struct http_parser_url));
    http_parser_url_init(url_parse_result);
    if (http_parser_parse_url(url, strlen(url), 0, url_parse_result) != 0) {
        ESP_LOGE(LOG_TAG, "http_parser_parse_url failed");
        esp_ret = ESP_FAIL;
        goto cleanup;
    }

    // Write the hostname into [http_host]
    memset(http_host, 0, 128);
    strncpy(
        http_host,
        url + url_parse_result->field_data[UF_HOST].off,
        url_parse_result->field_data[UF_HOST].len
    );

    // Write the path, query and fragment part into [http_path]
    memset(http_path, 0, 512);
    strncpy(
        http_path,
        url + url_parse_result->field_data[UF_PATH].off,
        url_parse_result->field_data[UF_PATH].len
    );
    strncpy(
        http_path + url_parse_result->field_data[UF_PATH].len,
        url + url_parse_result->field_data[UF_QUERY].off,
        url_parse_result->field_data[UF_QUERY].len
    );
    strncpy(
        http_path + url_parse_result->field_data[UF_PATH].len + url_parse_result->field_data[UF_QUERY].len,
        url + url_parse_result->field_data[UF_FRAGMENT].off,
        url_parse_result->field_data[UF_FRAGMENT].len
    );

    // Format the measurements as configured
    char* measurements_formatted_buffer = malloc(5120);
    formatter_format_measurements_as_json(measurements_formatted_buffer, 5120, measurements, measurements_length);

    sprintf(
        http_request,
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: esp-idf/1.0 esp32\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        http_path, http_host,
        strlen(measurements_formatted_buffer),
        measurements_formatted_buffer
    );

    free(measurements_formatted_buffer);

    tls = esp_tls_init();
    if (!tls) {
        ESP_LOGE(LOG_TAG, "Failed to allocate esp_tls handle!");
        esp_ret = ESP_FAIL;
        goto cleanup;
    }

    if (esp_tls_conn_http_new_sync(url, &cfg, tls) == 1) {
        ESP_LOGI(LOG_TAG, "Connection established...");
    } else {
        ESP_LOGE(LOG_TAG, "Connection failed...");
        esp_ret = ESP_FAIL;
        goto cleanup;
    }

    size_t written_bytes = 0;
    size_t received_bytes = 0;
    int ret;
    char buf[64];

    // Send our http request
    do {
        ret = esp_tls_conn_write(tls, http_request + written_bytes, strlen(http_request) - written_bytes);
        if (ret >= 0) {
            ESP_LOGI(LOG_TAG, "%d bytes written", ret);
            written_bytes += ret;
        } else if (ret != ESP_TLS_ERR_SSL_WANT_READ  && ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
            ESP_LOGE(LOG_TAG, "esp_tls_conn_write  returned: [0x%02X](%s)", ret, esp_err_to_name(ret));
            esp_ret = ESP_FAIL;
            goto cleanup;
        }
    } while (written_bytes < strlen(http_request));

    // Receive the response. We ignore the contents.
    do {
        memset(buf, 0x00, sizeof(buf));
        ret = esp_tls_conn_read(tls, (char *)buf, sizeof(buf)-1);

        if (ret == ESP_TLS_ERR_SSL_WANT_WRITE  || ret == ESP_TLS_ERR_SSL_WANT_READ) {
            continue;
        } else if (ret < 0) {
            ESP_LOGE(LOG_TAG, "esp_tls_conn_read  returned [-0x%02X](%s)", -ret, esp_err_to_name(ret));
            break;
        } else if (ret == 0) {
            ESP_LOGI(LOG_TAG, "connection closed");
            break;
        }

        received_bytes += ret;
    } while (1);
    ESP_LOGI(LOG_TAG, "%d bytes received", received_bytes);

cleanup:
    free(http_request);
    free(http_host);
    free(http_path);

    if (tls) esp_tls_conn_destroy(tls);

    return esp_ret;
}