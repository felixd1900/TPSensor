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

static uint32_t wait_for_state(cc1101_t *dev, cc1101_marc_state state_new)
{
	cc1101_marc_state state = CC1101_MARC_SLEEP;
	uint32_t timer_start = HAL_GetTick();

	//wait for state to return to idle after sending fifo
	while(1)
	{
		CC1101_read_marcstate(dev, &state);
		if((state == state_new) | ((HAL_GetTick() - timer_start) > 200))
		{
			return (HAL_GetTick() - timer_start);
		}
		else
		{
			HAL_Delay(1);
		}
	}
}


//===================================Register==========================================
void CC1101_write_register(cc1101_t *dev, cc1101_register addr, uint8_t val)
{
	//works
	uint8_t header = create_header(addr, CC1101_ACCESS_WRITE);
    uint8_t tx[2] = { header, val };

    cs_low_wait_ready(dev);
    HAL_Delay(1);
    HAL_SPI_Transmit(dev->hspi, tx, 2, 50);
    cs_high(dev);
}

void CC1101_write_register_burst(cc1101_t *dev, cc1101_register start_addr, uint8_t *buf, uint8_t len)
{
	//works
	uint8_t header = create_header(start_addr, CC1101_ACCESS_WRITE_BURST);

	cs_low_wait_ready(dev);
    HAL_Delay(1);
    HAL_SPI_Transmit(dev->hspi, &header, 1, 50);
    HAL_SPI_Transmit(dev->hspi, buf, len, 100);
    cs_high(dev);
}

void CC1101_read_register(cc1101_t *dev, cc1101_register addr, uint8_t *val)
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

void CC1101_read_register_burst(cc1101_t *dev, cc1101_register start_addr, uint8_t *arr_val, uint8_t len)
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
void CC1101_write_strobe(cc1101_t *dev, cc1101_strobe cmd)
{
	//works
	uint8_t tx = create_header(cmd, CC1101_ACCESS_WRITE);

    cs_low_wait_ready(dev);
    HAL_Delay(1);
    HAL_SPI_Transmit(dev->hspi, &tx, 1, 50);
    cs_high(dev);
}

void CC1101_read_strobe(cc1101_t *dev, cc1101_strobe addr, uint8_t *val)
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

//=====================================Status===========================================
void CC1101_read_status(cc1101_t *dev, cc1101_status_register addr, uint8_t *val)
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


//===============================Power table (patable)==================================

void CC1101_write_Patable(cc1101_t *dev, uint8_t *buf)
{
	uint8_t header = create_header(CC1101_PATABLE, CC1101_ACCESS_WRITE_BURST);

	cs_low_wait_ready(dev);
    HAL_Delay(1);
    HAL_SPI_Transmit(dev->hspi, &header, 1, 50);
    HAL_SPI_Transmit(dev->hspi, buf, 8, 100);
    cs_high(dev);
}

void CC1101_read_Patable(cc1101_t *dev, uint8_t *arr_val)
{
	uint8_t header = create_header(CC1101_PATABLE, CC1101_ACCESS_READ_BURST);

	cs_low_wait_ready(dev);
    HAL_Delay(1);
    HAL_SPI_Transmit(dev->hspi, &header, 1, 50);
    HAL_SPI_Receive(dev->hspi, arr_val, 8, 100);
    cs_high(dev);
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


//===================================Status messages===========================================
void CC1101_read_marcstate(cc1101_t *dev, cc1101_marc_state *state)
{
	uint8_t value = 0xFF;
	CC1101_read_status(dev, CC1101_STATUS_MARCSTATE, &value);
	*state = value & 0x1F;
}


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
    CC1101_write_strobe(dev, CC1101_STROBE_SRES);
    HAL_Delay(2);
    cs_high(dev);
}


