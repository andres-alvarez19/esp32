# 🧩 **Proyecto ESP32 – Monitor Ambiental con Ubidots y OLED**

## 📘 **Descripción general**

El proyecto implementa un **sistema de monitoreo ambiental IoT** basado en un **ESP-WROOM-32**, que mide temperatura, humedad, presión, CO₂ y compuestos orgánicos volátiles (TVOC), mostrando los datos en un **display OLED SSD1306** y enviándolos a **Ubidots STEM** mediante **MQTT**.

Se estructura de forma **modular**, donde cada sensor o servicio está encapsulado en su propio archivo fuente (`.h/.cpp`), facilitando mantenimiento, pruebas y futuras expansiones (por ejemplo, añadir sensores nuevos o cambiar el broker MQTT).

---

## 🏗️ **Estructura del proyecto**

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
├── oled.h
├── oled.cpp
├── ubidots.h
├── ubidots.cpp
├── config.h
├── pins.h
└── (otros futuros módulos opcionales)
```

---

## 🧱 **Componentes y responsabilidades**

### 🔹 **main.ino**

Punto de entrada mínimo del programa:

```cpp
#include "app.h"
App app;

void setup() { app.begin(); }
void loop()  { app.loop(); }
```

➡️ Delegación total del flujo principal a la clase `App`.

---

### 🔹 **app.h / app.cpp**

**Coordinador general del sistema**.
Controla el ciclo completo:

* Inicialización de sensores (`BME280Sensor`, `SGP30Sensor`)
* Configuración del WiFi y Ubidots
* Actualización del OLED
* Publicación de datos en intervalos definidos

Usa un objeto `EnvData` compartido para transferir lecturas entre módulos.

---

### 🔹 **env_data.h**

Estructura central que agrupa todas las variables de entorno:

```cpp
struct EnvData {
  bool hasBme, hasCcs;
  float temp, hum, press, alt; // datos BME280
  float eco2, tvoc;            // datos SGP30
};
```

Es el “bus de datos interno” del sistema, usado por todos los módulos.

---

### 🔹 **bme280.h / bme280.cpp**

Encapsula la lógica del **sensor BME280**:

* Lectura de **temperatura**, **humedad**, **presión** y **altitud**.
* Actualiza los campos correspondientes en `EnvData`.

```cpp
void BME280Sensor::read(EnvData& out);
```

---

### 🔹 **sgp30.h / sgp30.cpp**

Módulo del **sensor SGP30** (reemplazo del CCS811):

* Mide **eCO₂ (ppm)** y **TVOC (ppb)**.
* Incluye **compensación de humedad** usando datos del BME280.
* Controla internamente el intervalo mínimo de lectura (1 Hz).

```cpp
void SGP30Sensor::read(EnvData& io, float tempC, float humPct);
```

---

### 🔹 **oled.h / oled.cpp**

Gestiona el **display OLED SSD1306**:

* Muestra temperatura, humedad, CO₂ y estado de calidad del aire.
* Puede indicar si el nivel de CO₂ es “BUENO / REGULAR / MALO / PELIGRO”.

Se actualiza desde `App::loop()` tras cada publicación.

---

### 🔹 **ubidots.h / ubidots.cpp**

Maneja la comunicación **MQTT con Ubidots**:

* Publica variables con etiquetas (`VAR_TEMP`, `VAR_HUM`, `VAR_CO2_PPM`, etc.)
* Permite suscripción a tópicos para **control remoto de LEDs**.
* Usa la librería `UbidotsEsp32Mqtt`.

```cpp
ubidots.add(label, value);
ubidots.publish(DEVICE_LABEL);
```

---

### 🔹 **config.h**

Contiene la configuración global:

* Token de Ubidots
* SSID y contraseña Wi-Fi
* Etiquetas de variables para Ubidots (por ejemplo: `"bme_temp_c"`, `"ccs811_eco2_ppm"`)

```cpp
#define UBIDOTS_TOKEN "BBUS-XXXXX"
#define WIFI_SSID     "..."
#define WIFI_PASS     "..."
#define VAR_CO2_PPM   "ccs811_eco2_ppm" // ahora medido por SGP30
```

---

### 🔹 **pins.h**

Define los pines del hardware:

```cpp
#define LED_VERDE_PIN 4
#define LED_ROJO_PIN  2
```

Los LEDs se usan como indicadores visuales según el nivel de CO₂.

---

## 🌐 **Flujo de operación**

1. **Inicialización (`App::begin`)**

   * Configura Wi-Fi, sensores, pantalla y MQTT.
2. **Bucle principal (`App::loop`)**

   * Lee sensores (BME280 y SGP30).
   * Publica datos a Ubidots cada 5 s.
   * Actualiza OLED con lecturas actuales.
   * Cambia estado de LEDs según nivel de CO₂.
3. **MQTT**

   * Envia `eco2`, `tvoc`, `temp`, `hum`, etc.
   * Recibe comandos para los LEDs.

---

## 📊 **Rangos ambientales**

Basado en estándares ASHRAE/EPA:

| CO₂ (ppm) | Nivel     | Acción           |
| --------- | --------- | ---------------- |
| 400–800   | Bueno     | Normal           |
| 800–1200  | Regular   | Ventilar         |
| 1200–2000 | Malo      | Urge ventilación |
| >2000     | Peligroso | Aire no apto     |

---

## ⚙️ **Dependencias del proyecto**

Instaladas desde el **Library Manager** de Arduino IDE:

* `Adafruit BME280 Library`
* `Adafruit SGP30`
* `Adafruit SSD1306`
* `Adafruit GFX Library`
* `BH1750` (sensor de luminosidad)
* `UbidotsEsp32Mqtt`
* `WiFi.h` (nativa del ESP32 core)
* `PubSubClient` (incluida por Ubidots)

---

## ✅ **Objetivos del diseño modular**

* Reutilizable: cada sensor es independiente.
* Escalable: se pueden agregar nuevos módulos (`mq135.cpp`, `sd_logger.cpp`, etc.).
* Legible: cada archivo cumple una sola función.
