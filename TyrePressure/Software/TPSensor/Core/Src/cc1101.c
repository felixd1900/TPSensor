#include "cc1101.h"

static inline void cs_low (cc1101_t *d){ HAL_GPIO_WritePin(d->cs_port, d->cs_pin, GPIO_PIN_RESET); }
static inline void cs_high(cc1101_t *d){ HAL_GPIO_WritePin(d->cs_port, d->cs_pin, GPIO_PIN_SET); }

static uint8_t strobe(cc1101_t *d, uint8_t s)
{
    uint8_t status = 0;
    cs_low(d); HAL_Delay(1);
    HAL_SPI_TransmitReceive(d->hspi, &s, &status, 1, 50);
    cs_high(d);
    return status;
}

static void write_reg(cc1101_t *d, uint8_t addr, uint8_t val)
{
    uint8_t tx[2] = { addr, val };
    cs_low(d); HAL_Delay(1);
    HAL_SPI_Transmit(d->hspi, tx, 2, 50);
    cs_high(d);
}

static void write_burst(cc1101_t *d, uint8_t addr, const uint8_t *buf, uint8_t len)
{
    uint8_t a = addr | CC1101_WRITE_BURST;
    cs_low(d); HAL_Delay(1);
    HAL_SPI_Transmit(d->hspi, &a, 1, 50);
    HAL_SPI_Transmit(d->hspi, (uint8_t*)buf, len, 100);
    cs_high(d);
}

void CC1101_Init(cc1101_t *dev,
                 SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    dev->hspi    = hspi;
    dev->cs_port = cs_port;
    dev->cs_pin  = cs_pin;

    // CS idle high (CS muss im .ioc als GPIO Output konfiguriert sein!)
    cs_high(dev);
    HAL_Delay(1);

    // Reset-Sequenz (vereinfachte Variante)
    cs_low(dev);  HAL_Delay(1);
    cs_high(dev); HAL_Delay(1);
    cs_low(dev);  HAL_Delay(1);
    strobe(dev, CC1101_SRES);
    HAL_Delay(2);
    cs_high(dev);
}

void CC1101_Apply433_38k4(cc1101_t *d)
{
    strobe(d, CC1101_SIDLE);
    strobe(d, CC1101_SFTX);

    // Sinnvolle Defaults (ohne GDO-Nutzung)
    write_reg(d, CC1101_IOCFG2,   0x0B); // optional: CCA/SO
    write_reg(d, CC1101_IOCFG0,   0x06); // CRC_OK/SYNC (nur falls später GDO genutzt)

    write_reg(d, CC1101_FIFOTHR,  0x07);
    write_reg(d, CC1101_PKTLEN,   61);    // max. Payload
    write_reg(d, CC1101_PKTCTRL1, 0x04);  // CRC autoflush aus, kein Addr-Filter
    write_reg(d, CC1101_PKTCTRL0, 0x45);  // variable length, CRC an, whitening aus

    write_reg(d, CC1101_CHANNR,   0x00);
    write_reg(d, CC1101_FSCTRL1,  0x06);  // IF ~152 kHz
    write_reg(d, CC1101_FSCTRL0,  0x00);  // FREQOFF = 0

    // 433.92 MHz bei 26 MHz XTAL
    write_reg(d, CC1101_FREQ2,    0x10);
    write_reg(d, CC1101_FREQ1,    0xB0);
    write_reg(d, CC1101_FREQ0,    0x71);

    // 2-FSK ~38.4 kbps
    write_reg(d, CC1101_MDMCFG4,  0xCA);
    write_reg(d, CC1101_MDMCFG3,  0x83);
    write_reg(d, CC1101_MDMCFG2,  0x13);  // 2-FSK, 30/32 Sync
    write_reg(d, CC1101_MDMCFG1,  0x22);
    write_reg(d, CC1101_MDMCFG0,  0xF8);

    write_reg(d, CC1101_DEVIATN,  0x35);  // ~47 kHz Deviation

    write_reg(d, CC1101_MCSM0,    0x18);  // Autocal beim Eintritt in TX/RX

    write_reg(d, CC1101_FREND0,   0x11);
    write_reg(d, CC1101_FSCAL3,   0xE9);
    write_reg(d, CC1101_FSCAL2,   0x2A);
    write_reg(d, CC1101_FSCAL1,   0x00);
    write_reg(d, CC1101_FSCAL0,   0x1F);
    write_reg(d, CC1101_TEST2,    0x81);
    write_reg(d, CC1101_TEST1,    0x35);
    write_reg(d, CC1101_TEST0,    0x09);

    // Leistung (433 MHz): z. B. +10 dBm
    uint8_t pa = 0xC5;
    write_burst(d, CC1101_PATABLE, &pa, 1);
}

HAL_StatusTypeDef CC1101_SendPacket(cc1101_t *d, const uint8_t *data, uint8_t len)
{
    if (!data || len == 0 || len > 61) return HAL_ERROR;

    strobe(d, CC1101_SIDLE);
    strobe(d, CC1101_SFTX); // TX FIFO flush

    // Länge + Payload in TXFIFO
    uint8_t a = CC1101_TXFIFO;
    cs_low(d); HAL_Delay(1);
    HAL_SPI_Transmit(d->hspi, &a, 1, 20);
    HAL_SPI_Transmit(d->hspi, &len, 1, 20);
    HAL_SPI_Transmit(d->hspi, (uint8_t*)data, len, 200);
    cs_high(d);

    // Senden starten
    strobe(d, CC1101_STX);

    // grobe Wartezeit: bei ~38k4bps reichen einige ms für kurze Payloads
    HAL_Delay(10);

    // zurück in IDLE
    strobe(d, CC1101_SIDLE);
    return HAL_OK;
}

HAL_StatusTypeDef CC1101_SendCounter(cc1101_t *d, uint8_t counter)
{
    uint8_t payload[2];
    payload[0] = 0x01;       // simple „Version“/Typ
    payload[1] = counter;    // dein Counter
    return CC1101_SendPacket(d, payload, sizeof(payload));
}
