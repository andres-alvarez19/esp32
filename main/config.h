#pragma once

// Credenciales WiFi
#define WIFI_SSID     "SSID"
#define WIFI_PASS     "PASSWORD"
#define WIFI_CONNECT_TIMEOUT_MS 15000

// Punto de acceso para configurar nuevas credenciales cuando falla la conexion
#define WIFI_AP_SSID  "ESP32_Config"
#define WIFI_AP_PASS  "12345678"

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

// Intervalo de envío de telemetría (ms)
#define TB_PUBLISH_INTERVAL_MS 5000
