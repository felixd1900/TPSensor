#ifndef INC_TYPE_CONV_H_
#define INC_TYPE_CONV_H_

void Float_to_Bytearray(uint8_t p[4], float f);
void UINT32_to_Bytearray(uint8_t out[4], uint32_t u);
uint32_t Float_to_UINT32(float f);

uint32_t Bytearray_to_UINT32(const uint8_t p[4]);
float UINT32_to_Float(uint32_t u);
float Bytearray_to_Float(const uint8_t p[4]);

uint32_t EE_ADDR(uint32_t off);
uint8_t  EE_ReadU8 (uint32_t off);
uint32_t EE_ReadU32(uint32_t off);
float EE_ReadF32(uint32_t off);
void EE_WriteU8(uint32_t off, uint8_t v);
void EE_WriteU32(uint32_t off, uint32_t v);
void EE_WriteF32(uint32_t off, float f);

#endif /* INC_TYPE_CONV_H_ */
