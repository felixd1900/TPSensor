#pragma once
#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// -------- Config-Register (Auszug) --------
#define CC1101_IOCFG2    0x00
#define CC1101_IOCFG1    0x01
#define CC1101_IOCFG0    0x02
#define CC1101_FIFOTHR   0x03
#define CC1101_SYNC1     0x04
#define CC1101_SYNC0     0x05
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
#define CC1101_RXFIFO    0x3F

// -------- Status-Register --------
#define CC1101_PARTNUM   0x30  // erwartet 0x00
#define CC1101_VERSION   0x31  // ~0x14… (bei dir 0x14/0x0F plausibel)
#define CC1101_MARCSTATE 0x35
#define CC1101_PKTLSTATUS 0x38
#define CC1101_TXBYTES   0x3A
#define CC1101_RXBYTES   0x3B

// -------- Strobes --------
#define CC1101_SRES      0x30
#define CC1101_SRX       0x34
#define CC1101_STX       0x35
#define CC1101_SIDLE     0x36
#define CC1101_SFTX      0x3B
#define CC1101_SFRX      0x3A

// -------- SPI-Header --------
#define CC1101_HDR_READ_SINGLE(addr)   ((uint8_t)(0x80 | ((addr) & 0x3F))) // 10aaaaaa
#define CC1101_HDR_READ_BURST(addr)    ((uint8_t)(0xC0 | ((addr) & 0x3F))) // 11aaaaaa
#define CC1101_HDR_WRITE_SINGLE(addr)  ((uint8_t)((addr) & 0x3F))          // 00aaaaaa
#define CC1101_HDR_WRITE_BURST(addr)   ((uint8_t)(0x40 | ((addr) & 0x3F))) // 01aaaaaa

// -------- Handle --------
typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t     cs_pin;
} cc1101_t;

// Optional: MISO-Pin für „wait for ready“ (anpassen falls nicht PA6)
#ifndef CC1101_MISO_GPIO_Port
#define CC1101_MISO_GPIO_Port  GPIOA
#endif
#ifndef CC1101_MISO_Pin
#define CC1101_MISO_Pin        GPIO_PIN_6
#endif

// -------- API --------
void CC1101_Init(cc1101_t *dev,
                 SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *cs_port, uint16_t cs_pin);

// Config-R/W
uint8_t  CC1101_ReadConfig_Single(cc1101_t *dev, uint8_t addr);
HAL_StatusTypeDef CC1101_ReadConfig_Burst (cc1101_t *dev, uint8_t start_addr, uint8_t *dst, uint8_t len);
HAL_StatusTypeDef CC1101_WriteConfig_Single(cc1101_t *dev, uint8_t addr, uint8_t val);
HAL_StatusTypeDef CC1101_WriteConfig_Burst (cc1101_t *dev, uint8_t start_addr, const uint8_t *src, uint8_t len);

// Status (nur Single!)
uint8_t  CC1101_ReadStatus(cc1101_t *dev, uint8_t status_addr);

// Komfort
HAL_StatusTypeDef CC1101_ReadPartVersion(cc1101_t *dev, uint8_t *part, uint8_t *ver);

// Funk-Setup & Senden
void     CC1101_Apply433_38k4(cc1101_t *dev);
HAL_StatusTypeDef CC1101_SendPacket(cc1101_t *dev, const uint8_t *data, uint8_t len);
HAL_StatusTypeDef CC1101_SendCounter(cc1101_t *dev, uint8_t counter);

#ifdef __cplusplus
}
#endif
