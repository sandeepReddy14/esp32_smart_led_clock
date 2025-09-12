#include "nvs_storage.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>
static const char *TAG = "NVS";

esp_err_t nvs_init(void) {
    esp_err_t ret = nvs_flash_init();
    if ((ret == ESP_ERR_NVS_NO_FREE_PAGES) || (ret == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        ESP_LOGW(TAG, "Erasing NVS partition...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "NVS initialized");
    } else {
        ESP_LOGE(TAG, "NVS initialization failed: %s",esp_err_to_name(ret));
    }
    return ret;
}


esp_err_t nvs_save_wifi_credentials(const char *ssid, const char *password) {

    nvs_handle_t nvs_handle;
    /* open storage */
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s",esp_err_to_name(err));
        return err;
    }

    /* set wifi ssid */
    err = nvs_set_str(nvs_handle, "wifi_ssid", ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS set wifi ssid failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    /* set wifi password */
    err = nvs_set_str(nvs_handle, "wifi_pass", password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS set wifi pass failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

cleanup:

    /* commit to nvs */
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS commit failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Wi-Fi credentials saved: SSID=%s", ssid);
    }

    /* close opened storage */
    nvs_close(nvs_handle);
    return err;
}

esp_err_t nvs_load_wifi_credentials(char *ssid, size_t *ssid_len, char *password, size_t *pass_len) {
    nvs_handle_t nvs_handle;
    /* open storage */
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    // Get wifi ssid
    size_t temp_ssid_len = *ssid_len;  // Use temp to avoid overwrite
    err = nvs_get_str(nvs_handle, "wifi_ssid", ssid, &temp_ssid_len);
    if (err == ESP_OK) {
        *ssid_len = temp_ssid_len;
        ESP_LOGI(TAG, "SSID loaded: %s (length %d)", ssid, *ssid_len);
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ssid[0] = '\0';
        *ssid_len = 0;
        ESP_LOGW(TAG, "SSID not found in NVS");
    } else {
        ESP_LOGE(TAG, "Failed to load SSID: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Get wifi pass
    size_t temp_pass_len = *pass_len;
    err = nvs_get_str(nvs_handle, "wifi_pass", password, &temp_pass_len);
    if (err == ESP_OK) {
        *pass_len = temp_pass_len;
        ESP_LOGI(TAG, "Password loaded (length %d)", *pass_len);  // Hide content for security
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        password[0] = '\0';
        *pass_len = 0;
        ESP_LOGW(TAG, "Password not found in NVS");
    } else {
        ESP_LOGE(TAG, "Failed to load password: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    /* close the storage */
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "Wi-Fi credentials loaded: SSID length=%d", *ssid_len);
    return ESP_OK;
}

esp_err_t nvs_save_profile(uint8_t profile_id, const led_profile_t *profile) {
    nvs_handle_t nvs_handle;
    char key[16];
    memset(key,0,16);
    snprintf(key,sizeof(key), "led_profile_%d", profile_id);

    /* open storage */
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    /* Store data as blob */
    err = nvs_set_blob(nvs_handle, key, profile, sizeof(led_profile_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save profile %d: %s", profile_id, esp_err_to_name(err));
        goto cleanup;
    }

    /* commit to nvs */
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit profile %d: %s", profile_id, esp_err_to_name(err));
        goto cleanup;
    } else {
        ESP_LOGI(TAG, "Profile %d saved", profile_id);
    }

cleanup:
    nvs_close(nvs_handle);
    return err;
}

esp_err_t nvs_load_profile(uint8_t profile_id, led_profile_t *profile) {
    nvs_handle_t nvs_handle;
    char key[16];
    memset(key,0,16);
    snprintf(key,sizeof(key), "led_profile_%d", profile_id);

    /* Open storage */
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open nvs: %s", esp_err_to_name(err));
        return err;
    }

    /* get data as blob */
    size_t size = sizeof(led_profile_t);
    err = nvs_get_blob(nvs_handle, key, profile, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        memset(profile, 0 , sizeof(led_profile_t));
        ESP_LOGI(TAG, "Profile %d not found, returning default", profile_id);
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load profile %d: %s", profile_id, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    } else {
        ESP_LOGI(TAG, "Profile %d loaded", profile_id);
    }

    /* Close storage */
    nvs_close(nvs_handle);
    return ESP_OK;

}