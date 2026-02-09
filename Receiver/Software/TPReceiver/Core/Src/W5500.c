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

static w5500_bsb_t Get_Reg_by_SocketNr(uint8_t socket_nr)
{
	return (w5500_bsb_t) (socket_nr * 4) + 1;
}

static w5500_bsb_t Get_TX_by_SocketNr(uint8_t socket_nr)
{
	return (w5500_bsb_t) (socket_nr * 4) + 2;
}

static w5500_bsb_t Get_RX_by_SocketNr(uint8_t socket_nr)
{
	return (w5500_bsb_t) (socket_nr * 4) + 3;
}

void W5500_Create_Header(uint8_t header[3], uint16_t addr, w5500_bsb_t bsb, w5500_rw rw)
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
    W5500_Create_Header(header, addr, W5500_BSB_COM_REG, W5500_RW_READ);

	cs_low(dev);
    HAL_SPI_Transmit(dev->hspi, header, 3, 50);
    HAL_SPI_Receive(dev->hspi, rx, length, 50);
    cs_high(dev);
}

void W5500_WriteCommon_Register(w5500_t *dev, w5500_add_common_t addr, uint8_t *tx, uint8_t length)
{
    uint8_t header[3];
    W5500_Create_Header(header, addr, W5500_BSB_COM_REG, W5500_RW_WRITE);

	cs_low(dev);
    HAL_SPI_Transmit(dev->hspi, header, 3, 50);
    HAL_SPI_Transmit(dev->hspi, tx, length, 50);
    cs_high(dev);
}

void W5500_ReadSocket(w5500_t *dev, w5500_bsb_t bsb_reg, uint16_t addr, uint8_t *rx, uint8_t length)
{
    uint8_t header[3];
    W5500_Create_Header(header, addr, bsb_reg, W5500_RW_READ);

	cs_low(dev);
    HAL_SPI_Transmit(dev->hspi, header, 3, 50);
    HAL_SPI_Receive(dev->hspi, rx, length, 50);
    cs_high(dev);
}

void W5500_WriteSocket(w5500_t *dev, w5500_bsb_t bsb_reg, uint16_t addr, uint8_t *tx, uint8_t length)
{
    uint8_t header[3];
    W5500_Create_Header(header, addr, bsb_reg, W5500_RW_WRITE);

	cs_low(dev);
    HAL_SPI_Transmit(dev->hspi, header, 3, 50);
    HAL_SPI_Transmit(dev->hspi, tx, length, 50);
    cs_high(dev);
}

uint32_t W5500_WaitSocket_Status(w5500_t *dev, uint8_t socket_nr, w5500_status status_new)
{
	w5500_status status;
	w5500_bsb_t socket_register = Get_Reg_by_SocketNr(socket_nr);
	uint32_t timer_start = HAL_GetTick();

	//wait for state to return to idle after sending fifo
	while(1)
	{
		W5500_ReadSocket(dev, socket_register, W5500_ADD_SOCKET_SR, &status, 1);
		if((status == status_new) | ((HAL_GetTick() - timer_start) > 200))
		{
			return (HAL_GetTick() - timer_start);
		}
		else
		{
			HAL_Delay(1);
		}
	}
}

bool W5500_WaitSocket_SendOK(w5500_t *dev, uint8_t socket_nr)
{
	uint8_t status;
	w5500_bsb_t socket_register = Get_Reg_by_SocketNr(socket_nr);
	uint32_t timer_start = HAL_GetTick();
	bool sendok = false;
	uint8_t sendok_bit = 0x10;

	//wait for state to return to idle after sending fifo
	while(1)
	{
		W5500_ReadSocket(dev, socket_register, W5500_ADD_SOCKET_IR, &status, 1);
		sendok = (status & sendok_bit) != 0;
		if(sendok | ((HAL_GetTick() - timer_start) > 200))
		{
			if(sendok)
			{
				W5500_WriteSocket(dev, socket_register, W5500_ADD_SOCKET_IR, &sendok_bit, 1);
			}
			return sendok;
		}
		else
		{
			HAL_Delay(1);
		}
	}
}



