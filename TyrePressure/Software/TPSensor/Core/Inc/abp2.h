#ifndef INC_ABP2_H_
#define INC_ABP2_H_

#pragma once
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 24-bit scale for pressure
#define ABP2_FULL_SCALE   (16777215.0f)   // 2^24 - 1
#define ABP2_PRESS_MAX    (15099494.0f)   // 90% of FULL_SCALE (used to scale during conversion)
#define ABP2_PRESS_MIN    (1677722.0f)    // 10% of FULL_SCALE (used to scale during conversion)
#define ABP2_TEMP_MAX	  (150.0f)		  // Max temperature in °C used for scaling
#define ABP2_TEMP_MIN	  (-50.0f)		  // Min temperature in °C used for scaling

#define ABP2_TP_MAX		  (1.0f)		  //Tyre Pressure max range (used to scale during conversion)
#define ABP2_TP_MIN		  (0.0f)		  //Tyre Pressure min range (used to scale during conversion)

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint16_t addr8;     // 8-Bit I2C-Adresse (7-bit << 1) — z. B. 0x28 << 1
    					// use uint16_t because required for I2C functions
} abp2_t;

typedef struct {
	bool saturated;
	bool memory_error;
	bool busy;
	bool power;
}abp2_status;


HAL_StatusTypeDef ABP2_Init(abp2_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr7);

abp2_status ABP2_ReadPressure(abp2_t *dev, float *pressure);
abp2_status ABP2_ReadAll(abp2_t *dev, float *pressure, float *temperature, uint8_t *status_raw);


#ifdef __cplusplus
}
#endif


#endif /* INC_ABP2_H_ */
