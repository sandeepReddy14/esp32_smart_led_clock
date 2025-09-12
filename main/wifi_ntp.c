#include "wifi_ntp.h"
#include "nvs_storage.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"  // Modern SNTP APIs
#include "lwip/netdb.h"  // For getaddrinfo, freeaddrinfo
#include "freertos/event_groups.h"
#include <string.h>
#include <time.h>

static const char *TAG = "WIFI_NTP";
static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define MAX_RETRY 5

static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch(event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                if (s_retry_num < MAX_RETRY) {
                    esp_wifi_connect();
                    s_retry_num++;
                    ESP_LOGI(TAG, "Retry Wi-Fi connection (%d/%d)", s_retry_num, MAX_RETRY);
                } else {
                    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                }
                ESP_LOGI(TAG, "Wi-Fi disconnected");
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void test_dns_resolution(const char *hostname) {
    struct addrinfo hints = {0};
    struct addrinfo *res;
    hints.ai_family = AF_INET;  // IPv4
    hints.ai_socktype = SOCK_DGRAM;  // UDP for NTP
    int err = getaddrinfo(hostname, NULL, &hints, &res);
    if (err != 0 || res == NULL) {
        ESP_LOGE(TAG, "DNS resolution failed for %s: %d", hostname, err);
    } else {
        char addr_str[16];
        inet_ntoa_r(((struct sockaddr_in *)res->ai_addr)->sin_addr, addr_str, sizeof(addr_str));
        ESP_LOGI(TAG, "DNS resolved %s to %s", hostname, addr_str);
        freeaddrinfo(res);
    }
}

static bool sntp_synced = false;
static void time_sync_notification_cb(struct timeval *tv) {
    sntp_synced = true;
    ESP_LOGI(TAG, "NTP sync notification: time set to %lld.%06ld", (long long)tv->tv_sec, (long)tv->tv_usec);
}

esp_err_t wifi_ntp_init(void) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    // Load credentials from NVS
    char ssid[32] = {0};
    char password[64] = {0};
    size_t ssid_len = sizeof(ssid);
    size_t pass_len = sizeof(password);
    esp_err_t err = nvs_load_wifi_credentials(ssid, &ssid_len, password, &pass_len);
    ESP_LOGI(TAG, "Loaded credentials: SSID=%s (len %d), Password len %d", ssid, ssid_len, pass_len);
    if (err != ESP_OK || ssid_len == 0) {
        ESP_LOGE(TAG, "No Wi-Fi credentials in NVS: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }

    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid));
    memcpy(wifi_config.sta.password, password, strlen(password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi initialized with SSID=%s", ssid);

    // Wait for connection
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Wi-Fi connected successfully");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Wi-Fi connection failed after %d retries", MAX_RETRY);
        return ESP_FAIL;
    }
}

esp_err_t ntp_sync_time(void) {
    // Reset SNTP if running
    esp_sntp_stop();

    // Set mode, servers, callback, and init
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.nist.gov");
    test_dns_resolution("pool.ntp.org");
    test_dns_resolution("time.nist.gov");
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_synced = false;
    esp_sntp_init();

    // Wait for callback or timeout (60s)
    int retry = 0;
    const int retry_count = 30;
    while (!sntp_synced && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for NTP sync (%d/%d)...", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if (sntp_synced) {
        time_t now = 0;
        struct tm timeinfo = {0};
        time(&now);
        localtime_r(&now, &timeinfo);
        ESP_LOGI(TAG, "NTP synced: %02d:%02d:%02d %02d/%02d/%04d",
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "NTP sync failed after %d retries (status: %d)", retry_count, esp_sntp_get_sync_status());
        esp_sntp_stop();
        return ESP_FAIL;
    }
}