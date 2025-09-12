#ifndef WIFI_NTP_H
#define WIFI_NTP_H

#include "esp_err.h"

// Initialize Wi-Fi using credentials from NVS and wait for connection
esp_err_t wifi_ntp_init(void);

// Sync system time with NTP server (pool.ntp.org)
esp_err_t ntp_sync_time(void);

// Debug DNS resolution
void test_dns_resolution(const char *hostname);

#endif