#ifndef INC_CC1101_H_
#define INC_CC1101_H_

#pragma once
#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- Strobes (Befehle) ----
#define CC1101_SRES      0x30  // Reset

// ---- Status-Register-Adressen (5 LSBits) ----
// (Status-Regs werden mit 0xC0 | addr gelesen; addr hier = 0x30..)
#define CC1101_PARTNUM   0x30
#define CC1101_VERSION   0x31

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
} cc1101_t;

// Basis
void              CC1101_Init(cc1101_t *dev,
                              SPI_HandleTypeDef *hspi,
                              GPIO_TypeDef *cs_port, uint16_t cs_pin);

uint8_t           CC1101_Strobe(cc1101_t *dev, uint8_t strobe);

// Statusregister lesen (liest 1 Byte aus z. B. PARTNUM/ VERSION)
HAL_StatusTypeDef CC1101_ReadStatusReg(cc1101_t *dev, uint8_t status_addr, uint8_t *out_val);

#ifdef __cplusplus
}
#endif


#endif /* INC_CC1101_H_ */
