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
#define CC1101_SNOP      0x3D

// --------- CC1101 Register (Auszug, RX nötig) ---------
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
#define CC1101_FIFO      0x3F


// --------- Status-Register ---------
#define CC1101_PARTNUM   0x30
#define CC1101_VERSION   0x31
#define CC1101_MARCSTATE 0x35
#define CC1101_RXBYTES   0x3B
#define CC1101_LQI       0x33
#define CC1101_RSSI      0x34
#define CC1101_PKTSTATUS 0x38

#define CC1101_MISO_GPIO_Port  SPI1_CS_GPIO_Port

#define CC1101_MISO_Pin        SPI1_CS_Pin

#define CC1101_SPI_TIMEOUT	50


typedef enum {
    CC1101_ACCESS_WRITE = 			0x00, // 00
    CC1101_ACCESS_WRITE_BURST = 	0x40, // 01
    CC1101_ACCESS_READ =			0x80, // 10
    CC1101_ACCESS_READ_BURST = 		0xC0  // 11
} cc1101_access_mode;

typedef enum {
    CC1101_STATE_IDLE = 			0x00, // 000
    CC1101_STATE_RX  = 				0x10, // 001
    CC1101_STATE_TX  =				0x20, // 010
    CC1101_STATE_FSTXON   = 		0x30, // 011
    CC1101_STATE_CALIBRATE  = 		0x40, // 100
    CC1101_STATE_SETTLING   = 		0x50, // 101
    CC1101_STATE_RXOVERFLOW   = 	0x60, // 110
    CC1101_STATETXOVERFLOW   = 		0x70  // 111
} cc1101_state;


typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
} cc1101_t;

typedef struct {
	uint8_t chip_rdy;
	cc1101_state state;
	uint8_t fifo_bytes;
}c1101_status_word;


// ---------- API --------------
void CC1101_Init(cc1101_t *dev, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);

void CC1101_write_register(cc1101_t *dev, uint8_t addr, uint8_t val);
void CC1101_write_register_burst(cc1101_t *dev, uint8_t start_addr, uint8_t *buf, uint8_t len);
void CC1101_read_register(cc1101_t *dev, uint8_t addr, uint8_t *val);
void CC1101_read_register_burst(cc1101_t *dev, uint8_t start_addr, uint8_t *arr_val, uint8_t len);

void CC1101_write_strobe(cc1101_t *dev, uint8_t cmd);
void CC1101_read_strobe(cc1101_t *dev, uint8_t addr, uint8_t *val);

void CC1101_read_status(cc1101_t *dev, uint8_t addr, uint8_t *val);

void CC1101_write_TX_FIFO(cc1101_t *dev, uint8_t *buf, uint8_t len);
void CC1101_read_RX_FIFO(cc1101_t *dev, uint8_t *arr_val, uint8_t len);

// 433,92 MHz, 2-FSK ~38.4 kbps, variable Länge, CRC an, Autocal
void CC1101_Apply433_38k4(cc1101_t *dev);

// In RX gehen (FIFO flush + SRX)
void CC1101_EnterRx(cc1101_t *dev);


#ifdef __cplusplus
}
#endif


#endif /* INC_CC1101_H_ */
