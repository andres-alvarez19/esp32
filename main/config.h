#pragma once

#define UBIDOTS_TOKEN "BBUS-TcuDrqL35peeGflABzk9m184wncXMr"
#define WIFI_SSID     "FAMILIA MORALES"
#define WIFI_PASS     "alvarezmorales"

#define DEVICE_LABEL  "esp_wroom_32"

// Ubidots connection settings
// Host recommended: industrial.api.ubidots.com
// Port: 1883 (no TLS) or 8883 (TLS recommended)
#define UBIDOTS_HOST  "industrial.api.ubidots.com"
#define UBIDOTS_PORT  1883
// Note: enabling TLS requires support in the Ubidots library; this flag is
// informational unless the library exposes an API to enable TLS.
#define UBIDOTS_USE_TLS 1

#define VAR_TEMP      "bme_temp_c"
#define VAR_HUM       "bme_hum_pct"
#define VAR_PRESS     "bme_press_hpa"
#define VAR_ALT       "bme_alt_m"
#define VAR_CO2_PPM   "sgp30_eco2_ppm"
#define VAR_TVOC_PPB  "sgp30_tvoc_ppb"
#define VAR_LUX       "bh1750_lux"
#define VAR_NOISE_DB  "spm1423_noise_db"
