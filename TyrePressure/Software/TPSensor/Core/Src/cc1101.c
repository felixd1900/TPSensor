#include "cc1101.h"

static void cs_low(cc1101_t *d)
{
    HAL_GPIO_WritePin(d->cs_port, d->cs_pin, GPIO_PIN_RESET);
}

static void cs_high(cc1101_t *d)
{
    HAL_GPIO_WritePin(d->cs_port, d->cs_pin, GPIO_PIN_SET);
}

void CC1101_Init(cc1101_t *dev,
                 SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    dev->hspi    = hspi;
    dev->cs_port = cs_port;
    dev->cs_pin  = cs_pin;

    // CS idle high
    cs_high(dev);
    HAL_Delay(1);

    // Empfohlene Reset-Sequenz (vereinfacht, ohne SO-Polling):
    cs_low(dev);  HAL_Delay(1);
    cs_high(dev); HAL_Delay(1);
    cs_low(dev);  HAL_Delay(1);
    CC1101_Strobe(dev, CC1101_SRES);
    HAL_Delay(2);
    cs_high(dev);
}

uint8_t CC1101_Strobe(cc1101_t *dev, uint8_t strobe)
{
    uint8_t status = 0;
    cs_low(dev);  HAL_Delay(1);
    HAL_SPI_TransmitReceive(dev->hspi, &strobe, &status, 1, 50);
    cs_high(dev);
    return status;   // enthält u. a. MARCSTATE im oberen Nibble
}

HAL_StatusTypeDef CC1101_ReadStatusReg(cc1101_t *dev, uint8_t status_addr, uint8_t *out_val)
{
    if (!out_val) return HAL_ERROR;
    uint8_t cmd = (uint8_t)(0xC0 | (status_addr & 0x3F)); // Status-Read (11aaaaaa)
    uint8_t dummy = 0x00;
    uint8_t status; // wird beim Senden des cmd zurückgegeben (hier ignoriert)

    cs_low(dev);  HAL_Delay(1);
    if (HAL_SPI_TransmitReceive(dev->hspi, &cmd, &status, 1, 50) != HAL_OK) {
        cs_high(dev);
        return HAL_ERROR;
    }
    if (HAL_SPI_TransmitReceive(dev->hspi, &dummy, out_val, 1, 50) != HAL_OK) {
        cs_high(dev);
        return HAL_ERROR;
    }
    cs_high(dev);
    return HAL_OK;
}
