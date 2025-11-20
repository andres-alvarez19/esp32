# 🧩 Proyecto ESP32 – Monitor Ambiental con Ubidots y OLED

## 📘 Descripción general

El proyecto implementa un sistema de monitoreo ambiental IoT basado en un **ESP-WROOM-32**, que mide **temperatura, humedad, presión, CO₂, compuestos orgánicos volátiles (TVOC), luminosidad** y **contaminación acústica**, mostrando los datos en un display **OLED SSD1306** y enviándolos a **Thingsboard** mediante **MQTT**.

Se estructura de forma modular, donde cada sensor o servicio está encapsulado en su propio archivo fuente (`.h` / `.cpp`), facilitando mantenimiento, pruebas y futuras expansiones (por ejemplo, añadir sensores nuevos o cambiar el broker MQTT).

---

## 📈 Variables medidas por cada sensor

| Sensor                                      | Variables medidas      | Unidad | Campo en `EnvData`                         | Descripción                                            |
| :------------------------------------------ | :--------------------- | :----- | :----------------------------------------- | :----------------------------------------------------- |
| **BME280** (Temperatura, Humedad y Presión) | Temperatura            | °C     | `temp`                                     | Temperatura ambiente actual                            |
|                                             | Humedad relativa       | %      | `hum`                                      | Porcentaje de humedad ambiental                        |
|                                             | Presión atmosférica    | hPa    | `press`                                    | Presión del aire al nivel del sensor                   |
|                                             | Altitud (calculada)    | m      | `alt`                                      | Estimación de altura según presión y nivel del mar     |
| **SGP30** (Calidad del aire)                | eCO₂ (equivalente CO₂) | ppm    | `eco2`                                     | Estimación de concentración de CO₂                     |
|                                             | TVOC                   | ppb    | `tvoc`                                     | Concentración de compuestos orgánicos volátiles        |
| **BH1750** (Sensor de luz)                  | Iluminancia            | lux    | `lux`                                      | Intensidad lumínica ambiental                          |
| **SPM1423** (Micrófono digital)             | Nivel de ruido         | dB(A)  | `noiseDb`                                  | Nivel sonoro estimado en decibelios                    |

Estas variables son publicadas periódicamente en Ubidots mediante etiquetas (`VAR_TEMP`, `VAR_HUM`, `VAR_CO2_PPM`, etc.) definidas en `config.h`.
---
# Proyecto ESP32 – Monitor Ambiental

## Módulos y variables publicadas en Ubidots

| Módulo / Sensor | Descripción | Variable (tag) MQTT |
|-----------------|-------------|---------------------|
| BME280          | Temperatura, humedad relativa, presión y altitud calculada | `bme_temp_c`, `bme_hum_pct`, `bme_press_hpa`, `bme_alt_m` |
| SGP30           | CO₂ equivalente y compuestos orgánicos volátiles totales | `sgp30_eco2_ppm`, `sgp30_tvoc_ppb` |
| BH1750          | Medición de iluminación en lux | `bh1750_lux` |
| SPM1423         | Nivel de ruido estimado en dB SPL | `spm1423_noise_db` |

Las etiquetas anteriores se encuentran definidas en `config.h` y se agregan a la carga MQTT mediante `UbidotsClient::addEnv` (`ubidots.cpp`). Cada lectura válida de un sensor se empaqueta con su tag correspondiente y se envía al broker con el identificador de dispositivo `esp_wroom_32`.
---

## 🗷️ Estructura del proyecto

Ubicados todos en la carpeta principal (modo compatible con Arduino IDE 2.3.6):

```
Proyecto-ESP32/
├── main.ino
├── app.h
├── app.cpp
├── env_data.h
├── bme280.h
├── bme280.cpp
├── sgp30.h
├── sgp30.cpp
├── bh1750.h
├── bh1750.cpp
├── spm1423.h
├── spm1423.cpp
├── oled.h
├── oled.cpp
├── thingsboard.h
├── thingsboard.cpp
├── config.h
├── pins.h
└── (otros futuros módulos opcionales)
```

---

## 🧱 Componentes y responsabilidades

### 🔹 main.ino

Punto de entrada mínimo del programa:

```cpp
#include "app.h"
App app;

void setup() { app.begin(); }
void loop()  { app.loop(); }
```

➡️ Delegación total del flujo principal a la clase `App`.

### 🔹 app.h / app.cpp

Coordinador general del sistema. Controla el ciclo completo:

* Inicialización de sensores (`BME280Sensor`, `SGP30Sensor`, `BH1750Sensor`, `SPM1423Sensor`)
* Configuración del WiFi y Ubidots
* Actualización del OLED
* Publicación de datos en intervalos definidos

