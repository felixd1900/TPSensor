#ifndef INC_CC1101_H_
#define INC_CC1101_H_

#pragma once
#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============CC1101 Strobes===============
#define CC1101_SRES      0x30	//Reset Chip
#define CC1101_SFSTXON	 0x31	//Enable and calibrate frequency synthesizer (if MCSM0.FS_AUTOCAL=1). If in RX (with CCA): Go to a wait state where only the synthesizer is running (for quick RX / TX turnaround).
#define CC1101_SXOFF	 0x32	//Turn off crystal osszilator
#define CC1101_SCAL	 	 0x33	//Calibrate frequency synthesizer and turn it off. SCAL can be strobed from IDLE mode without setting manual calibration mode (MCSM0.FS_AUTOCAL=0)
#define CC1101_SRX       0x34	//Enable RX. Perform calibration first if coming from IDLE and MCSM0.FS_AUTOCAL=1.
#define CC1101_STX		 0x35	//In IDLE state: Enable TX. Perform calibration first if MCSM0.FS_AUTOCAL=1. If in RX state and CCA is enabled: Only go to TX if channel is clear.
#define CC1101_SIDLE     0x36	//Exit RX / TX, turn off frequency synthesizer and exit Wake-On-Radio mode if applicable.
#define CC1101_SWOR		 0x38	//Start automatic RX polling sequence (Wake-on-Radio) as described in Section 19.5 if WORCTRL.RC_PD=0.
#define CC1101_SPWD		 0x39	//Enter power down mode when CSn goes high.
#define CC1101_SFRX      0x3A	//Flush the RX FIFO buffer. Only issue SFRX in IDLE or RXFIFO_OVERFLOW states.
#define CC1101_SFTX		 0x3B	//Flush the TX FIFO buffer. Only issue SFTX in IDLE or TXFIFO_UNDERFLOW states.
#define CC1101_SWORRST	 0x3C	// Reset real time clock to Event1 value.
#define CC1101_SNOP      0x3D	//No operation. May be used to get access to the chip status byte.

//===============CC1101 Register=============
#define CC1101_IOCFG2    0x00	//GDO2 output pin configuration
#define CC1101_IOCFG1    0x01	//GDO1 output pin configuration
#define CC1101_IOCFG0    0x02	//GDO0 output pin configuration
#define CC1101_FIFOTHR   0x03	//RX FIFO and TX FIFO thresholds
#define CC1101_SYNC1     0x04	//Sync word, high byte
#define CC1101_SYNC0     0x05	//Sync word, low byte
#define CC1101_PKTLEN    0x06	//Packet length
#define CC1101_PKTCTRL1  0x07	//Packet automation control
#define CC1101_PKTCTRL0  0x08	//Packet automation control
#define CC1101_ADDR		 0x09	//Device address
#define CC1101_CHANNR    0x0A	//Channel number
#define CC1101_FSCTRL1   0x0B	//Frequency synthesizer control
#define CC1101_FSCTRL0   0x0C	//Frequency synthesizer control
#define CC1101_FREQ2     0x0D	//Frequency control word, high byte
#define CC1101_FREQ1     0x0E	//Frequency control word, middle byte
#define CC1101_FREQ0     0x0F	//Frequency control word, low byte
#define CC1101_MDMCFG4   0x10	//Modem configuration
#define CC1101_MDMCFG3   0x11	//Modem configuration
#define CC1101_MDMCFG2   0x12	//Modem configuration
#define CC1101_MDMCFG1   0x13	//Modem configuration
#define CC1101_MDMCFG0   0x14	//Modem configuration
#define CC1101_DEVIATN   0x15	//Modem configuration
#define CC1101_MCSM2     0x16	//Main Radio Control State Machine configuration
#define CC1101_MCSM1     0x17	//Main Radio Control State Machine configuration
#define CC1101_MCSM0     0x18	//Main Radio Control State Machine configuration
#define CC1101_FOCCFG    0x19	//Frequency Offset Compensation configuration
#define CC1101_BSCFG     0x1A	//Bit Synchronization configuration
#define CC1101_AGCTRL2   0x1B	//AGC control
#define CC1101_AGCTRL1   0x1C	//AGC control
#define CC1101_AGCTRL0   0x1D	//AGC control
#define CC1101_WOREVT1   0x1E	//High byte Event 0 timeout
#define CC1101_WOREVT0   0x1F	//Low byte Event 0 timeout
#define CC1101_WORCTRL   0x20	//Wake On Radio control
#define CC1101_FREND1    0x21	//Front end RX configuration
#define CC1101_FREND0    0x22	//Front end TX configuration
#define CC1101_FSCAL3    0x23	//Frequency synthesizer calibration
#define CC1101_FSCAL2    0x24	//Frequency synthesizer calibration
#define CC1101_FSCAL1    0x25	//Frequency synthesizer calibration
#define CC1101_FSCAL0    0x26	//Frequency synthesizer calibration
#define CC1101_RCCTRL1   0x27	//RC oscillator configuration
#define CC1101_RCCTRL0   0x28	//RC oscillator configuration
#define CC1101_FSTEST    0x29	//Frequency synthesizer calibration control
#define CC1101_PTEST     0x2A	//Production test
#define CC1101_AGCTEST   0x2B	//AGC test
#define CC1101_TEST2     0x2C	//Various test settings
#define CC1101_TEST1     0x2D	//Various test settings
#define CC1101_TEST0     0x2E	//Various test settings