void CC1101_Apply433_38k4(cc1101_t *dev)
{
	//MCSM1 Bit 2 & 3 are required to stay in RX Mode after receiving a package (SmartRF Studio does not set this by default
	CC1101_write_register(dev, CC1101_REG_MCSM1, 0x3C);   //Main Radio Control State Machine Configuration
	//
	//Rf settings for 433.92MHz TX
	//
	CC1101_write_register(dev, CC1101_REG_IOCFG0, 0x06);  //GDO0 Output Pin Configuration
	CC1101_write_register(dev, CC1101_REG_FIFOTHR, 0x47); //RX FIFO and TX FIFO Thresholds
	CC1101_write_register(dev, CC1101_REG_SYNC1, 0xD3);   //Sync Word, High Byte
	CC1101_write_register(dev, CC1101_REG_SYNC0, 0x91);   //Sync Word, Low Byte
	CC1101_write_register(dev, CC1101_REG_PKTCTRL1, 0x04);//Packet Automation Control
	CC1101_write_register(dev, CC1101_REG_PKTCTRL0, 0x05);//Packet Automation Control
	CC1101_write_register(dev, CC1101_REG_CHANNR, 0x00);  //Channel Number
	CC1101_write_register(dev, CC1101_REG_FSCTRL1, 0x06); //Frequency Synthesizer Control
	CC1101_write_register(dev, CC1101_REG_FREQ2, 0x10);   //Frequency Control Word, High Byte
	CC1101_write_register(dev, CC1101_REG_FREQ1, 0xB0);   //Frequency Control Word, Middle Byte
	CC1101_write_register(dev, CC1101_REG_FREQ0, 0x71);   //Frequency Control Word, Low Byte
	CC1101_write_register(dev, CC1101_REG_MDMCFG4, 0xCA); //Modem Configuration
	CC1101_write_register(dev, CC1101_REG_MDMCFG3, 0x83); //Modem Configuration
	CC1101_write_register(dev, CC1101_REG_MDMCFG2, 0x13); //Modem Configuration
	CC1101_write_register(dev, CC1101_REG_MDMCFG1, 0x42); //Modem Configuration
	CC1101_write_register(dev, CC1101_REG_MDMCFG0, 0xF8); //Modem Configuration
	CC1101_write_register(dev, CC1101_REG_DEVIATN, 0x35); //Modem Deviation Setting
	CC1101_write_register(dev, CC1101_REG_MCSM0, 0x18);   //Main Radio Control State Machine Configuration
	CC1101_write_register(dev, CC1101_REG_FOCCFG, 0x16);  //Frequency Offset Compensation Configuration
	CC1101_write_register(dev, CC1101_REG_AGCCTRL2, 0x43);//AGC Control
	CC1101_write_register(dev, CC1101_REG_WORCTRL, 0xFB); //Wake On Radio Control
	CC1101_write_register(dev, CC1101_REG_FREND0, 0x10);  //Front End TX Configuration
	CC1101_write_register(dev, CC1101_REG_FSCAL3, 0xE9);  //Frequency Synthesizer Calibration
	CC1101_write_register(dev, CC1101_REG_FSCAL2, 0x2A);  //Frequency Synthesizer Calibration
	CC1101_write_register(dev, CC1101_REG_FSCAL1, 0x00);  //Frequency Synthesizer Calibration
	CC1101_write_register(dev, CC1101_REG_FSCAL0, 0x1F);  //Frequency Synthesizer Calibration
	CC1101_write_register(dev, CC1101_REG_TEST2, 0x81);   //Various Test Settings
	CC1101_write_register(dev, CC1101_REG_TEST1, 0x35);   //Various Test Settings
	CC1101_write_register(dev, CC1101_REG_TEST0, 0x09);   //Various Test Settings
}

void CC1101_EnterSleep(cc1101_t *dev)
{
	CC1101_write_strobe(dev, CC1101_STROBE_SIDLE);
	wait_for_state(dev, CC1101_MARC_IDLE);
	CC1101_write_strobe(dev, CC1101_STROBE_SPWD);  // go to sleep (when CS high)
}

//===============================RX Methods================================
void CC1101_EnterRx(cc1101_t *dev)
{
	CC1101_write_strobe(dev, CC1101_STROBE_SIDLE);
	wait_for_state(dev, CC1101_MARC_IDLE);
	CC1101_write_strobe(dev, CC1101_STROBE_SFRX);  // clear RX FIFO
	CC1101_write_strobe(dev, CC1101_STROBE_SRX);
	wait_for_state(dev, CC1101_MARC_RX);
}

void CC1101_Recover_RX_Overflow(cc1101_t *dev)
{
	CC1101_write_strobe(dev, CC1101_STROBE_SFRX);  // clear RX FIFO
	wait_for_state(dev, CC1101_MARC_IDLE);
	CC1101_write_strobe(dev, CC1101_STROBE_SRX);
	wait_for_state(dev, CC1101_MARC_RX);
}

void CC1101_Set_Rx_threshold(cc1101_t *dev, c1101_rx_threshold val)
{
	uint8_t fifothr = 0x00;
	CC1101_read_register(dev, CC1101_REG_FIFOTHR, &fifothr);
	CC1101_write_register(dev, CC1101_REG_FIFOTHR, (fifothr | val));
}

int16_t CC1101_Convert_RSSI(uint8_t rssi_raw)
{
	int16_t rssi = 0;
	if(rssi_raw < 128)
	{
		rssi = rssi_raw / 2 - CC1101_RSSI_OFFSET;
	}
	else
	{
		rssi = (rssi_raw - 256) / 2 - CC1101_RSSI_OFFSET;
	}
	return rssi;
}

void CC1101_Get_RSSI(cc1101_t *dev, int16_t *rssi)
{
	uint8_t rssi_raw = 0;
	CC1101_read_status(dev, CC1101_STATUS_RSSI, &rssi_raw);
	*rssi = CC1101_Convert_RSSI(rssi_raw);
}

//===============================TX Methods================================
void CC1101_PrepareTx(cc1101_t *dev)
{
	CC1101_write_strobe(dev, CC1101_STROBE_SIDLE);
	wait_for_state(dev, CC1101_MARC_IDLE);
	CC1101_write_strobe(dev, CC1101_STROBE_SFTX);  // clear TX FIFO
}

void CC1101_Set_Tx_power(cc1101_t *dev, cc1101_tx_power power)
{
	uint8_t pa_table[8] = {0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	pa_table[0] = power;
	CC1101_write_Patable(dev, pa_table);
}

uint32_t CC1101_SendTx(cc1101_t *dev)
{
	//expected state when starting is idle
	CC1101_write_strobe(dev, CC1101_STROBE_STX);  			// send Tx FIFO
	return wait_for_state(dev, CC1101_MARC_IDLE);			//wait to return to idle state
}
