#include "nvs_storage.h"
#include "wifi_ntp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"  // For portTICK_PERIOD_MS
#include "freertos/task.h"      // For vTaskDelay
#include "ble_prov.h"
static const char *TAG = "MAIN";

void app_main(void) {
    // Initialize NVS
    if (nvs_init() != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed");
        return;
    }

    // Hands-on test: Save sample Wi-Fi credentials to NVS (replace with your real SSID/password; run once, then comment out)
    nvs_save_wifi_credentials("490..EXCITEL...2.4G", "7993783818");

    // Initialize Wi-Fi using NVS credentials
    if (wifi_ntp_init() != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi init failed");
        return;
    }

    // Sync NTP time
    if (ntp_sync_time() != ESP_OK) {
        ESP_LOGE(TAG, "NTP sync failed");
        return;
    }

    ESP_LOGI(TAG, "Wi-Fi and NTP sync complete");

    ESP_ERROR_CHECK(ble_prov_init());
    
    while (1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);  // Main loop (add RTC sync later)
    }
}