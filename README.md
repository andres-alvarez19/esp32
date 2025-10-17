🧩 Proyecto ESP32 – Monitor Ambiental con Ubidots y OLED

📘 Descripción general

El proyecto implementa un sistema de monitoreo ambiental IoT basado en un ESP-WROOM-32, que mide temperatura, humedad, presión, CO₂, compuestos orgánicos volátiles (TVOC), luminosidad (BH1750) y contaminación acústica con un micrófono SPM1423, mostrando los datos en un display OLED SSD1306 y enviándolos a Ubidots STEM mediante MQTT.

Se estructura de forma modular, donde cada sensor o servicio está encapsulado en su propio archivo fuente (.h/.cpp), facilitando mantenimiento, pruebas y futuras expansiones (por ejemplo, añadir sensores nuevos o cambiar el broker MQTT).

🏗️ Estructura del proyecto

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
├── ubidots.h
├── ubidots.cpp
├── config.h
├── pins.h
└── (otros futuros módulos opcionales)
```

🧱 Componentes y responsabilidades

🔹 **main.ino**

Punto de entrada mínimo del programa:

```cpp
#include "app.h"
App app;

void setup() { app.begin(); }
void loop()  { app.loop(); }
```

➡️ Delegación total del flujo principal a la clase App.

---

🔹 **app.h / app.cpp**

Coordinador general del sistema. Controla el ciclo completo:

* Inicialización de sensores (BME280Sensor, SGP30Sensor, BH1750Sensor, SPM1423Sensor)
* Configuración del WiFi y Ubidots
* Actualización del OLED
* Publicación de datos en intervalos definidos

Usa un objeto `EnvData` compartido para transferir lecturas entre módulos.

---

🔹 **env_data.h**

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

---

🔹 **bme280.h / bme280.cpp**

Encapsula la lógica del sensor BME280:

* Lectura de temperatura, humedad, presión y altitud.
* Actualiza los campos correspondientes en EnvData.

```cpp
void BME280Sensor::read(EnvData& out);
```

---

🔹 **sgp30.h / sgp30.cpp**

Módulo del sensor SGP30 (reemplazo del CCS811):

* Mide eCO₂ (ppm) y TVOC (ppb).
* Incluye compensación de humedad usando datos del BME280.
* Controla internamente el intervalo mínimo de lectura (1 Hz).

```cpp
void SGP30Sensor::read(EnvData& io, float tempC, float humPct);
```

---

🔹 **bh1750.h / bh1750.cpp**

Sensor digital de luminosidad BH1750:

* Mide iluminancia ambiental en lux (rango: 1 lx a 65535 lx).
* Permite determinar condiciones de iluminación interior.
* Se comunica por I²C (dirección 0x23 u 0x5C).

```cpp
void BH1750Sensor::read(EnvData& out);
```

---

🔹 **spm1423.h / spm1423.cpp**

Módulo del micrófono digital SPM1423:

* Captura muestras PDM a través del periférico I2S del ESP32.
* Calcula el nivel sonoro en dB SPL aproximados a partir del valor RMS.
* Expone banderas en EnvData para indicar si hay lectura válida (`hasNoise`).

```cpp
void SPM1423Sensor::read(EnvData& out);
```

Los datos se usan para evaluar la contaminación acústica en tiempo real.

---

🔹 **oled.h / oled.cpp**

Gestiona el display OLED SSD1306:

* Muestra temperatura, humedad, CO₂, lux y estado de calidad del aire.
* Puede indicar si el nivel de CO₂ es “BUENO / REGULAR / MALO / PELIGRO”.

Se actualiza desde `App::loop()` tras cada publicación.

---

🔹 **ubidots.h / ubidots.cpp**

Maneja la comunicación MQTT con Ubidots:

* Publica variables con etiquetas (`VAR_TEMP`, `VAR_HUM`, `VAR_CO2_PPM`, `VAR_NOISE_DB`, etc.)
* Permite suscripción a tópicos para control remoto de LEDs.
* Usa la librería `UbidotsEsp32Mqtt`.

```cpp
ubidots.add(label, value);
ubidots.publish(DEVICE_LABEL);
```

---

🔹 **config.h**

Contiene la configuración global:

* Token de Ubidots
* SSID y contraseña Wi-Fi
* Etiquetas de variables para Ubidots (por ejemplo: `"bme_temp_c"`, `"sgp30_eco2_ppm"`)

```cpp
#define UBIDOTS_TOKEN "BBUS-XXXXX"
#define WIFI_SSID     "..."
#define WIFI_PASS     "..."
#define VAR_CO2_PPM   "sgp30_eco2_ppm"
```

---

🔹 **pins.h**

Define los pines del hardware:

```cpp
#define LED_VERDE_PIN 13
#define LED_ROJO_PIN  12
```

Los LEDs se usan como indicadores visuales según el nivel de CO₂ y las condiciones ambientales.

---

🌐 Flujo de operación

1. **Inicialización (App::begin)**  → Configura Wi-Fi, sensores, pantalla y MQTT.
2. **Bucle principal (App::loop)**  → Lee sensores (BME280, SGP30, BH1750, SPM1423).
3. **Publicación** → Envía datos a Ubidots cada 5 s.
4. **Visualización** → Actualiza OLED y LEDs según condiciones.
5. **MQTT** → Permite monitoreo remoto y control básico.

---

📊 **Rangos ambientales recomendados (interiores saludables)**

Basados en estándares **ASHRAE**, **EPA**, y **OMS**:

| Parámetro                | Unidad | Rango Óptimo | Nivel       | Recomendación                                       |
| ------------------------ | ------ | ------------ | ----------- | --------------------------------------------------- |
| **Temperatura**          | °C     | 20 – 25      | Confortable | Ideal para interiores habitados                     |
| **Humedad Relativa**     | %      | 40 – 60      | Confortable | Menor crecimiento de moho y buena sensación térmica |
| **Presión atmosférica**  | hPa    | 1000 ± 10    | Normal      | Valores fuera pueden indicar clima cambiante        |
| **CO₂ (eCO₂)**           | ppm    | 400 – 800    | Bueno       | Normal, aire fresco                                 |
|                          |        | 800 – 1200   | Regular     | Se recomienda ventilar                              |
|                          |        | 1200 – 2000  | Malo        | Urge ventilación                                    |
|                          |        | >2000        | Peligroso   | Aire no apto, riesgo de fatiga y somnolencia        |
| **TVOC**                 | ppb    | < 300        | Bueno       | Sin contaminantes perceptibles                      |
|                          |        | 300 – 600    | Regular     | Posibles fuentes químicas leves                     |
|                          |        | > 600        | Alto        | Ventilar o revisar fuentes de VOC                   |
| **Iluminancia (BH1750)** | lux    | 300 – 500    | Adecuada    | Ideal para oficinas o habitaciones                  |
|                          |        | < 150        | Baja        | Insuficiente para tareas visuales                   |
|                          |        | > 1000       | Muy alta    | Podría generar deslumbramiento                      |
| **Ruido (SPM1423)**      | dB(A)  | 30 – 50      | Silencioso  | Nivel típico de oficina o hogar tranquilo           |
|                          |        | 50 – 70      | Moderado    | Conversaciones normales o electrodomésticos         |
|                          |        | > 70         | Alto        | Puede generar fatiga auditiva o estrés              |

---

⚙️ **Dependencias del proyecto**

Instaladas desde el Library Manager de Arduino IDE:

* Adafruit BME280 Library
* Adafruit SGP30
* Adafruit SSD1306
* Adafruit GFX Library
* BH1750 (sensor de luminosidad)
* driver/i2s (incluido en el core ESP32; necesario para el micrófono SPM1423)
* UbidotsEsp32Mqtt
* WiFi.h (nativa del ESP32 core)
* PubSubClient (incluida por Ubidots)

---

✅ **Objetivos del diseño modular**

* **Reutilizable:** cada sensor es independiente.
* **Escalable:** se pueden agregar nuevos módulos (`mq135.cpp`, `sd_logger.cpp`, etc.).
* **Legible:** cada archivo cumple una sola función.

---

📎 **Autor:** Andrés Álvarez Morales
📅 **Versión:** Octubre 2025
📡 **Plataforma:** ESP-WROOM-32 + Ubidots STEM
