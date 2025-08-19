#ifndef INC_CC1101_H_
#define INC_CC1101_H_

#pragma once
#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// --------- CC1101 Strobes ---------
#define CC1101_SRES      0x30
#define CC1101_SIDLE     0x36
#define CC1101_STX       0x35
#define CC1101_SFTX      0x3B

// --------- CC1101 Register (Auszug, TX nötig) ---------
#define CC1101_IOCFG2    0x00
#define CC1101_IOCFG0    0x02
#define CC1101_FIFOTHR   0x03
#define CC1101_PKTLEN    0x06
#define CC1101_PKTCTRL1  0x07
#define CC1101_PKTCTRL0  0x08
#define CC1101_CHANNR    0x0A
#define CC1101_FSCTRL1   0x0B
#define CC1101_FSCTRL0   0x0C
#define CC1101_FREQ2     0x0D
#define CC1101_FREQ1     0x0E
#define CC1101_FREQ0     0x0F
#define CC1101_MDMCFG4   0x10
#define CC1101_MDMCFG3   0x11
#define CC1101_MDMCFG2   0x12
#define CC1101_MDMCFG1   0x13
#define CC1101_MDMCFG0   0x14
#define CC1101_DEVIATN   0x15
#define CC1101_MCSM0     0x18
#define CC1101_FREND0    0x22
#define CC1101_FSCAL3    0x23
#define CC1101_FSCAL2    0x24
#define CC1101_FSCAL1    0x25
#define CC1101_FSCAL0    0x26
#define CC1101_TEST2     0x2C
#define CC1101_TEST1     0x2D
#define CC1101_TEST0     0x2E
#define CC1101_PATABLE   0x3E
#define CC1101_TXFIFO    0x3F

// SPI Flags
#define CC1101_WRITE_BURST 0x40

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
} cc1101_t;

// ---- API ----
// Grund-Setup (SPI Handle + CS Pin), macht die Reset-Sequenz
void CC1101_Init(cc1101_t *dev,
                 SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *cs_port, uint16_t cs_pin);

// 433,92 MHz, 2-FSK ~38.4 kbps, variable Länge, CRC an, Autocal beim TX/RX
void CC1101_Apply433_38k4(cc1101_t *dev);

// Kurzes Paket senden (variable Länge, CRC durch CC1101)
HAL_StatusTypeDef CC1101_SendPacket(cc1101_t *dev, const uint8_t *data, uint8_t len);

// Komfort: nur einen Counter senden (Payload = {counter})
HAL_StatusTypeDef CC1101_SendCounter(cc1101_t *dev, uint8_t counter);

#ifdef __cplusplus
}
#endif



#endif /* INC_CC1101_H_ */
