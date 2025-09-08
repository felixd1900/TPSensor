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

static void Create_Header(uint8_t header[3], w5500_add_common_t addr, w5500_bsb_t bsb, w5500_rw rw)
{
    header[0] = (uint8_t)(addr >> 8);
    header[1] =	(uint8_t)(addr & 0xFF);
    header[2] =	(uint8_t) ((bsb << 3) | (rw << 2));
}

void W5500_ReadCommon_Register(w5500_t *dev, w5500_add_common_t addr, uint8_t *rx, uint8_t length)
{
    uint8_t header[3];
    Create_Header(header, addr, W5500_BSB_COM_REG, W5500_RW_READ);

	cs_low(dev);
    HAL_SPI_Transmit(dev->hspi, header, 3, 50);
    HAL_SPI_Receive(dev->hspi, rx, length, 50);
    cs_high(dev);
}

void W5500_WriteCommon_Register(w5500_t *dev, w5500_add_common_t addr, uint8_t *tx, uint8_t length)
{
    uint8_t header[3];
    Create_Header(header, addr, W5500_BSB_COM_REG, W5500_RW_WRITE);

	cs_low(dev);
    HAL_SPI_Transmit(dev->hspi, header, 3, 50);
    HAL_SPI_Transmit(dev->hspi, tx, length, 50);
    cs_high(dev);
}

void W5500_Init(w5500_t *dev, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    dev->hspi    = hspi;
    dev->cs_port = cs_port;
    dev->cs_pin  = cs_pin;

    cs_high(dev);	    // Set CS High
}
