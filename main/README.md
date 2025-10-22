# Proyecto ESP32 – Monitor Ambiental

## Módulos y variables publicadas en Ubidots

| Módulo / Sensor | Descripción | Variable (tag) MQTT |
|-----------------|-------------|---------------------|
| BME280          | Temperatura, humedad relativa, presión y altitud calculada | `bme_temp_c`, `bme_hum_pct`, `bme_press_hpa`, `bme_alt_m` |
| SGP30           | CO₂ equivalente y compuestos orgánicos volátiles totales | `sgp30_eco2_ppm`, `sgp30_tvoc_ppb` |
| BH1750          | Medición de iluminación en lux | `bh1750_lux` |
| SPM1423         | Nivel de ruido estimado en dB SPL | `spm1423_noise_db` |

Las etiquetas anteriores se encuentran definidas en `config.h` y se agregan a la carga MQTT mediante `UbidotsClient::addEnv` (`ubidots.cpp`). Cada lectura válida de un sensor se empaqueta con su tag correspondiente y se envía al broker con el identificador de dispositivo `esp_wroom_32`.
