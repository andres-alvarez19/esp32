#pragma once

#define UBIDOTS_TOKEN "Token"
#define WIFI_SSID     "WIFI SSID"
#define WIFI_PASS     "contrasena"

#define DEVICE_LABEL  "esp_wroom_32"

// Ubidots connection settings
// Host recommended: industrial.api.ubidots.com
// Port: 1883 (no TLS) or 8883 (TLS recommended)
#define UBIDOTS_HOST  "industrial.api.ubidots.com"
#define UBIDOTS_PORT  1883
// Note: enabling TLS requires support in the Ubidots library; set to 0 to
// disable TLS (use plain MQTT on port 1883).
#define UBIDOTS_USE_TLS 0

// Increase MQTT buffers to allow larger payloads
#define MQTT_MAX_PACKET_SIZE 1024
#define MQTT_KEEPALIVE 60

#define VAR_TEMP      "bme_temp_c"
#define VAR_HUM       "bme_hum_pct"
#define VAR_PRESS     "bme_press_hpa"
#define VAR_ALT       "bme_alt_m"
#define VAR_CO2_PPM   "sgp30_eco2_ppm"
#define VAR_TVOC_PPB  "sgp30_tvoc_ppb"
#define VAR_LUX       "bh1750_lux"
#define VAR_NOISE_DB  "spm1423_noise_db"
