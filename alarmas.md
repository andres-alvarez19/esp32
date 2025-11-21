# Alarms

# ✔ Alarmas de Temperatura

| Condición                    | Nombre del `alarmType` |
| ---------------------------- | ---------------------- |
| Temperatura fuera de 20–25°C | **TEMP_OUT_OF_RANGE**  |

---

# ✔ Alarmas de Humedad

| Condición               | Nombre del `alarmType` |
| ----------------------- | ---------------------- |
| Humedad fuera de 40–60% | **HUM_OUT_OF_RANGE**   |

---

# ✔ Alarmas de CO₂ (SGP30)

Divididas por severidad:

| Rango (ppm) | Nombre del `alarmType` |
| ----------- | ---------------------- |
| 800–1200    | **CO2_REGULAR**        |
| 1200–2000   | **CO2_BAD**            |
| >2000       | **CO2_DANGEROUS**      |

---

# ✔ Alarmas de TVOC

| Rango (ppb) | Nombre del `alarmType` |
| ----------- | ---------------------- |
| 300–600     | **TVOC_REGULAR**       |
| >600        | **TVOC_HIGH**          |

---

# ✔ Alarmas de Luz (BH1750)

| Condición | Nombre del `alarmType` |
| --------- | ---------------------- |
| <150 lux  | **LUX_LOW**            |
| >1000 lux | **LUX_HIGH**           |

---

# ✔ Alarmas de Ruido (SPM1423 normalizado 0–100)

| Rango | Nombre del `alarmType` |
| ----- | ---------------------- |
| 40–70 | **NOISE_MODERATE**     |
| 70–80 | **NOISE_HIGH**         |
| >80   | **NOISE_IMPACT**       |

*(Los nombres son exactamente los que aparecen en el JSON del rule chain.)*

---

## En total se crean estas 11 alarmas exactas:

```
TEMP_OUT_OF_RANGE
HUM_OUT_OF_RANGE

CO2_REGULAR
CO2_BAD
CO2_DANGEROUS

TVOC_REGULAR
TVOC_HIGH

LUX_LOW
LUX_HIGH

NOISE_MODERATE
NOISE_HIGH
NOISE_IMPACT
```
