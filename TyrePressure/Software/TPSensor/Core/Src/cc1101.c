#include "cc1101.h"

// ---------- CS-Helper ----------
static inline void cs_high(cc1101_t *d){
    HAL_GPIO_WritePin(d->cs_port, d->cs_pin, GPIO_PIN_SET);
}
static inline void cs_low_wait_ready(cc1101_t *d){
    HAL_GPIO_WritePin(d->cs_port, d->cs_pin, GPIO_PIN_RESET);
    // Warten bis MISO low (Chip ready)
    uint32_t guard = 0;
    while (HAL_GPIO_ReadPin(CC1101_MISO_GPIO_Port, CC1101_MISO_Pin) == GPIO_PIN_SET && guard++ < 3000){
        __NOP(); __NOP(); __NOP(); __NOP();
    }
}

// ---------- Init/Reset ----------
void CC1101_Init(cc1101_t *dev,
                 SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    dev->hspi    = hspi;
    dev->cs_port = cs_port;
    dev->cs_pin  = cs_pin;

    cs_high(dev);
    HAL_Delay(1);

    // Reset-Sequenz + SRES (0x30)
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET); HAL_Delay(1);
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);   HAL_Delay(1);
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET); HAL_Delay(1);

    uint8_t sres = CC1101_SRES, dummy = 0;
    cs_low_wait_ready(dev);
    HAL_SPI_TransmitReceive(dev->hspi, &sres, &dummy, 1, 50);
    cs_high(dev);
    HAL_Delay(2);
}

// ---------- Config R/W ----------
uint8_t CC1101_ReadConfig_Single(cc1101_t *d, uint8_t addr)
{
    uint8_t hdr = CC1101_HDR_READ_SINGLE(addr);
    uint8_t val = 0;
    cs_low_wait_ready(d);
    HAL_SPI_Transmit(d->hspi, &hdr, 1, 50);
    __NOP(); __NOP(); __NOP(); __NOP();
    HAL_SPI_Receive (d->hspi, &val, 1, 50);
    cs_high(d);
    return val;
}

HAL_StatusTypeDef CC1101_ReadConfig_Burst(cc1101_t *d, uint8_t start_addr, uint8_t *dst, uint8_t len)
{
    if (!dst || len == 0) return HAL_ERROR;
    uint8_t hdr = CC1101_HDR_READ_BURST(start_addr);
    cs_low_wait_ready(d);
    if (HAL_SPI_Transmit(d->hspi, &hdr, 1, 50) != HAL_OK){ cs_high(d); return HAL_ERROR; }
    HAL_StatusTypeDef r = HAL_SPI_Receive(d->hspi, dst, len, 100);
    cs_high(d);
    return r;
}

HAL_StatusTypeDef CC1101_WriteConfig_Single(cc1101_t *d, uint8_t addr, uint8_t val)
{
    uint8_t tx[2] = { CC1101_HDR_WRITE_SINGLE(addr), val };
    cs_low_wait_ready(d);
    HAL_StatusTypeDef r = HAL_SPI_Transmit(d->hspi, tx, 2, 50);
    cs_high(d);
    return r;
}

HAL_StatusTypeDef CC1101_WriteConfig_Burst(cc1101_t *d, uint8_t start_addr, const uint8_t *src, uint8_t len)
{
    if (!src || len == 0) return HAL_ERROR;
    uint8_t hdr = CC1101_HDR_WRITE_BURST(start_addr);
    cs_low_wait_ready(d);
    if (HAL_SPI_Transmit(d->hspi, &hdr, 1, 50) != HAL_OK){ cs_high(d); return HAL_ERROR; }
    HAL_StatusTypeDef r = HAL_SPI_Transmit(d->hspi, (uint8_t*)src, len, 100);
    cs_high(d);
    return r;
}

// ---------- Status (nur Single) ----------
uint8_t CC1101_ReadStatus(cc1101_t *d, uint8_t status_addr)
{
    uint8_t hdr = CC1101_HDR_READ_SINGLE(status_addr);
    uint8_t val = 0;
    cs_low_wait_ready(d);
    HAL_SPI_Transmit(d->hspi, &hdr, 1, 50);
    __NOP(); __NOP(); __NOP(); __NOP();
    HAL_SPI_Receive (d->hspi, &val, 1, 50);
    cs_high(d);
    return val;
}

HAL_StatusTypeDef CC1101_ReadPartVersion(cc1101_t *d, uint8_t *part, uint8_t *ver)
{
    if (!part || !ver) return HAL_ERROR;
    *part = CC1101_ReadStatus(d, CC1101_PARTNUM);  // erwartet 0x00
    *ver  = CC1101_ReadStatus(d, CC1101_VERSION);  // bei dir ~0x14/0x0F
    return HAL_OK;
}

