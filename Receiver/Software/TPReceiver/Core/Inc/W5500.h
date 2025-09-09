#ifndef INC_W5500_H_
#define INC_W5500_H_

#pragma once
#include "main.h"
#include <stdint.h>

#define W5505_MISO_GPIO_Port  	SPI1_CS_GPIO_Port
#define W5505_MISO_Pin        	SPI1_CS_Pin

typedef enum {
	W5500_RW_READ = 	0x00,
	W5500_RW_WRITE = 	0x01
}w5500_rw;

typedef enum {
    W5500_BSB_COM_REG   = 0x00, // Common Register

    W5500_BSB_0_REG     = 0x01,
    W5500_BSB_0_TX      = 0x02,
    W5500_BSB_0_RX      = 0x03,

    W5500_BSB_1_REG     = 0x05,
    W5500_BSB_1_TX      = 0x06,
    W5500_BSB_1_RX      = 0x07,

    W5500_BSB_2_REG     = 0x09,
    W5500_BSB_2_TX      = 0x0A,
    W5500_BSB_2_RX      = 0x0B,

    W5500_BSB_3_REG     = 0x0D,
    W5500_BSB_3_TX      = 0x0E,
    W5500_BSB_3_RX      = 0x0F,

    W5500_BSB_4_REG     = 0x11,
    W5500_BSB_4_TX      = 0x12,
    W5500_BSB_4_RX      = 0x13,

    W5500_BSB_5_REG     = 0x15,
    W5500_BSB_5_TX      = 0x16,
    W5500_BSB_5_RX      = 0x17,

    W5500_BSB_6_REG     = 0x19,
    W5500_BSB_6_TX      = 0x1A,
    W5500_BSB_6_RX      = 0x1B,

    W5500_BSB_7_REG     = 0x1D,
    W5500_BSB_7_TX      = 0x1E,
    W5500_BSB_7_RX      = 0x1F,
} w5500_bsb_t;

// Common Register: nur Startadressen für Blockzugriffe
typedef enum {
    W5500_ADD_COMMON_MR        = 0x0000,
    W5500_ADD_COMMON_GAR       = 0x0001, // 4B
    W5500_ADD_COMMON_SUBR      = 0x0005, // 4B
    W5500_ADD_COMMON_SHAR      = 0x0009, // 6B
    W5500_ADD_COMMON_SIPR      = 0x000F, // 4B
    W5500_ADD_COMMON_INTLEVEL  = 0x0013, // 2B
    W5500_ADD_COMMON_IR        = 0x0015, // 1B
    W5500_ADD_COMMON_IMR       = 0x0016, // 1B
    W5500_ADD_COMMON_SIR       = 0x0017, // 1B
    W5500_ADD_COMMON_SIMR      = 0x0018, // 1B
    W5500_ADD_COMMON_RTR       = 0x0019, // 2B
    W5500_ADD_COMMON_RCR       = 0x001B, // 1B
    W5500_ADD_COMMON_PTIMER    = 0x001C, // 1B
    W5500_ADD_COMMON_PMAGIC    = 0x001D, // 1B
    W5500_ADD_COMMON_PHAR      = 0x001E, // 6B
    W5500_ADD_COMMON_PSID      = 0x0024, // 2B
    W5500_ADD_COMMON_PMRU      = 0x0026, // 2B
    W5500_ADD_COMMON_UIPR      = 0x0028, // 4B
    W5500_ADD_COMMON_UPORTR    = 0x002C, // 2B
    W5500_ADD_COMMON_PHYCFGR   = 0x002E, // 1B
    W5500_ADD_COMMON_VERSIONR  = 0x0039  // 1B
} w5500_add_common_t;