void W5500_WriteSocket_TX(w5500_t *dev, uint8_t socket_nr, uint8_t *tx, uint16_t length)
{
	uint8_t off_old_U8[2];
	w5500_bsb_t socket_register = Get_Reg_by_SocketNr(socket_nr);
	w5500_bsb_t socket_tx = Get_TX_by_SocketNr(socket_nr);

	W5500_ReadSocket(dev, socket_register, W5500_ADD_SOCKET_TX_WR, off_old_U8, 2);					//read TX buffer pointer adresse (offset) --> _TX_WR

	uint16_t off_old_U16 = { ((uint16_t)off_old_U8[0] << 8) | (uint16_t)off_old_U8[1] };			//Convert to uint16

	W5500_WriteSocket(dev, socket_tx, off_old_U16, tx, length);										//Write Buffer to TX at current pointer

    uint16_t off_new_U16 = off_old_U16 + length;													//add tx length to wr pointer
    uint8_t off_new_U8[2] = { (uint8_t)(off_new_U16 >> 8), (uint8_t)off_new_U16 };

	W5500_WriteSocket(dev, socket_register, W5500_ADD_SOCKET_TX_WR, off_new_U8, 2);					//advance pointer to by tx length to signal how many bytes to write
}

void W5500_Configure_SocketUDP(w5500_t *dev, uint8_t socket_nr, uint16_t local_port)
{
	uint8_t p[2] = { (uint8_t)(local_port >> 8), (uint8_t)(local_port & 0xFF) };

	uint8_t rxbuf_size = 0x02;
	uint8_t txbuf_size = 0x02;

	w5500_bsb_t socket_register = Get_Reg_by_SocketNr(socket_nr);

	W5500_WriteSocket(dev, socket_register, W5500_ADD_SOCKET_MR, (uint8_t[]){ 0x02 }, 1);						// UDP Mode
	W5500_WriteSocket(dev, socket_register, W5500_ADD_SOCKET_PORT, p, 2);										// UDP Port 5000
	W5500_WriteSocket(dev, socket_register, W5500_ADD_SOCKET_RXBUF_SIZE, &rxbuf_size, 1);						// RX Buffer Size 2kB (default value)
	W5500_WriteSocket(dev, socket_register, W5500_ADD_SOCKET_TXBUF_SIZE, &txbuf_size, 1);						// TX Buffer Size 2kB (default value)
	W5500_WriteSocket(dev, socket_register, W5500_ADD_SOCKET_CR, (uint8_t[]){ W5500_SOCKET_CR_OPEN }, 1);		// Open Socket
	// Wait for Status Register (w5500_status == W5500_SOCKET_STATUS_UDP)
	// Socket Interrupt Mask already set by default --> Sn_IMR
}


void W5500_SendUDP(w5500_t *dev, uint8_t socket_nr, uint8_t *remote_ip, uint16_t remote_port, uint8_t *tx, uint16_t length)
{
	uint8_t p[2] = { (uint8_t)(remote_port >> 8), (uint8_t)(remote_port & 0xFF) };

	w5500_bsb_t socket_register = Get_Reg_by_SocketNr(socket_nr);

	W5500_WriteSocket(dev, socket_register, W5500_ADD_SOCKET_DIPR, remote_ip, 4);								//write remote ip to _DIPR
	W5500_WriteSocket(dev, socket_register, W5500_ADD_SOCKET_DPORT, p, 2);										//write remote port to _DPORT
	//maybe wait for TX buffer to be free --> _TX_FSR

	W5500_WriteSocket_TX(dev, socket_nr, tx, length);															//read TX buffer pointer adresse (offset) --> _TX_WR
																												//write tx buffer to offset read from _TX_WR
	W5500_WriteSocket(dev, socket_register, W5500_ADD_SOCKET_CR, (uint8_t[]){ W5500_SOCKET_CR_SEND }, 1);		//set _CR to SEND
	W5500_WaitSocket_SendOK(dev, socket_nr);
}

void W5500_ReceiveUDP(w5500_t *dev, uint8_t socket_nr, uint8_t *rx)
{
	//IMP and SMIR need to be enabled for this to work with interrupts
	uint8_t socket_irq = 0xFF;
	w5500_bsb_t socket_register = Get_Reg_by_SocketNr(socket_nr);
	uint16_t num_rx = 0x00;

	W5500_ReadSocket(dev, socket_register, W5500_ADD_SOCKET_TX_WR, &socket_irq, 1);
	if (socket_nr == socket_irq)
	{
		uint8_t pointerU8[2];
		W5500_ReadSocket(dev, socket_register, W5500_ADD_SOCKET_RX_RD, &pointerU8, 2);
		uint16_t off_old_U16 = { ((uint16_t)pointerU8[0] << 8) | (uint16_t)pointerU8[1] };
	}
}
