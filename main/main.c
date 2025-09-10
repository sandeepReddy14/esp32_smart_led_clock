#include "nvs_storage.h"
#include "esp_log.h"

static const char *TAG = "MAIN";

void app_main(void) {
    // Initialize NVS
    esp_err_t err = nvs_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS initialization failed: %s", esp_err_to_name(err));
        return;
    }

    // Test Wi-Fi credentials
    const char *test_ssid = "SmartClockWiFi";
    const char *test_password = "securepass456";
    err = nvs_save_wifi_credentials(test_ssid, test_password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save Wi-Fi credentials: %s", esp_err_to_name(err));
        return;
    }

    char loaded_ssid[32] = {0};
    char loaded_password[64] = {0};
    err = nvs_load_wifi_credentials(loaded_ssid, sizeof(loaded_ssid), loaded_password, sizeof(loaded_password));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Loaded Wi-Fi: SSID=%s, Password=%s", loaded_ssid, loaded_password);
    } else {
        ESP_LOGE(TAG, "Failed to load Wi-Fi credentials: %s", esp_err_to_name(err));
    }

    // Test LED profile
    led_profile_t test_profile = {
        .red = 0,      // No red
        .green = 255,  // Full green
        .blue = 128,   // Half blue
        .brightness = 150, // Medium-high brightness
        .mode = 0      // Clock mode
    };
    err = nvs_save_profile(1, &test_profile);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save profile 1: %s", esp_err_to_name(err));
        return;
    }

    led_profile_t loaded_profile = {0};
    err = nvs_load_profile(1, &loaded_profile);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Loaded profile 1: R=%d, G=%d, B=%d, Brightness=%d, Mode=%d",
                 loaded_profile.red, loaded_profile.green, loaded_profile.blue,
                 loaded_profile.brightness, loaded_profile.mode);
    } else {
        ESP_LOGE(TAG, "Failed to load profile 1: %s", esp_err_to_name(err));
    }
}