Usa un objeto `EnvData` compartido para transferir lecturas entre módulos.

### 🔹 env_data.h

Estructura central que agrupa todas las variables de entorno:

```cpp
struct EnvData {
  bool hasBme, hasCcs;
  bool hasLight, hasNoise;
  float temp, hum, press, alt; // datos BME280
  float eco2, tvoc;            // datos SGP30
  float lux;                   // datos BH1750
  float noiseDb;               // datos SPM1423
};
```

Es el “bus de datos interno” del sistema, usado por todos los módulos.

### 🔹 bme280.h / bme280.cpp

Lectura de temperatura, humedad, presión y altitud.
`void BME280Sensor::read(EnvData& out);`

### 🔹 sgp30.h / sgp30.cpp

Medición de eCO₂ y TVOC con compensación de humedad.
`void SGP30Sensor::read(EnvData& io, float tempC, float humPct);`

### 🔹 bh1750.h / bh1750.cpp

Lectura de iluminancia ambiental (lux).
`void BH1750Sensor::read(EnvData& out);`

### 🔹 spm1423.h / spm1423.cpp

Cálculo del nivel sonoro en dB SPL (I2S).
`void SPM1423Sensor::read(EnvData& out);`

### 🔹 oled.h / oled.cpp

Muestra temperatura, humedad, CO₂, lux y estado de calidad del aire.

### 🔹 ubidots.h / ubidots.cpp

Publica las variables al dashboard IoT mediante MQTT.

### 🔹 config.h

Configura credenciales, etiquetas y tokens.

### 🔹 pins.h

Define pines de hardware (LEDs, buzzer, I2C, etc.)

---

## 🌐 Flujo de operación

1. **Inicialización** → Configura Wi-Fi, sensores, pantalla y MQTT.
2. **Bucle principal** → Lee sensores (`BME280`, `SGP30`, `BH1750`, `SPM1423`).
3. **Publicación** → Envía datos a Ubidots cada 5 s.
4. **Visualización** → Actualiza OLED y LEDs.
5. **MQTT** → Permite monitoreo y control remoto.

---

## 📊 Rangos ambientales recomendados (interiores saludables)

| Parámetro            | Unidad | Rango Óptimo | Nivel       | Recomendación                          |
| :------------------- | :----- | :----------- | :---------- | :------------------------------------- |
| **Temperatura**      | °C     | 20 – 25      | Confortable | Ideal para interiores habitados        |
| **Humedad Relativa** | %      | 40 – 60      | Confortable | Reduce moho y mejora sensación térmica |
| **Presión**          | hPa    | 1000 ± 10    | Normal      | Desviaciones indican cambios de clima  |
| **CO₂ (eCO₂)**       | ppm    | 400–800      | Bueno       | Aire fresco                            |
|                      |        | 800–1200     | Regular     | Ventilar                               |
|                      |        | 1200–2000    | Malo        | Urge ventilación                       |
|                      |        | >2000        | Peligroso   | Aire no apto                           |
| **TVOC**             | ppb    | <300         | Bueno       | Sin contaminantes perceptibles         |
|                      |        | 300–600      | Regular     | Posibles fuentes químicas              |
|                      |        | >600         | Alto        | Revisar fuentes VOC                    |
| **Iluminancia**      | lux    | 300–500      | Adecuada    | Ideal para oficinas                    |
|                      |        | <150         | Baja        | Insuficiente                           |
|                      |        | >1000        | Muy alta    | Riesgo de deslumbramiento              |
| **Ruido (SPM1423)**  | Normalizada  | 5–20         | Silencioso  | Habitación tranquila                  |
|                      |        | 20–50        | Moderado    | Conversación normal                    |
|                      |        | 40–70          | Alto        | Música fuerte / TV              |
|                      |        | 80–100           | Muy Alto        | Golpe o sonido fuerte cerca             |


---

## ⚙️ Dependencias del proyecto

Instaladas desde el **Library Manager de Arduino IDE**:

* Adafruit BME280 Library
* Adafruit SGP30
* Adafruit SSD1306
* Adafruit GFX Library
* BH1750
* driver/i2s *(para el micrófono SPM1423)*
* arduinojson
* thingsboard
* WiFi.h
* PubSubClient

---

## ✅ Objetivos del diseño modular

* **Reutilizable:** cada sensor es independiente
* **Escalable:** se pueden agregar nuevos módulos (p. ej. `mq135.cpp`, `sd_logger.cpp`)
* **Legible:** cada archivo cumple una sola función

---

💎 **Autor:** Andrés Álvarez Morales
🗓 **Versión:** Noviembre 2025
📡 **Plataforma:** ESP-WROOM-32 + Thingsboard (Ufro Host)
