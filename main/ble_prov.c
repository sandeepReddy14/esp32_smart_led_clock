#include "ble_prov.h"
#include "esp_log.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"
#include "esp_wifi_types.h" // For wifi_sta_config_t

#define TAG "BLE_PROV"
#define SERVICE_NAME "CLOCK_PROV"

// Provisioning Event Handler
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                ESP_LOGI(TAG, "Provisioning started");
                break;
            case WIFI_PROV_CRED_RECV: {
                // In ESP-IDF v4.4/v5.x, event_data is a wifi_sta_config_t pointer
                wifi_sta_config_t *wifi_cfg = (wifi_sta_config_t *)event_data;
                if (wifi_cfg) {
                    ESP_LOGI(TAG, "Received Wi-Fi credentials: SSID=%s", wifi_cfg->ssid);
                } else {
                    ESP_LOGE(TAG, "Invalid credentials data");
                }
                break;
            }
            case WIFI_PROV_CRED_FAIL:
                ESP_LOGE(TAG, "Provisioning failed");
                break;
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Provisioning successful");
                break;
            case WIFI_PROV_END:
                // Restart provisioning to keep it always active
                ESP_LOGI(TAG, "Provisioning ended, restarting to stay active");
                wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, NULL, SERVICE_NAME, NULL);
                break;
            default:
                break;
        }
    }
}

// Initialize BLE Wi-Fi provisioning
esp_err_t ble_prov_init(void) {
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
    };
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    // Set app info with capabilities
    const char *capabilities[] = {"wifi", NULL};
    ESP_ERROR_CHECK(wifi_prov_mgr_set_app_info(SERVICE_NAME, "1.0", capabilities, NULL));

    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, NULL, SERVICE_NAME, NULL));
    ESP_LOGI(TAG, "BLE Provisioning started");
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    return ESP_OK;
}