// Socket-n Register: ebenfalls nur Basis-Offsets
typedef enum {
    W5500_ADD_SOCKET_MR        = 0x0000, // 1B
    W5500_ADD_SOCKET_CR        = 0x0001, // 1B
    W5500_ADD_SOCKET_IR        = 0x0002, // 1B
    W5500_ADD_SOCKET_SR        = 0x0003, // 1B

    W5500_ADD_SOCKET_PORT      = 0x0004, // 2B (Sn_PORT0/1)
    W5500_ADD_SOCKET_DHAR      = 0x0006, // 6B
    W5500_ADD_SOCKET_DIPR      = 0x000C, // 4B
    W5500_ADD_SOCKET_DPORT     = 0x0010, // 2B
    W5500_ADD_SOCKET_MSSR      = 0x0012, // 2B
    // 0x0014 reserved
    W5500_ADD_SOCKET_TOS       = 0x0015, // 1B
    W5500_ADD_SOCKET_TTL       = 0x0016, // 1B
    W5500_ADD_SOCKET_RXBUF_SIZE= 0x001E, // 1B
    W5500_ADD_SOCKET_TXBUF_SIZE= 0x001F, // 1B

    W5500_ADD_SOCKET_TX_FSR    = 0x0020, // 2B
    W5500_ADD_SOCKET_TX_RD     = 0x0022, // 2B
    W5500_ADD_SOCKET_TX_WR     = 0x0024, // 2B
    W5500_ADD_SOCKET_RX_RSR    = 0x0026, // 2B
    W5500_ADD_SOCKET_RX_RD     = 0x0028, // 2B
    W5500_ADD_SOCKET_RX_WR     = 0x002A, // 2B

    W5500_ADD_SOCKET_IMR       = 0x002C, // 1B
    W5500_ADD_SOCKET_FRAG      = 0x002D, // 2B
    W5500_ADD_SOCKET_KPALVTR   = 0x002F  // 1B
} w5500_add_socket_t;

typedef enum {
	W5500_SOCKET_CR_OPEN 		= 0x01,
	W5500_SOCKET_CR_LISTEN 		= 0x02,
	W5500_SOCKET_CR_CONNECT		= 0x04,
	W5500_SOCKET_CR_DISCON 		= 0x08,
	W5500_SOCKET_CR_CLOSE 		= 0x10,
	W5500_SOCKET_CR_SEND 		= 0x20,
	W5500_SOCKET_CR_SENDMAC 	= 0x21,
	W5500_SOCKET_CR_SENDKEEP 	= 0x22,
	W5500_SOCKET_CR_RECV 		= 0x40
}w5500_socket_cr;

typedef enum {
	W5500_SOCKET_STATUS_CLOSED 			= 0x00,
	W5500_SOCKET_STATUS_INIT 			= 0x13,
	W5500_SOCKET_STATUS_LISTEN 			= 0x14,
	W5500_SOCKET_STATUS_ESTABLISHED 	= 0x17,
	W5500_SOCKET_STATUS_CLOSEWAIT 		= 0x1C,
	W5500_SOCKET_STATUS_UDP 			= 0x22,
	W5500_SOCKET_STATUS_MACRAW 			= 0x42,

	W5500_SOCKET_STATUS_SYNSENT 		= 0x15,
	W5500_SOCKET_STATUS_SYNRECV 		= 0x16,
	W5500_SOCKET_STATUS_FINWAIT 		= 0x18,
	W5500_SOCKET_STATUS_CLOSING 		= 0x1A,
	W5500_SOCKET_STATUS_TIMEWAIT 		= 0x1B,
	W5500_SOCKET_STATUS_LASTACK 		= 0x1D,
}w5500_status;

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
} w5500_t;

//==================== Functions ========================
void W5500_Init(w5500_t *dev, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);

void W5500_ReadCommon_Register(w5500_t *dev, w5500_add_common_t addr, uint8_t *rx, uint8_t length);
void W5500_WriteCommon_Register(w5500_t *dev, w5500_add_common_t addr, uint8_t *tx, uint8_t length);

void W5500_ReadSocket(w5500_t *dev, w5500_add_socket_t addr, w5500_bsb_t socket, uint8_t *rx, uint8_t length);
void W5500_WriteSocket(w5500_t *dev, w5500_add_socket_t addr, w5500_bsb_t socket, uint8_t *tx, uint8_t length);

#endif /* INC_W5500_H_ */
