#pragma once

// Credenciales WiFi
#define WIFI_SSID     "FAMILIA MORALES"
#define WIFI_PASS     "alvarezmorales"

// Identificador de dispositivo (para logs y clientId)
#define DEVICE_LABEL  "esp_wroom_32"

// Configuracion de ThingsBoard
#define TB_HOST         "iot.ceisufro.cl"
#define TB_PORT         1883
#define TB_ACCESS_TOKEN "TrPoEAdBd7PNI46BLPC5"

// Habilitar/deshabilitar buzzer si no está conectado
#define BUZZER_ENABLED 1

// Claves de telemetria en ThingsBoard
#define VAR_TEMP_C      "bme_temp_c"
#define VAR_HUM_PCT     "bme_hum_pct"
#define VAR_ECO2_PPM    "sgp30_eco2_ppm"
#define VAR_TVOC_PPB    "sgp30_tvoc_ppb"
#define VAR_LUX         "bh1750_lux"
#define VAR_NOISE_DB    "spm1423_noise_db"
