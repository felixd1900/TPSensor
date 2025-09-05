//============================== DATA CONVERSION =================================

#include "stm32l0xx_hal.h"

void Float_to_Bytearray(uint8_t p[4], float f)
{
    uint32_t u = Float_to_UINT32(f);
    UINT32_to_Bytearray(p, u);
}

void UINT32_to_Bytearray(uint8_t out[4], uint32_t u)
{
    out[0] = (uint8_t)(u >> 24);
    out[1] = (uint8_t)(u >> 16);
    out[2] = (uint8_t)(u >>  8);
    out[3] = (uint8_t)(u >>  0);
}

uint32_t Float_to_UINT32(float f)
{
    uint32_t u;
    memcpy(&u, &f, sizeof u);   // Bitwise
    return u;
}

uint32_t Bytearray_to_UINT32(const uint8_t p[4])
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |
           ((uint32_t)p[3] <<  0);
}

float UINT32_to_Float(uint32_t u)
{
    float f;
    memcpy(&f, &u, sizeof f);   // bitwise
    return f;
}

float Bytearray_to_Float(const uint8_t p[4])
{
    uint32_t u = Bytearray_to_UINT32(p);
    return UINT32_to_Float(u);
}



//=============================== EEPROM ==================================

uint32_t EE_ADDR(uint32_t off)
{
	return (0x08080000UL + off);
}

// --- READ ---
uint8_t  EE_ReadU8 (uint32_t off)
{
	return *(__IO uint8_t*) EE_ADDR(off);
}

static uint32_t EE_ReadU32(uint32_t off)
{
	return *(__IO uint32_t*)EE_ADDR(off);
}

float EE_ReadF32(uint32_t off)
{
    uint32_t u = EE_ReadU32(off);
    float f;
    memcpy(&f, &u, sizeof f);     // Bitwise
    return f;
}

// --- WRITE ---
void EE_WriteU8(uint32_t off, uint8_t v)
{
    if (EE_ReadU8(off) == v) return;
    HAL_FLASHEx_DATAEEPROM_Unlock();
    HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE, EE_ADDR(off), v);
    HAL_FLASHEx_DATAEEPROM_Lock();
}

void EE_WriteU32(uint32_t off, uint32_t v)
{
    if (EE_ReadU32(off) == v) return;
    HAL_FLASHEx_DATAEEPROM_Unlock();
    HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, EE_ADDR(off), v);
    HAL_FLASHEx_DATAEEPROM_Lock();
}

void EE_WriteF32(uint32_t off, float f)
{
    uint32_t u;
    memcpy(&u, &f, sizeof u);     // Float-Bits unverändert
    EE_WriteU32(off, u);
}
