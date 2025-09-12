#ifndef NVS_STORAGE_H
#define NVS_STORAGE_H

#include "esp_err.h"
#include "nvs.h"

/* LED profile structure */
typedef struct {
    uint8_t red, green, blue; // RGB values (0-255)
    uint8_t brightness;       // 0-255
    uint8_t mode;             // 0:clock, 1: countdown
} led_profile_t;

// Initialize NVS
esp_err_t nvs_init(void);

// Save Wi-Fi credentials
esp_err_t nvs_save_wifi_credentials(const char *ssid, const char *password);

// Load Wi-Fi credentials
esp_err_t nvs_load_wifi_credentials(char *ssid, size_t *ssid_len, char *password, size_t *pass_len);
// Save profiles 
esp_err_t nvs_save_profile(uint8_t profile_id, const led_profile_t *profile);

//Load profiles
esp_err_t nvs_load_profile(uint8_t profile_id, led_profile_t *profile);
#endif