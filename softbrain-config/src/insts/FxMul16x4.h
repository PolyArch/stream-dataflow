int16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
int16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
int16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
int16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
int16_t b0 = (ops[1]&0x000000000000FFFF)>>0;
int16_t b1 = (ops[1]&0x00000000FFFF0000)>>16;
int16_t b2 = (ops[1]&0x0000FFFF00000000)>>32;
int16_t b3 = (ops[1]&0xFFFF000000000000)>>48;

int32_t im0 = ((int32_t)a0 * (int32_t)b0) >> 11;
int16_t m0 = im0 > (int16_t)0x7FFF ? (int16_t)0x7FFF : (im0 < (int32_t)0xFFFF8001 ? (int32_t)0xFFFF8001 : im0);

int32_t im1 = ((int32_t)a1 * (int32_t)b1) >> 11;
int16_t m1 = im1 > 0x7FFF ? 0x7FFF : (im1 < (int32_t)(int16_t)0xFFFF8001 ? (int32_t)(int16_t)0xFFFF8001 : im1);

int32_t im2 = ((int32_t)a2 * (int32_t)b2) >> 11;
int16_t m2 = im2 > (int16_t)0x7FFF ? (int16_t)0x7FFF : (im2 < (int32_t)0xFFFF8001 ? (int32_t)0xFFFF8001 : im2);

int32_t im3 = ((int32_t)a3 * (int32_t)b3) >> 11;
int16_t m3 = im3 > (int16_t)0x7FFF ? (int16_t)0x7FFF : (im3 < (int32_t)0xFFFF8001 ? (int32_t)0xFFFF8001 : im3);

uint64_t c0 = ((uint64_t)(m0)<<0)&0x000000000000FFFF;
uint64_t c1 = ((uint64_t)(m1)<<16)&0x00000000FFFF0000;
uint64_t c2 = ((uint64_t)(m2)<<32)&0x0000FFFF00000000;
uint64_t c3 = ((uint64_t)(m3)<<48)&0xFFFF000000000000;

return c0 | c1 | c2 | c3;
