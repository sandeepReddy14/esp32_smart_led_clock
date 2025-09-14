#ifndef BLE_PROV_H
#define BLE_PROV_H

#include "esp_err.h"

// Initialize BLE Wi-Fi provisioning (always-on)
esp_err_t ble_prov_init(void);

#endif // BLE_PROV_H