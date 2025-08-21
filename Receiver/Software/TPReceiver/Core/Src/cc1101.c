#include "cc1101.h"

//================Logic to use CS pin correctly====================
static void cs_high(cc1101_t *d)
{
    HAL_GPIO_WritePin(d->cs_port, d->cs_pin, GPIO_PIN_SET);
}

static void cs_low_wait_ready(cc1101_t *d)
{
    HAL_GPIO_WritePin(d->cs_port, d->cs_pin, GPIO_PIN_RESET);
    // Wait until MIISO low (CHIP_RDY Bit)
    uint32_t guard = 0;
    while (HAL_GPIO_ReadPin(CC1101_MISO_GPIO_Port, CC1101_MISO_Pin) == GPIO_PIN_SET && guard++ < 3000){
        __NOP(); __NOP(); __NOP(); __NOP();
    }
}

//========================Helper functions==========================
static uint8_t create_header(uint8_t addr, cc1101_access_mode mode)
{
	uint8_t header = (addr & 0x3F) | mode;
	return header;
}

static c1101_status_word convert_status(uint8_t status_byte)
{
	c1101_status_word status;
	status.chip_rdy = status_byte & 0x80;
	status.state = status_byte & 0x70;
	status.fifo_bytes = status_byte & 0x0F;
	return status;
}


//===================================Register==========================================
void CC1101_write_register(cc1101_t *dev, uint8_t addr, uint8_t val)
{
	//works
	uint8_t header = create_header(addr, CC1101_ACCESS_WRITE);
    uint8_t tx[2] = { header, val };

    cs_low_wait_ready(dev);
    HAL_Delay(1);
    HAL_SPI_Transmit(dev->hspi, tx, 2, 50);
    cs_high(dev);
}

void CC1101_write_register_burst(cc1101_t *dev, uint8_t start_addr, uint8_t *buf, uint8_t len)
{
	//works
	uint8_t header = create_header(start_addr, CC1101_ACCESS_WRITE_BURST);

	cs_low_wait_ready(dev);
    HAL_Delay(1);
    HAL_SPI_Transmit(dev->hspi, &header, 1, 50);
    HAL_SPI_Transmit(dev->hspi, buf, len, 100);
    cs_high(dev);
}

void CC1101_read_register(cc1101_t *dev, uint8_t addr, uint8_t *val)
{
	//works byte 0 is status byte
	uint8_t tx[2] = { create_header(addr, CC1101_ACCESS_READ), 0x00}; // Add dummy byte
	uint8_t rx[2] = {0,0};

	cs_low_wait_ready(dev);
    HAL_Delay(1);
    HAL_SPI_TransmitReceive(dev->hspi, tx, rx, 2, 50);
    cs_high(dev);
    *val = rx[1];
}

void CC1101_read_register_burst(cc1101_t *dev, uint8_t start_addr, uint8_t *arr_val, uint8_t len)
{
	//works
	uint8_t header = create_header(start_addr, CC1101_ACCESS_READ_BURST);

	cs_low_wait_ready(dev);
    HAL_Delay(1);
    HAL_SPI_Transmit(dev->hspi, &header, 1, 50);
    HAL_SPI_Receive(dev->hspi, arr_val, len, 100);
    cs_high(dev);
}


//====================================Strobes==========================================
void CC1101_write_strobe(cc1101_t *dev, uint8_t cmd)
{
	uint8_t tx = create_header(cmd, CC1101_ACCESS_WRITE);

    cs_low_wait_ready(dev);
    HAL_Delay(1);
    HAL_SPI_Transmit(dev->hspi, &tx, 1, 50);
    cs_high(dev);
}

void CC1101_read_strobe(cc1101_t *dev, uint8_t addr, uint8_t *val)
{
	uint8_t tx[2] = { create_header(addr, CC1101_ACCESS_READ), 0x00}; // Add dummy byte
	//uint8_t header = create_header(addr, CC1101_ACCESS_READ);
	uint8_t rx[2] = {0,0};

	cs_low_wait_ready(dev);
    HAL_Delay(1);
    //HAL_SPI_Transmit(dev->hspi, &header, 1, 50);
    //HAL_SPI_Receive(dev->hspi, rx, 2, 50);
    HAL_SPI_TransmitReceive(dev->hspi, tx, rx, 2, 50);
    cs_high(dev);
    *val = rx[1];
}

//=====================================Status===========================================
void CC1101_read_status(cc1101_t *dev, uint8_t addr, uint8_t *val)
{
	//works byte 0 is status byte
	uint8_t tx[2] = { create_header(addr, CC1101_ACCESS_READ_BURST), 0x00}; // Add dummy byte
	uint8_t rx[2] = {0,0};

	cs_low_wait_ready(dev);
    HAL_Delay(1);
    HAL_SPI_TransmitReceive(dev->hspi, tx, rx, 2, 50);
    cs_high(dev);
    *val = rx[1];
}


