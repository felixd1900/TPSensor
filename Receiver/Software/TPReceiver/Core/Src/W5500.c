#include "W5500.h"

//================Logic to use CS pin correctly====================
static void cs_high(w5500_t *dev)
{
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

static void cs_low(w5500_t *dev)
{
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
}

static void CreateCommon_Header(uint8_t header[3], w5500_add_common_t addr, w5500_bsb_t bsb, w5500_rw rw)
{
    header[0] = (uint8_t)(addr >> 8);
    header[1] =	(uint8_t)(addr & 0xFF);
    header[2] =	(uint8_t) ((bsb << 3) | (rw << 2));
}

static void CreateSocket_Header(uint8_t header[3], w5500_add_socket_t addr, w5500_bsb_t bsb, w5500_rw rw)
{
    header[0] = (uint8_t)(addr >> 8);
    header[1] =	(uint8_t)(addr & 0xFF);
    header[2] =	(uint8_t) ((bsb << 3) | (rw << 2));
}

void W5500_Init(w5500_t *dev, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    dev->hspi    = hspi;
    dev->cs_port = cs_port;
    dev->cs_pin  = cs_pin;

    cs_high(dev);	    // Set CS High
}

void W5500_ReadCommon_Register(w5500_t *dev, w5500_add_common_t addr, uint8_t *rx, uint8_t length)
{
    uint8_t header[3];
    CreateCommon_Header(header, addr, W5500_BSB_COM_REG, W5500_RW_READ);

	cs_low(dev);
    HAL_SPI_Transmit(dev->hspi, header, 3, 50);
    HAL_SPI_Receive(dev->hspi, rx, length, 50);
    cs_high(dev);
}

void W5500_WriteCommon_Register(w5500_t *dev, w5500_add_common_t addr, uint8_t *tx, uint8_t length)
{
    uint8_t header[3];
    CreateCommon_Header(header, addr, W5500_BSB_COM_REG, W5500_RW_WRITE);

	cs_low(dev);
    HAL_SPI_Transmit(dev->hspi, header, 3, 50);
    HAL_SPI_Transmit(dev->hspi, tx, length, 50);
    cs_high(dev);
}

void W5500_ReadSocket(w5500_t *dev, w5500_add_socket_t addr, w5500_bsb_t socket, uint8_t *rx, uint8_t length)
{
    uint8_t header[3];
    CreateCommon_Header(header, addr, socket, W5500_RW_READ);

	cs_low(dev);
    HAL_SPI_Transmit(dev->hspi, header, 3, 50);
    HAL_SPI_Receive(dev->hspi, rx, length, 50);
    cs_high(dev);
}

void W5500_WriteSocket(w5500_t *dev, w5500_add_socket_t addr, w5500_bsb_t socket, uint8_t *tx, uint8_t length)
{
    uint8_t header[3];
    CreateSocket_Header(header, addr, socket, W5500_RW_WRITE);

	cs_low(dev);
    HAL_SPI_Transmit(dev->hspi, header, 3, 50);
    HAL_SPI_Transmit(dev->hspi, tx, length, 50);
    cs_high(dev);
}

void W5500_Configure_RXSocket(w5500_t *dev, w5500_bsb_t socket_register, uint16_t local_port)
{
	uint8_t p[2];
	p[0] = (uint8_t)(local_port >> 8);
	p[1] = (uint8_t)(local_port & 0xFF);

	W5500_WriteSocket(&w5500, W5500_ADD_SOCKET_MR, socket_register, 0x02, 1);						// UDP Mode
	W5500_WriteSocket(&w5500, W5500_ADD_SOCKET_PORT, socket_register, p, 2);						// UDP Port 5000
	W5500_WriteSocket(&w5500, W5500_ADD_SOCKET_RXBUF_SIZE, socket_register, 0x02, 1);				// RX Buffer Size 2kB (default value)
	W5500_WriteSocket(&w5500, W5500_ADD_SOCKET_CR, socket_register, W5500_SOCKET_CR_OPEN, 1);		// Open Socket
	// Wait for Status Register (w5500_status == W5500_SOCKET_STATUS_UDP)
}

void W5500_Configure_TXSocket(w5500_t *dev, w5500_bsb_t socket_register, uint16_t local_port)
{
	uint8_t p[2];
	p[0] = (uint8_t)(local_port >> 8);
	p[1] = (uint8_t)(local_port & 0xFF);

	W5500_WriteSocket(&w5500, W5500_ADD_SOCKET_MR, socket_register, 0x02, 1);						// UDP Mode
	W5500_WriteSocket(&w5500, W5500_ADD_SOCKET_PORT, socket_register, p, 2);						// UDP Port 5000
	W5500_WriteSocket(&w5500, W5500_ADD_SOCKET_TXBUF_SIZE, socket_register, 0x02, 1);				// RX Buffer Size 2kB (default value)
	W5500_WriteSocket(&w5500, W5500_ADD_SOCKET_CR, socket_register, W5500_SOCKET_CR_OPEN, 1);		// Open Socket
	// Wait for Status Register (w5500_status == W5500_SOCKET_STATUS_UDP)
}

void W5500_Send(w5500_t *dev, uint8_t *remote_ip, uint16_t remote_port)
{
	//write remote ip to _DIPR
	//write remote port to _DPORT
	//maybe wait for TX buffer to be free --> _TX_FSR
	//read TX buffer pointer adresse (offset) --> _TX_WR
	//write tx buffer to offset read from _TX_WR
	//set _CR to SEND
	//wait for sendok or timeout from _IR
}
