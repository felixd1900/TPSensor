#include "cc1101.h"


static void cs_low (cc1101_t *d)
{
	HAL_GPIO_WritePin(d->cs_port, d->cs_pin, GPIO_PIN_RESET);
}
static void cs_high(cc1101_t *d)
{
	HAL_GPIO_WritePin(d->cs_port, d->cs_pin, GPIO_PIN_SET);
}

static uint8_t create_header(uint8_t addr, cc1101_access_mode mode)
{
	uint8_t header = (addr & 0x3F) | mode;
	return header;
}

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

static void write_burst(cc1101_t *d, uint8_t addr, uint8_t *buf, uint8_t len)
{
	uint8_t header = create_header(addr, CC1101_ACCESS_WRITE_BURST);
    cs_low(d); HAL_Delay(1);
    HAL_SPI_Transmit(d->hspi, &header, 1, 50);
    HAL_SPI_Transmit(d->hspi, buf, len, 100);
    cs_high(d);
}

uint8_t CC1101_ReadStatus(cc1101_t *d, uint8_t status_addr)
{
    uint8_t a  = 0xC0 | (status_addr & 0x3F);
    uint8_t rx = 0;
    cs_low(d); HAL_Delay(1);
    HAL_SPI_Transmit(d->hspi, &a, 1, 50);
    HAL_SPI_Receive (d->hspi, &rx, 1, 50);
    cs_high(d);
    return rx;
}

void CC1101_Init(cc1101_t *dev,
                 SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    dev->hspi    = hspi;
    dev->cs_port = cs_port;
    dev->cs_pin  = cs_pin;

    // CS muss bereits als GPIO-Output konfiguriert sein – wir setzen nur High:
    cs_high(dev);
    HAL_Delay(1);

    // Reset-Sequenz
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
    strobe(d, CC1101_SFRX); // RX FIFO leeren

    // sinnvolle Defaults (RX)
    write_reg(d, CC1101_IOCFG2,   0x0B); // optional: CCA auf GDO2
    write_reg(d, CC1101_IOCFG0,   0x06); // CRC_OK/SYNC auf GDO0 (nur falls genutzt)

    write_reg(d, CC1101_FIFOTHR,  0x07);
    write_reg(d, CC1101_PKTLEN,   61);    // max Payload
    write_reg(d, CC1101_PKTCTRL1, 0x04);  // APPEND_STATUS=1 (RSSI,LQI)
    write_reg(d, CC1101_PKTCTRL0, 0x45);  // variable length, CRC on, whitening off

    write_reg(d, CC1101_CHANNR,   0x00);
    write_reg(d, CC1101_FSCTRL1,  0x06);
    write_reg(d, CC1101_FSCTRL0,  0x00);

    // 433.92 MHz (26 MHz XTAL)
    write_reg(d, CC1101_FREQ2,    0x10);
    write_reg(d, CC1101_FREQ1,    0xB0);
    write_reg(d, CC1101_FREQ0,    0x71);

    // 2-FSK ~38.4 kbps
    write_reg(d, CC1101_MDMCFG4,  0xCA);
    write_reg(d, CC1101_MDMCFG3,  0x83);
    write_reg(d, CC1101_MDMCFG2,  0x13);  // 2-FSK, 30/32 Sync
    write_reg(d, CC1101_MDMCFG1,  0x22);
    write_reg(d, CC1101_MDMCFG0,  0xF8);

    write_reg(d, CC1101_DEVIATN,  0x35);  // ~47 kHz
    write_reg(d, CC1101_MCSM0,    0x18);  // Autocal bei TX/RX

    write_reg(d, CC1101_FREND0,   0x11);
    write_reg(d, CC1101_FSCAL3,   0xE9);
    write_reg(d, CC1101_FSCAL2,   0x2A);
    write_reg(d, CC1101_FSCAL1,   0x00);
    write_reg(d, CC1101_FSCAL0,   0x1F);
    write_reg(d, CC1101_TEST2,    0x81);
    write_reg(d, CC1101_TEST1,    0x35);
    write_reg(d, CC1101_TEST0,    0x09);

    // RX-Leistung ist irrelevant; PATABLE wird nur für TX genutzt. Setzen wir trotzdem „sauber“:
    uint8_t pa = 0xC5;
    write_burst(d, CC1101_PATABLE, &pa, 1);
}

void CC1101_EnterRx(cc1101_t *d)
{
    strobe(d, CC1101_SIDLE);
    strobe(d, CC1101_SFRX);  // sicherstellen, dass RX FIFO leer ist
    strobe(d, CC1101_SRX);
}

// RSSI-Rohwert -> dBm (433 MHz Offset ~74 dB)
static int8_t rssi_to_dbm(uint8_t rssi_raw)
{
    int16_t v = (rssi_raw >= 128) ? (int16_t)rssi_raw - 256 : (int16_t)rssi_raw;
    return (int8_t)(v / 2 - 74);
}

HAL_StatusTypeDef CC1101_TryReadPacket(cc1101_t *d,
                                       uint8_t *payload, uint8_t *len, uint8_t maxlen,
                                       int8_t *rssi_dbm, uint8_t *lqi, uint8_t *crc_ok)
{
    if (!payload || !len) return HAL_ERROR;

    uint8_t rxbytes = CC1101_ReadStatus(d, CC1101_RXBYTES) & 0x7F;
    if (rxbytes == 0) return HAL_BUSY; // nichts da

    // 1) Erst die Längenangabe lesen
    uint8_t addr = CC1101_RXFIFO | CC1101_READ_BURST; // 0xFF
    uint8_t pkt_len = 0;

    cs_low(d); HAL_Delay(1);
    HAL_SPI_Transmit(d->hspi, &addr, 1, 20);
    HAL_SPI_Receive (d->hspi, &pkt_len, 1, 20);
    cs_high(d);

    if (pkt_len == 0 || pkt_len > 61 || pkt_len > maxlen) {
        // ungültig -> FIFO flush, in RX bleiben
        strobe(d, CC1101_SIDLE);
        strobe(d, CC1101_SFRX);
        strobe(d, CC1101_SRX);
        return HAL_ERROR;
    }

    // warten bis genug Bytes im FIFO: payload + 2 Status (RSSI, LQI/CRC)
    uint32_t tries = 0;
    while (((CC1101_ReadStatus(d, CC1101_RXBYTES) & 0x7F) < (uint8_t)(pkt_len + 2)) && tries++ < 2000) {
        // kurze Busy-Wait (ein paar 100 µs insgesamt)
    }

    // 2) Rest (payload + 2 status) lesen
    uint8_t tmp[64];
    cs_low(d); HAL_Delay(1);
    HAL_SPI_Transmit(d->hspi, &addr, 1, 20);
    HAL_SPI_Receive (d->hspi, tmp, pkt_len + 2, 200);
    cs_high(d);

    // Payload kopieren
    for (uint8_t i = 0; i < pkt_len; ++i) payload[i] = tmp[i];
    *len = pkt_len;

    // Status am Ende
    if (rssi_dbm) *rssi_dbm = rssi_to_dbm(tmp[pkt_len + 0]);
    if (lqi)      *lqi      = (uint8_t)(tmp[pkt_len + 1] & 0x7F);
    if (crc_ok)   *crc_ok   = (uint8_t)((tmp[pkt_len + 1] & 0x80) ? 1 : 0);

    return HAL_OK;
}
