int16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
int16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
int16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
int16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
a0 = a0 >= 0 ? a0 : -a0;
a1 = a1 >= 0 ? a1 : -a1;
a2 = a2 >= 0 ? a2 : -a2;
a3 = a3 >= 0 ? a3 : -a3;
uint64_t c0 = (uint64_t)(a0)<<0;
uint64_t c1 = (uint64_t)(a1)<<16;
uint64_t c2 = (uint64_t)(a2)<<32;
uint64_t c3 = (uint64_t)(a3)<<48;
return c0 | c1 | c2 | c3;
