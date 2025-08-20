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
#define CC1101_SRX       0x34
#define CC1101_SIDLE     0x36
#define CC1101_SFRX      0x3A

// --------- CC1101 Register (Auszug, RX nötig) ---------
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
#define CC1101_RXFIFO    0x3F

// SPI Flags
#define CC1101_READ_BURST  0xC0

// --------- Status-Register ---------
#define CC1101_PARTNUM   0x30
#define CC1101_VERSION   0x31
#define CC1101_MARCSTATE 0x35
#define CC1101_RXBYTES   0x3B
#define CC1101_LQI       0x33
#define CC1101_RSSI      0x34
#define CC1101_PKTSTATUS 0x38

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
} cc1101_t;

// ---- API ----
void CC1101_Init(cc1101_t *dev,
                 SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *cs_port, uint16_t cs_pin);

// 433,92 MHz, 2-FSK ~38.4 kbps, variable Länge, CRC an, Autocal
void CC1101_Apply433_38k4(cc1101_t *dev);

// In RX gehen (FIFO flush + SRX)
void CC1101_EnterRx(cc1101_t *dev);

// Statusregister lesen (ein Byte)
uint8_t CC1101_ReadStatus(cc1101_t *dev, uint8_t status_addr);

// Versucht einmal, ein komplettes Packet aus dem RX-FIFO zu lesen
// Gibt HAL_OK bei Erfolg (payload/len/RSSI/LQI/CRC_OK gefüllt), HAL_BUSY wenn keins da.
HAL_StatusTypeDef CC1101_TryReadPacket(cc1101_t *dev,
                                       uint8_t *payload, uint8_t *len, uint8_t maxlen,
                                       int8_t *rssi_dbm, uint8_t *lqi, uint8_t *crc_ok);

typedef enum {
    CC1101_ACCESS_WRITE = 0x00, // 00
    CC1101_ACCESS_WRITE_BURST  = 0x40, // 01
    CC1101_ACCESS_READ  = 0x80, // 10
    CC1101_ACCESS_READ_BURST   = 0xC0  // 11
} cc1101_access_mode;

#ifdef __cplusplus
}
#endif


#endif /* INC_CC1101_H_ */
