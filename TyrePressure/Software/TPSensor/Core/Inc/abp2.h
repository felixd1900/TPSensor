#ifndef INC_ABP2_H_
#define INC_ABP2_H_

#pragma once
#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 24-bit Vollskala
#define ABP2_FULL_SCALE   (16777215.0f)   // 2^24 - 1
#define ABP2_OUT_MAX      (15099494.0f)   // ~90% von FULL_SCALE
#define ABP2_OUT_MIN      (1677722.0f)    // ~10% von FULL_SCALE
#define ABP2_RX_LEN       7               // 7 Bytes wie im Arduino-Beispiel

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint16_t addr8;     // 8-Bit I2C-Adresse (7-bit << 1) — z. B. 0x28 << 1
    float pmin;         // z. B. 0.0f
    float pmax;         // z. B. 1.0f (bar/kPa/psi – Einheit ist egal, nur Range)
} abp2_t;

/** Initialisiert das Device-Objekt und pingt die Adresse einmal an.
 *  addr7 = 7-Bit-Adresse (z. B. 0x28)
 */
HAL_StatusTypeDef ABP2_Init(abp2_t *dev, I2C_HandleTypeDef *hi2c,
                            uint8_t addr7, float pmin, float pmax);

/** Sendet {0xAA,0x00,0x00}, wartet delay_ms und liest 7 Bytes. */
HAL_StatusTypeDef ABP2_RequestAndRead(const abp2_t *dev,
                                      uint8_t data[ABP2_RX_LEN],
                                      uint32_t delay_ms);

/** Zerlegt die 7 Bytes in Status, 24-bit Druck-/Temperaturzähler. */
void ABP2_Parse(const uint8_t raw[ABP2_RX_LEN], uint8_t *status,
                uint32_t *press_counts, uint32_t *temp_counts);

/** Rechnet Zähler in physikalische Werte um (Druck/°C/%FS). */
void ABP2_Convert(const abp2_t *dev, uint32_t press_counts, uint32_t temp_counts,
                  float *pressure, float *temperature_c, float *percentage);

/** Komfortfunktion: Request+Read+Parse+Convert in einem Schritt. */
HAL_StatusTypeDef ABP2_ReadAll(const abp2_t *dev,
                               float *pressure, float *temperature_c, float *percentage,
                               uint8_t *status, uint32_t *press_counts, uint32_t *temp_counts);

#ifdef __cplusplus
}
#endif


#endif /* INC_ABP2_H_ */