// ---------- 433,92 MHz @ ~38,4 kbps 2-FSK ----------
void CC1101_Apply433_38k4(cc1101_t *d)
{
    // in IDLE + TX-FIFO leeren
    (void)CC1101_ReadStatus(d, CC1101_MARCSTATE);
    uint8_t sidle = CC1101_SIDLE, sftx = CC1101_SFTX;
    cs_low_wait_ready(d); HAL_SPI_Transmit(d->hspi, &sidle, 1, 50); cs_high(d);
    cs_low_wait_ready(d); HAL_SPI_Transmit(d->hspi, &sftx,  1, 50); cs_high(d);

    // Packet/Modem
    CC1101_WriteConfig_Single(d, CC1101_IOCFG2,   0x0B); // optional (GDO2)
    CC1101_WriteConfig_Single(d, CC1101_IOCFG0,   0x06); // optional (GDO0)
    CC1101_WriteConfig_Single(d, CC1101_FIFOTHR,  0x07);
    CC1101_WriteConfig_Single(d, CC1101_PKTLEN,   61);
    CC1101_WriteConfig_Single(d, CC1101_PKTCTRL1, 0x04); // append status
    CC1101_WriteConfig_Single(d, CC1101_PKTCTRL0, 0x45); // var len, CRC on, whitening off

    CC1101_WriteConfig_Single(d, CC1101_CHANNR,   0x00);
    CC1101_WriteConfig_Single(d, CC1101_FSCTRL1,  0x06);
    CC1101_WriteConfig_Single(d, CC1101_FSCTRL0,  0x00);

    // 433.92 MHz @ 26 MHz XTAL
    CC1101_WriteConfig_Single(d, CC1101_FREQ2,    0x10);
    CC1101_WriteConfig_Single(d, CC1101_FREQ1,    0xB0);
    CC1101_WriteConfig_Single(d, CC1101_FREQ0,    0x71);

    // 2-FSK ~38.4 kbps
    CC1101_WriteConfig_Single(d, CC1101_MDMCFG4,  0xCA);
    CC1101_WriteConfig_Single(d, CC1101_MDMCFG3,  0x83);
    CC1101_WriteConfig_Single(d, CC1101_MDMCFG2,  0x13); // 30/32 sync
    CC1101_WriteConfig_Single(d, CC1101_MDMCFG1,  0x22);
    CC1101_WriteConfig_Single(d, CC1101_MDMCFG0,  0xF8);

    CC1101_WriteConfig_Single(d, CC1101_DEVIATN,  0x35); // ~20.6 kHz (passt zur RxBW)
    CC1101_WriteConfig_Single(d, CC1101_MCSM0,    0x18); // autocal on TX/RX

    CC1101_WriteConfig_Single(d, CC1101_FREND0,   0x11);
    CC1101_WriteConfig_Single(d, CC1101_FSCAL3,   0xE9);
    CC1101_WriteConfig_Single(d, CC1101_FSCAL2,   0x2A);
    CC1101_WriteConfig_Single(d, CC1101_FSCAL1,   0x00);
    CC1101_WriteConfig_Single(d, CC1101_FSCAL0,   0x1F);
    CC1101_WriteConfig_Single(d, CC1101_TEST2,    0x81);
    CC1101_WriteConfig_Single(d, CC1101_TEST1,    0x35);
    CC1101_WriteConfig_Single(d, CC1101_TEST0,    0x09);

    // PA (1 byte reicht)
    CC1101_WriteConfig_Single(d, CC1101_PATABLE,  0xC5); // ~+10 dBm
}

// ---------- Senden ----------
HAL_StatusTypeDef CC1101_SendPacket(cc1101_t *d, const uint8_t *data, uint8_t len)
{
    if (!data || len == 0 || len > 61) return HAL_ERROR;

    // IDLE + TX FIFO flush
    uint8_t sidle = CC1101_SIDLE, sftx = CC1101_SFTX, stx = CC1101_STX;
    cs_low_wait_ready(d); HAL_SPI_Transmit(d->hspi, &sidle, 1, 50); cs_high(d);
    cs_low_wait_ready(d); HAL_SPI_Transmit(d->hspi, &sftx,  1, 50); cs_high(d);

    // TXFIFO schreiben: [len][payload...]
    uint8_t hdr = CC1101_HDR_WRITE_BURST(CC1101_TXFIFO);
    cs_low_wait_ready(d);
    if (HAL_SPI_Transmit(d->hspi, &hdr, 1, 50) != HAL_OK){ cs_high(d); return HAL_ERROR; }
    if (HAL_SPI_Transmit(d->hspi, &len, 1, 50)      != HAL_OK){ cs_high(d); return HAL_ERROR; }
    if (HAL_SPI_Transmit(d->hspi, (uint8_t*)data, len, 200) != HAL_OK){ cs_high(d); return HAL_ERROR; }
    cs_high(d);

    // Senden
    cs_low_wait_ready(d); HAL_SPI_Transmit(d->hspi, &stx, 1, 50); cs_high(d);

    // kurze Wartezeit (kleine Payload) + zurück in IDLE
    HAL_Delay(10);
    cs_low_wait_ready(d); HAL_SPI_Transmit(d->hspi, &sidle, 1, 50); cs_high(d);
    return HAL_OK;
}

HAL_StatusTypeDef CC1101_SendCounter(cc1101_t *d, uint8_t counter)
{
    uint8_t p[2] = { 0x01, counter };
    return CC1101_SendPacket(d, p, sizeof p);
}
