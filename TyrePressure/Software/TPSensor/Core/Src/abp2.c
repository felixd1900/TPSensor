#include "abp2.h"

static const uint8_t ABP2_CMD[3] = {0xAA, 0x00, 0x00};

HAL_StatusTypeDef ABP2_Init(abp2_t *dev, I2C_HandleTypeDef *hi2c,
                            uint8_t addr7, float pmin, float pmax)
{
    if (!dev || !hi2c) return HAL_ERROR;
    dev->hi2c  = hi2c;
    dev->addr8 = ((uint16_t)addr7) << 1; // HAL nutzt 8-bit Adresse
    dev->pmin  = pmin;
    dev->pmax  = pmax;

    // Einmal ping
    return HAL_I2C_IsDeviceReady(dev->hi2c, dev->addr8, 3, 5);
}

HAL_StatusTypeDef ABP2_RequestAndRead(const abp2_t *dev,
                                      uint8_t data[ABP2_RX_LEN],
                                      uint32_t delay_ms)
{
    if (!dev || !data) return HAL_ERROR;

    HAL_StatusTypeDef st;

    st = HAL_I2C_Master_Transmit(dev->hi2c, dev->addr8,
                                 (uint8_t*)ABP2_CMD, sizeof(ABP2_CMD), 50);
    if (st != HAL_OK) return st;

    HAL_Delay(delay_ms); // wie Arduino: delay(10)

    st = HAL_I2C_Master_Receive(dev->hi2c, dev->addr8, data, ABP2_RX_LEN, 50);
    return st;
}

void ABP2_Parse(const uint8_t raw[ABP2_RX_LEN], uint8_t *status,
                uint32_t *press_counts, uint32_t *temp_counts)
{
    if (status)       *status       = raw[0];
    if (press_counts) *press_counts = ((uint32_t)raw[3]) |
                                      ((uint32_t)raw[2] << 8) |
                                      ((uint32_t)raw[1] << 16);
    if (temp_counts)  *temp_counts  = ((uint32_t)raw[6]) |
                                      ((uint32_t)raw[5] << 8) |
                                      ((uint32_t)raw[4] << 16);
}

void ABP2_Convert(const abp2_t *dev, uint32_t press_counts, uint32_t temp_counts,
                  float *pressure, float *temperature_c, float *percentage)
{
    if (percentage)    *percentage    = ((float)press_counts / ABP2_FULL_SCALE) * 100.0f;
    if (pressure)      *pressure      = (( (float)press_counts - ABP2_OUT_MIN ) *
                                        (dev->pmax - dev->pmin)) /
                                        (ABP2_OUT_MAX - ABP2_OUT_MIN) + dev->pmin;
    if (temperature_c) *temperature_c = ( (float)temp_counts * 200.0f / ABP2_FULL_SCALE ) - 50.0f;
}

HAL_StatusTypeDef ABP2_ReadAll(const abp2_t *dev,
                               float *pressure, float *temperature_c, float *percentage,
                               uint8_t *status, uint32_t *press_counts, uint32_t *temp_counts)
{
    uint8_t buf[ABP2_RX_LEN];
    HAL_StatusTypeDef st = ABP2_RequestAndRead(dev, buf, 10); // 10 ms wie Arduino
    if (st != HAL_OK) return st;

    ABP2_Parse(buf, status, press_counts, temp_counts);
    ABP2_Convert(dev,
                 press_counts ? *press_counts : 0,
                 temp_counts  ? *temp_counts  : 0,
                 pressure, temperature_c, percentage);
    return HAL_OK;
}