//==============FIFOs====================
#define CC1101_PATABLE   		0x3E
#define CC1101_FIFO      		0x3F


//=============Status-Register============
#define CC1101_PARTNUM   		0x30	//Part number for CC1101
#define CC1101_VERSION   		0x31	//Current version number
#define CC1101_FREQEST   		0x32	//Frequency Offset Estimate
#define CC1101_LQI       		0x33	//Demodulator estimate for Link Quality
#define CC1101_RSSI 	 		0x34	//Received signal strength indication
#define CC1101_MARCSTATE 		0x35	//Control state machine state
#define CC1101_WORTIME1  		0x36	//High byte of WOR timer
#define CC1101_WORTIME0  		0x37	//Low byte of WOR timer
#define CC1101_PKTSTATUS 		0x38	//Current GDOx status and packet status
#define CC1101_VCO_VC_DAC 		0x39	//Current setting from PLL calibration module
#define CC1101_TXBYTES   		0x3A	//Underflow and number of bytes in the TX FIFO
#define CC1101_RXBYTES   		0x3B	//Overflow and number of bytes in the RX FIFO
#define CC1101_RCCTRL1_STATUS 	0x3C	//Last RC oscillator calibration result
#define CC1101_RCCTRL1_STATUS 	0x3D	//Last RC oscillator calibration result


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
    CC1101_STATE_TXOVERFLOW   = 	0x70  // 111
} cc1101_state;

typedef enum {
	CC1101_MARC_SLEEP =				0x00,
	CC1101_MARC_IDLE =				0X01,
	CC1101_MARC_X0FF =				0X02,
	CC1101_MARC_VCOON_MC =			0X03,
	CC1101_MARC_REGON_MC =			0x04,
	CC1101_MARC_MANCAL =			0x05,
	CC1101_MARC_VCOON =				0x06,
	CC1101_MARC_REGON =				0x07,
	CC1101_MARC_STARTCAL =			0x08,
	CC1101_MARC_BWBOOST =			0x09,
	CC1101_MARC_FS_LOCK =			0x0A,
	CC1101_MARC_IFADCON =			0x0B,
	CC1101_MARC_ENDCAL =			0x0C,
	CC1101_MARC_RX =				0x0D,
	CC1101_MARC_RX_END =			0x0E,
	CC1101_MARC_RX_RST =			0x0F,
	CC1101_MARC_TXRX_SWITCH =		0x10,
	CC1101_MARC_RXFIFO_OVERFLOW =	0x11,
	CC1101_MARC_FSTXON =			0x12,
	CC1101_MARC_TX =				0x13,
	CC1101_MARC_TX_END =			0x14,
	CC1101_MARC_RXTX_SWITCH =		0x15,
	CC1101_MARC_TXFIFO_UNDERFLOW =	0x16
}cc1101_marc_state;

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