//===================================FIFO access===========================================

void CC1101_write_TX_FIFO(cc1101_t *dev, uint8_t *buf, uint8_t len)
{
	uint8_t header = create_header(CC1101_FIFO, CC1101_ACCESS_WRITE_BURST);

	cs_low_wait_ready(dev);
    HAL_Delay(1);
    HAL_SPI_Transmit(dev->hspi, &header, 1, 50);
    HAL_SPI_Transmit(dev->hspi, buf, len, 100);
    cs_high(dev);
}

void CC1101_read_RX_FIFO(cc1101_t *dev, uint8_t *arr_val, uint8_t len)
{
	uint8_t header = create_header(CC1101_FIFO, CC1101_ACCESS_READ_BURST);

	cs_low_wait_ready(dev);
    HAL_Delay(1);
    HAL_SPI_Transmit(dev->hspi, &header, 1, 50);
    HAL_SPI_Receive(dev->hspi, arr_val, len, 100);
    cs_high(dev);
}

//================================Power table (patable)===================================

//====================================Configuration=======================================
void CC1101_Init(cc1101_t *dev,
                 SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    dev->hspi    = hspi;
    dev->cs_port = cs_port;
    dev->cs_pin  = cs_pin;

    // CS initial high
    cs_high(dev);
    HAL_Delay(1);

    // Reset Sequence
    cs_low_wait_ready(dev);
    HAL_Delay(1);
    cs_high(dev);
    HAL_Delay(1);
    cs_low_wait_ready(dev);
    HAL_Delay(1);
    CC1101_write_strobe(dev, CC1101_SRES);
    HAL_Delay(2);
    cs_high(dev);
}


void CC1101_Apply433_38k4(cc1101_t *dev)
{
	CC1101_write_strobe(dev, CC1101_SIDLE);
	CC1101_write_strobe(dev, CC1101_SFRX); // RX FIFO leeren

    // sinnvolle Defaults (RX)
    CC1101_write_register(dev, CC1101_IOCFG2,   0x0B); // optional: CCA auf GDO2
    CC1101_write_register(dev, CC1101_IOCFG0,   0x06); // CRC_OK/SYNC auf GDO0 (nur falls genutzt)

    CC1101_write_register(dev, CC1101_FIFOTHR,  0x07);
    CC1101_write_register(dev, CC1101_PKTLEN,   61);    // max Payload
    CC1101_write_register(dev, CC1101_PKTCTRL1, 0x04);  // APPEND_STATUS=1 (RSSI,LQI)
    CC1101_write_register(dev, CC1101_PKTCTRL0, 0x45);  // variable length, CRC on, whitening off

    CC1101_write_register(dev, CC1101_CHANNR,   0x00);
    CC1101_write_register(dev, CC1101_FSCTRL1,  0x06);
    CC1101_write_register(dev, CC1101_FSCTRL0,  0x00);

    // 433.92 MHz (26 MHz XTAL)
    CC1101_write_register(dev, CC1101_FREQ2,    0x10);
    CC1101_write_register(dev, CC1101_FREQ1,    0xB0);
    CC1101_write_register(dev, CC1101_FREQ0,    0x71);

    // 2-FSK ~38.4 kbps
    CC1101_write_register(dev, CC1101_MDMCFG4,  0xCA);
    CC1101_write_register(dev, CC1101_MDMCFG3,  0x83);
    CC1101_write_register(dev, CC1101_MDMCFG2,  0x13);  // 2-FSK, 30/32 Sync
    CC1101_write_register(dev, CC1101_MDMCFG1,  0x22);
    CC1101_write_register(dev, CC1101_MDMCFG0,  0xF8);

    CC1101_write_register(dev, CC1101_DEVIATN,  0x35);  // ~47 kHz
    CC1101_write_register(dev, CC1101_MCSM0,    0x18);  // Autocal bei TX/RX

    CC1101_write_register(dev, CC1101_FREND0,   0x11);
    CC1101_write_register(dev, CC1101_FSCAL3,   0xE9);
    CC1101_write_register(dev, CC1101_FSCAL2,   0x2A);
    CC1101_write_register(dev, CC1101_FSCAL1,   0x00);
    CC1101_write_register(dev, CC1101_FSCAL0,   0x1F);
    CC1101_write_register(dev, CC1101_TEST2,    0x81);
    CC1101_write_register(dev, CC1101_TEST1,    0x35);
    CC1101_write_register(dev, CC1101_TEST0,    0x09);

    // set TX power (not required for RX)
    uint8_t pa = 0xC5;
    CC1101_write_register_burst(dev, CC1101_PATABLE, &pa, 1);
}

void CC1101_EnterRx(cc1101_t *dev)
{
	CC1101_write_strobe(dev, CC1101_SIDLE);
	CC1101_write_strobe(dev, CC1101_SFRX);  // sicherstellen, dass RX FIFO leer ist
	CC1101_write_strobe(dev, CC1101_SRX);
}

