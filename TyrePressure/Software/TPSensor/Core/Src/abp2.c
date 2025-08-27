#include "abp2.h"

static const uint8_t ABP2_CMD[3] = {0xAA, 0x00, 0x00};

static void read_data(abp2_t *dev, uint8_t *buf, uint8_t len)
{
    HAL_I2C_Master_Transmit(dev->hi2c, dev->addr8, (uint8_t*)ABP2_CMD, sizeof(ABP2_CMD), 50);
    HAL_Delay(10); // wait for measurement to be completed
    HAL_I2C_Master_Receive(dev->hi2c, dev->addr8, buf, len, 50);
}

//=======================================Parse Data======================================

static uint8_t parse_status(uint8_t *buf, abp2_status *status)
{
    uint8_t status_raw = buf[0];
    status->saturated = status_raw & 0x01;
    status->memory_error = status_raw & 0x04;
    status->busy = status_raw & 0x20;
    status->power = status_raw & 0x40;
    return status_raw;
}

static uint32_t parse_pressure(uint8_t *buf, float *pressure, float press_min, float press_max)
{
	//parse from read buffer
    uint32_t press_counts = ((uint32_t)buf[3]) |
                            ((uint32_t)buf[2] << 8) |
                            ((uint32_t)buf[1] << 16);

    //convert to temperature according to calculation from data sheet
    *pressure = (( (float)press_counts - ABP2_PRESS_MIN ) * (press_max - press_min)) / (ABP2_PRESS_MAX - ABP2_PRESS_MIN) + press_min;
    return press_counts;
}

static uint32_t parse_temp(uint8_t *buf, float *temp)
{
	//parse from read buffer
    uint32_t temp_counts = ((uint32_t)buf[6]) |
                           ((uint32_t)buf[5] << 8) |
                           ((uint32_t)buf[4] << 16);

    //convert to temperature according to calculation from data sheet
    *temp = (( (float)temp_counts * (ABP2_TEMP_MAX - ABP2_TEMP_MIN)) / ABP2_FULL_SCALE ) + ABP2_TEMP_MIN;
    return temp_counts;
}

//=======================================Public functions================================
HAL_StatusTypeDef ABP2_Init(abp2_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr7)
{
    if (!dev || !hi2c) return HAL_ERROR;
    dev->hi2c  = hi2c;
    dev->addr8 = ((uint16_t)addr7) << 1; // convert ABP2 7Bit address to 8Bit HAL
    // Ping device
    return HAL_I2C_IsDeviceReady(dev->hi2c, dev->addr8, 3, 5);
}

abp2_status ABP2_ReadPressure(abp2_t *dev, float *pressure)
{
	uint8_t buffer[4] = {0};
	abp2_status status;

	read_data(dev, buffer, 4);
	parse_status(buffer, &status);
	parse_pressure(buffer, pressure, ABP2_TP_MIN, ABP2_TP_MAX);
	return status;
}

abp2_status ABP2_ReadAll(abp2_t *dev, float *pressure, float *temperature, uint8_t *status_raw)
{
	uint8_t buffer[7] = {0};
	abp2_status status;

	read_data(dev, buffer, 7);
	status_raw = buffer[0];
	parse_status(buffer, &status);
	parse_pressure(buffer, pressure, ABP2_TP_MIN, ABP2_TP_MAX);
	parse_temp(buffer, temperature);
	return status;
}
