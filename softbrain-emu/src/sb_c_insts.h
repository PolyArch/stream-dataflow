#ifndef _SB_EMU_INSTS
#define _SB_EMU_INSTS
#include <array>
#include <cstring>
#include <iostream>
#include "sb_init.h"
inline uint64_t Xor(std::array<uint64_t,2> ops) {
	return ops[0] ^ ops[1]; 
}

inline uint64_t FMul64(std::array<uint64_t,2> ops) {
	double a = as_double(ops[0]);
	double b = as_double(ops[1]);
	double c = a*b;
	return as_uint64(c); 
}

inline uint64_t RedSMax16x4(std::array<uint64_t,2> ops) {
	int16_t r0 = (ops[0]&0x000000000000FFFF)>>0;
	int16_t r1 = (ops[0]&0x00000000FFFF0000)>>16;
	int16_t r2 = (ops[0]&0x0000FFFF00000000)>>32;
	int16_t r3 = (ops[0]&0xFFFF000000000000)>>48;
	
	int16_t x = r0;
	if(r1 > x) {x=r1;}
	if(r2 > x) {x=r2;}
	if(r3 > x) {x=r3;}
	
	if(ops.size() > 1) { //additional op is acc
	  int16_t b = (int16_t)ops[1];
	  if(b > x) {x=b;}
	} 
	return (uint64_t)x;
	
}

inline uint64_t Sub64(std::array<uint64_t,2> ops) {
	return ops[0] - ops[1]; 
}

inline uint64_t FxMul16x4(std::array<uint64_t,2> ops) {
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
}

inline uint64_t FxTanh16x4(std::array<uint64_t,2> ops) {
	int16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
	int16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
	int16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
	int16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
	
	if ((ops.size() > 1) && (ops[1] == 0))
	    return ops[0];
	
	double d0 = FIX_TO_DOUBLE(a0);
	double d1 = FIX_TO_DOUBLE(a1);
	double d2 = FIX_TO_DOUBLE(a2);
	double d3 = FIX_TO_DOUBLE(a3);
	
	d0 = tanh(d0);
	d1 = tanh(d1);
	d2 = tanh(d2);
	d3 = tanh(d3);
	
	int16_t b0 = DOUBLE_TO_FIX(d0);
	int16_t b1 = DOUBLE_TO_FIX(d1);
	int16_t b2 = DOUBLE_TO_FIX(d2);
	int16_t b3 = DOUBLE_TO_FIX(d3);
	
	uint64_t c0 = ((uint64_t)(b0)<<0)&0x000000000000FFFF;
	uint64_t c1 = ((uint64_t)(b1)<<16)&0x00000000FFFF0000;
	uint64_t c2 = ((uint64_t)(b2)<<32)&0x0000FFFF00000000;
	uint64_t c3 = ((uint64_t)(b3)<<48)&0xFFFF000000000000;
	
	return c0 | c1 | c2 | c3;
}

inline uint64_t FxSig16x4(std::array<uint64_t,2> ops) {
	int16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
	int16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
	int16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
	int16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
	
	if ((ops.size() > 1) && (ops[1] == 0))
	    return ops[0];
	
	double d0 = FIX_TO_DOUBLE(a0);
	double d1 = FIX_TO_DOUBLE(a1);
	double d2 = FIX_TO_DOUBLE(a2);
	double d3 = FIX_TO_DOUBLE(a3);
	
	d0 = 1 / (1 + exp(-d0));
	d1 = 1 / (1 + exp(-d1));
	d2 = 1 / (1 + exp(-d2));
	d3 = 1 / (1 + exp(-d3));
	
	int16_t b0 = DOUBLE_TO_FIX(d0);
	int16_t b1 = DOUBLE_TO_FIX(d1);
	int16_t b2 = DOUBLE_TO_FIX(d2);
	int16_t b3 = DOUBLE_TO_FIX(d3);
	
	uint64_t c0 = ((uint64_t)(b0)<<0)&0x000000000000FFFF;
	uint64_t c1 = ((uint64_t)(b1)<<16)&0x00000000FFFF0000;
	uint64_t c2 = ((uint64_t)(b2)<<32)&0x0000FFFF00000000;
	uint64_t c3 = ((uint64_t)(b3)<<48)&0xFFFF000000000000;
	
	return c0 | c1 | c2 | c3;
}

inline uint64_t FxRed16x4(std::array<uint64_t,2> ops) {
	uint16_t r0 = (ops[0]&0x000000000000FFFF)>>0;
	uint16_t r1 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t r2 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t r3 = (ops[0]&0xFFFF000000000000)>>48;
	
	uint16_t sum;
	
	uint16_t sum0 = r0 + r1;
	if (!((r0 ^ r1) & 0x8000) && ((r0 ^ sum0) & 0x8000) && !(r0 & 0x8000))
	  sum0 = 0x7FFF;
	else if (!((r0 ^ r1) & 0x8000) && ((r0 ^ sum0) & 0x8000) && (r0 & 0x8000))
	  sum0 = 0x8001;
	
	uint16_t sum1 = r2 + r3;
	if (!((r2 ^ r3) & 0x8000) && ((r2 ^ sum1) & 0x8000) && !(r2 & 0x8000))
	  sum1 = 0x7FFF;
	else if (!((r2 ^ r3) & 0x8000) && ((r2 ^ sum1) & 0x8000) && (r2 & 0x8000))
	  sum1 = 0x8001;
	
	uint16_t sum2 = sum0 + sum1;
	if (!((sum0 ^ sum1) & 0x8000) && ((sum0 ^ sum2) & 0x8000) && !(sum0 & 0x8000))
	  sum2 = 0x7FFF;
	else if (!((sum0 ^ sum1) & 0x8000) && ((sum0 ^ sum2) & 0x8000) && (sum0 & 0x8000))
	  sum2 = 0x8001;
	
	if(ops.size() > 1) { //additional op is acc
	  sum = sum2 + (uint16_t)ops[1];
	  if (!((sum2 ^ (uint16_t)ops[1]) & 0x8000) && ((sum2 ^ sum) & 0x8000) && !(sum2 & 0x8000))
	    sum = 0x7FFF;
	  else if (!((sum2 ^ (uint16_t)ops[1]) & 0x8000) && ((sum2 ^ sum) & 0x8000) && (sum2 & 0x8000))
	    sum = 0x8001;
	} else {
	  sum = sum2;
	}
	
	return sum;
}

inline uint64_t LShf64(std::array<uint64_t,2> ops) {
	if(ops[1]==64) {
	  return 0;
	}
	return ops[0] << ops[1];
}

inline uint64_t FxAdd32x2(std::array<uint64_t,2> ops) {
	uint32_t a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
	uint32_t a1 = (ops[0]&0xFFFFFFFF00000000)>>32;
	uint32_t b0 = (ops[1]&0x00000000FFFFFFFF)>>0;
	uint32_t b1 = (ops[1]&0xFFFFFFFF00000000)>>32;
	
	uint32_t sum;
	
	sum = a0 + b0;
	if (!((a0 ^ b0) & 0x80000000) && ((a0 ^ sum) & 0x80000000) && !(a0 & 0x80000000))
	  a0 = 0x7FFFFFFF;
	else if (!((a0 ^ b0) & 0x80000000) && ((a0 ^ sum) & 0x80000000) && (a0 & 0x80000000))
	  a0 = 0x80000001;
	else
	  a0 = sum;
	
	sum = a1 + b1;
	if (!((a1 ^ b1) & 0x80000000) && ((a1 ^ sum) & 0x80000000) && !(a1 & 0x80000000))
	  a1 = 0x7FFFFFFF;
	else if (!((a1 ^ b1) & 0x80000000) && ((a1 ^ sum) & 0x80000000) && (a1 & 0x80000000))
	  a1 = 0x80000001;
	else
	  a1 = sum;
	
	uint64_t c0 = ((uint64_t)(a0))<<0;
	uint64_t c1 = ((uint64_t)(a1))<<32;
	return c0 | c1;
}

inline uint64_t FxMul32x2(std::array<uint64_t,2> ops) {
	int32_t a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
	int32_t a1 = (ops[0]&0xFFFFFFFF00000000)>>32;
	int32_t b0 = (ops[1]&0x00000000FFFFFFFF)>>0;
	int32_t b1 = (ops[1]&0xFFFFFFFF00000000)>>32;
	
	int64_t im0 = ((int64_t)a0 * (int64_t)b0) >> 14; // 14 fractional bits 
	int32_t m0 = im0 > (int64_t)0x000000007FFFFFFF ? (int32_t)0x7FFFFFFF : (im0 < (int64_t)0xFFFFFFFF80000001 ? (int32_t)0x80000001 : im0);
	
	int64_t im1 = ((int64_t)a1 * (int64_t)b1) >> 14;
	int32_t m1 = im1 > (int64_t)0x000000007FFFFFFF ? (int32_t)0x7FFFFFFF : (im1 < (int64_t)0xFFFFFFFF80000001 ? (int32_t)0x80000001 : im1);
	
	uint64_t c0 = ((uint64_t)(m0)<<0)&0x00000000FFFFFFFF;
	uint64_t c1 = ((uint64_t)(m1)<<32)&0xFFFFFFFF00000000;
	
	return c0 | c1;
}

inline uint64_t SMax16x4(std::array<uint64_t,2> ops) {
	int16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
	int16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
	int16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
	int16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
	int16_t b0 = (ops[1]&0x000000000000FFFF)>>0;
	int16_t b1 = (ops[1]&0x00000000FFFF0000)>>16;
	int16_t b2 = (ops[1]&0x0000FFFF00000000)>>32;
	int16_t b3 = (ops[1]&0xFFFF000000000000)>>48;
	int16_t t0 = a0 >= b0 ? a0 : b0;
	int16_t t1 = a1 >= b1 ? a1 : b1;
	int16_t t2 = a2 >= b2 ? a2 : b2;
	int16_t t3 = a3 >= b3 ? a3 : b3;
	uint64_t c0 = (uint64_t)(t0)<<0;
	uint64_t c1 = (uint64_t)(t1)<<16;
	uint64_t c2 = (uint64_t)(t2)<<32;
	uint64_t c3 = (uint64_t)(t3)<<48;
	return c0 | c1 | c2 | c3;
}

inline uint64_t RedSMin16x4(std::array<uint64_t,2> ops) {
	int16_t r0 = (ops[0]&0x000000000000FFFF)>>0;
	int16_t r1 = (ops[0]&0x00000000FFFF0000)>>16;
	int16_t r2 = (ops[0]&0x0000FFFF00000000)>>32;
	int16_t r3 = (ops[0]&0xFFFF000000000000)>>48;
	
	int16_t x = r0;
	if(r1 < x) {x=r1;}
	if(r2 < x) {x=r2;}
	if(r3 < x) {x=r3;}
	
	if(ops.size() > 1) { //additional op is acc
	  int16_t b = (int16_t)ops[1];
	  if(b < x) {x=b;}
	} 
	return (uint64_t)x;
	
}

inline uint64_t FxRelu16x4(std::array<uint64_t,2> ops) {
	uint16_t i1 = (ops[0]&0x000000000000FFFF)>>0;
	uint16_t i2 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t i3 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t i4 = (ops[0]&0xFFFF000000000000)>>48;
	
	if ((ops.size() > 1) && (ops[1] == 0))
	    return ops[0];
	
	if (i1 & 0x8000)
	  i1 = 0;
	
	if (i2 & 0x8000)
	  i2 = 0;
	
	if (i3 & 0x8000)
	  i3 = 0;
	
	if (i4 & 0x8000)
	  i4 = 0;
	
	uint64_t o1 = (uint64_t)(i1)<<0;
	uint64_t o2 = (uint64_t)(i2)<<16;
	uint64_t o3 = (uint64_t)(i3)<<32;
	uint64_t o4 = (uint64_t)(i4)<<48;
	
	return o1 | o2 | o3 | o4;
}

inline uint64_t HAdd16x4(std::array<uint64_t,2> ops) {
	uint16_t a0 = (ops[0]&0xFFFF000000000000)>>48;
	uint16_t a1 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t a2 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t a3 = (ops[0]&0x000000000000FFFF)>>0;
	
	uint16_t b0 = (ops[1]&0xFFFF000000000000)>>48;
	//uint16_t b1 = (ops[1]&0x0000FFFF00000000)>>32;
	//uint16_t b2 = (ops[1]&0x00000000FFFF0000)>>16;
	//uint16_t b3 = (ops[1]&0x000000000000FFFF)>>0;
	
	uint64_t c0 = (uint64_t)(a0+a1)<<48;
	uint64_t c1 = (uint64_t)(a1+a2)<<32;
	uint64_t c2 = (uint64_t)(a2+a3)<<16;
	uint64_t c3 = (uint64_t)(a3+b0)<<0;
	
	return c0 | c1 | c2 | c3;
	//return (uint64_t) _mm_adds_pu16((__m64)ops[0], (__m64)ops[1]);
}

inline uint64_t ICmpEQ(std::array<uint64_t,2> ops) {
	return ops[0] == ops[1]; 
}

inline uint64_t Red32x2(std::array<uint64_t,2> ops) {
	uint32_t r0 = (ops[0]&0x00000000FFFFFFFF)>>0;
	uint32_t r1 = (ops[0]&0xFFFFFFFF00000000)>>32;
	
	if(ops.size() > 1) { //additional op is acc
	  return (r0+r1+((uint32_t)ops[1]));
	} 
	return (r0+r1);
	
}

inline uint64_t FxRed32x2(std::array<uint64_t,2> ops) {
	uint32_t r0 = (ops[0]&0x00000000FFFFFFFF)>>0;
	uint32_t r1 = (ops[0]&0xFFFFFFFF00000000)>>32;
	
	uint32_t sum;
	
	uint32_t sum0 = r0 + r1;
	if (!((r0 ^ r1) & 0x80000000) && ((r0 ^ sum0) & 0x80000000) && !(r0 & 0x80000000))
	  sum0 = 0x7FFFFFFF;
	else if (!((r0 ^ r1) & 0x80000000) && ((r0 ^ sum0) & 0x80000000) && (r0 & 0x80000000))
	  sum0 = 0x80000001;
	
	if(ops.size() > 1) { //additional op is acc
	  sum = sum0 + (uint32_t)ops[1];
	  if (!((sum0 ^ (uint32_t)ops[1]) & 0x80000000) && ((sum0 ^ sum) & 0x80000000) && !(sum0 & 0x80000000))
	    sum = 0x7FFFFFFF;
	  else if (!((sum0 ^ (uint32_t)ops[1]) & 0x80000000) && ((sum0 ^ sum) & 0x80000000) && (sum0 & 0x80000000))
	    sum = 0x80000001;
	} else {
	  sum = sum0;
	}
	
	return sum;
}

inline uint64_t Select(std::array<uint64_t,2> ops) {
	return ops[2]==0 ? ops[0] : ops[1];
}

inline uint64_t FMul32x2(std::array<uint64_t,2> ops) {
	uint32_t t_a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
	uint32_t t_a1 = (ops[0]&0xFFFFFFFF00000000)>>32;
	uint32_t t_b0 = (ops[1]&0x00000000FFFFFFFF)>>0;
	uint32_t t_b1 = (ops[1]&0xFFFFFFFF00000000)>>32;
	
	float a0=as_float(t_a0);
	float a1=as_float(t_a1);
	float b0=as_float(t_b0);
	float b1=as_float(t_b1);
	
	a0*=b0;
	a1*=b1;
	
	uint64_t c0 = (uint64_t)(as_uint32(a0))<<0;
	uint64_t c1 = (uint64_t)(as_uint32(a1))<<32;
	return c0 | c1;
	
	//return (uint64_t) _mm_mullo_pi32((__m64)ops[0], (__m64)ops[1]);  -- mullo_pi32 doesnt exisit in mm intrinsics
}

inline uint64_t FAdd64(std::array<uint64_t,2> ops) {
	double a = as_double(ops[0]);
	double b = as_double(ops[1]);
	double c = a+b;
	return as_uint64(c); 
}

inline uint64_t Or(std::array<uint64_t,2> ops) {
	return ops[0] | ops[1]; 
}

inline uint64_t RShf4_16x4(std::array<uint64_t,2> ops) {
	uint16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
	uint16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
	
	uint64_t b = ops[1];
	if(ops.size()==1) {
	  b = 4;
	}
	uint64_t c0 = (uint64_t)(a0>>b)<<0;
	uint64_t c1 = (uint64_t)(a1>>b)<<16;
	uint64_t c2 = (uint64_t)(a2>>b)<<32;
	uint64_t c3 = (uint64_t)(a3>>b)<<48;
	return c0 | c1 | c2 | c3;
	//return (uint64_t) _mm_adds_pu16((__m64)ops[0], (__m64)ops[1]);
}

inline uint64_t Max16x4(std::array<uint64_t,2> ops) {
	uint16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
	uint16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
	uint16_t b0 = (ops[1]&0x000000000000FFFF)>>0;
	uint16_t b1 = (ops[1]&0x00000000FFFF0000)>>16;
	uint16_t b2 = (ops[1]&0x0000FFFF00000000)>>32;
	uint16_t b3 = (ops[1]&0xFFFF000000000000)>>48;
	uint16_t t0 = a0 >= b0 ? a0 : b0;
	uint16_t t1 = a1 >= b1 ? a1 : b1;
	uint16_t t2 = a2 >= b2 ? a2 : b2;
	uint16_t t3 = a3 >= b3 ? a3 : b3;
	uint64_t c0 = (uint64_t)(t0)<<0;
	uint64_t c1 = (uint64_t)(t1)<<16;
	uint64_t c2 = (uint64_t)(t2)<<32;
	uint64_t c3 = (uint64_t)(t3)<<48;
	return c0 | c1 | c2 | c3;
}

inline uint64_t RShf64(std::array<uint64_t,2> ops) {
	if(ops[1]==64) {
	  return 0;
	}
	return ops[0] >> ops[1];
}

inline uint64_t RShf2_16x4(std::array<uint64_t,2> ops) {
	uint16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
	uint16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
	
	uint64_t b = ops[1];
	if(ops.size()==1) {
	  b = 2;
	}
	uint64_t c0 = (uint64_t)(a0>>b)<<0;
	uint64_t c1 = (uint64_t)(a1>>b)<<16;
	uint64_t c2 = (uint64_t)(a2>>b)<<32;
	uint64_t c3 = (uint64_t)(a3>>b)<<48;
	return c0 | c1 | c2 | c3;
	//return (uint64_t) _mm_adds_pu16((__m64)ops[0], (__m64)ops[1]);
}

inline uint64_t SMin16x4(std::array<uint64_t,2> ops) {
	int16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
	int16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
	int16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
	int16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
	int16_t b0 = (ops[1]&0x000000000000FFFF)>>0;
	int16_t b1 = (ops[1]&0x00000000FFFF0000)>>16;
	int16_t b2 = (ops[1]&0x0000FFFF00000000)>>32;
	int16_t b3 = (ops[1]&0xFFFF000000000000)>>48;
	int16_t t0 = a0 <= b0 ? a0 : b0;
	int16_t t1 = a1 <= b1 ? a1 : b1;
	int16_t t2 = a2 <= b2 ? a2 : b2;
	int16_t t3 = a3 <= b3 ? a3 : b3;
	uint64_t c0 = (uint64_t)(t0)<<0;
	uint64_t c1 = (uint64_t)(t1)<<16;
	uint64_t c2 = (uint64_t)(t2)<<32;
	uint64_t c3 = (uint64_t)(t3)<<48;
	return c0 | c1 | c2 | c3;
}

inline uint64_t Acc16x4(std::array<uint64_t,2> ops) {
	uint16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
	uint16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
	uint16_t b0 = (accum&0x000000000000FFFF)>>0;
	uint16_t b1 = (accum&0x00000000FFFF0000)>>16;
	uint16_t b2 = (accum&0x0000FFFF00000000)>>32;
	uint16_t b3 = (accum&0xFFFF000000000000)>>48;
	a0+=b0;
	a1+=b1;
	a2+=b2;
	a3+=b3;
	uint64_t c0 = (uint64_t)(a0)<<0;
	uint64_t c1 = (uint64_t)(a1)<<16;
	uint64_t c2 = (uint64_t)(a2)<<32;
	uint64_t c3 = (uint64_t)(a3)<<48;
	
	accum = c0 | c1 | c2 | c3;
	
	uint64_t ret = accum;
	
	if(ops[1]) {
	  accum=0;
	}
	
	return ret; 
}

inline uint64_t FxAdd16x4(std::array<uint64_t,2> ops) {
	uint16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
	uint16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
	uint16_t b0 = (ops[1]&0x000000000000FFFF)>>0;
	uint16_t b1 = (ops[1]&0x00000000FFFF0000)>>16;
	uint16_t b2 = (ops[1]&0x0000FFFF00000000)>>32;
	uint16_t b3 = (ops[1]&0xFFFF000000000000)>>48;
	
	uint16_t sum = a0 + b0;
	
	if (!((a0 ^ b0) & 0x8000) && ((a0 ^ sum) & 0x8000) && !(a0 & 0x8000))
	  a0 = 0x7FFF;
	else if (!((a0 ^ b0) & 0x8000) && ((a0 ^ sum) & 0x8000) && (a0 & 0x8000))
	  a0 = 0x8001;
	else
	  a0 = sum;
	
	sum = a1 + b1;
	
	if (!((a1 ^ b1) & 0x8000) && ((a1 ^ sum) & 0x8000) && !(a1 & 0x8000))
	  a1 = 0x7FFF;
	else if (!((a1 ^ b1) & 0x8000) && ((a1 ^ sum) & 0x8000) && (a1 & 0x8000))
	  a1 = 0x8001;
	else
	  a1 = sum;
	
	sum = a2 + b2;
	
	if (!((a2 ^ b2) & 0x8000) && ((a2 ^ sum) & 0x8000) && !(a2 & 0x8000))
	  a2 = 0x7FFF;
	else if (!((a2 ^ b2) & 0x8000) && ((a2 ^ sum) & 0x8000) && (a2 & 0x8000))
	  a2 = 0x8001;
	else
	  a2 = sum;
	
	sum = a3 + b3;
	
	if (!((a3 ^ b3) & 0x8000) && ((a3 ^ sum) & 0x8000) && !(a3 & 0x8000))
	  a3 = 0x7FFF;
	else if (!((a3 ^ b3) & 0x8000) && ((a3 ^ sum) & 0x8000) && (a3 & 0x8000))
	  a3 = 0x8001;
	else
	  a3 = sum;
	
	uint64_t c0 = (uint64_t)(a0)<<0;
	uint64_t c1 = (uint64_t)(a1)<<16;
	uint64_t c2 = (uint64_t)(a2)<<32;
	uint64_t c3 = (uint64_t)(a3)<<48;
	return c0 | c1 | c2 | c3;
}

inline uint64_t Mul32x2(std::array<uint64_t,2> ops) {
	uint32_t a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
	uint32_t a1 = (ops[0]&0xFFFFFFFF00000000)>>32;
	uint32_t b0 = (ops[1]&0x00000000FFFFFFFF)>>0;
	uint32_t b1 = (ops[1]&0xFFFFFFFF00000000)>>32;
	
	a0*=b0;
	a1*=b1;
	
	uint64_t c0 = (uint64_t)(a0)<<0;
	uint64_t c1 = (uint64_t)(a1)<<32;
	return c0 | c1;
	
	//return (uint64_t) _mm_mullo_pi32((__m64)ops[0], (__m64)ops[1]);  -- mullo_pi32 doesnt exisit in mm intrinsics
}

inline uint64_t Add32x2(std::array<uint64_t,2> ops) {
	uint32_t a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
	uint32_t a1 = (ops[0]&0xFFFFFFFF00000000)>>32;
	uint32_t b0 = (ops[1]&0x00000000FFFFFFFF)>>0;
	uint32_t b1 = (ops[1]&0xFFFFFFFF00000000)>>32;
	a0+=b0;
	a1+=b1;
	uint64_t c0 = (uint64_t)(a0)<<0;
	uint64_t c1 = (uint64_t)(a1)<<32;
	return c0 | c1;
	//return (uint64_t) _mm_adds_pu16((__m64)ops[0], (__m64)ops[1]);
}

inline uint64_t RShf16x4(std::array<uint64_t,2> ops) {
	uint16_t a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
	uint16_t a1 = (ops[0]&0xFFFFFFFF00000000)>>32;
	
	uint64_t b = ops[1];
	if(ops.size()==1) {
	  b = 2;
	}
	uint64_t c0 = (uint64_t)(a0>>b)<<0;
	uint64_t c1 = (uint64_t)(a1>>b)<<32;
	return c0 | c1;
	//return (uint64_t) _mm_adds_pu16((__m64)ops[0], (__m64)ops[1]);
}

inline uint64_t Mul64(std::array<uint64_t,2> ops) {
	return ops[0] * ops[1]; 
}

inline uint64_t Abs16x4(std::array<uint64_t,2> ops) {
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
}

inline uint64_t Min16x4(std::array<uint64_t,2> ops) {
	uint16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
	uint16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
	uint16_t b0 = (ops[1]&0x000000000000FFFF)>>0;
	uint16_t b1 = (ops[1]&0x00000000FFFF0000)>>16;
	uint16_t b2 = (ops[1]&0x0000FFFF00000000)>>32;
	uint16_t b3 = (ops[1]&0xFFFF000000000000)>>48;
	uint16_t t0 = a0 <= b0 ? a0 : b0;
	uint16_t t1 = a1 <= b1 ? a1 : b1;
	uint16_t t2 = a2 <= b2 ? a2 : b2;
	uint16_t t3 = a3 <= b3 ? a3 : b3;
	uint64_t c0 = (uint64_t)(t0)<<0;
	uint64_t c1 = (uint64_t)(t1)<<16;
	uint64_t c2 = (uint64_t)(t2)<<32;
	uint64_t c3 = (uint64_t)(t3)<<48;
	return c0 | c1 | c2 | c3;
}

inline uint64_t RedMin16x4(std::array<uint64_t,2> ops) {
	uint16_t r0 = (ops[0]&0x000000000000FFFF)>>0;
	uint16_t r1 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t r2 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t r3 = (ops[0]&0xFFFF000000000000)>>48;
	
	uint16_t x = r0;
	if(r1 < x) {x=r1;}
	if(r2 < x) {x=r2;}
	if(r3 < x) {x=r3;}
	
	if(ops.size() > 1) { //additional op is acc
	  uint16_t b = (uint16_t)ops[1];
	  if(b < x) {x=b;}
	} 
	return x;
	
}

inline uint64_t RShf32x2(std::array<uint64_t,2> ops) {
	uint16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
	uint16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
	
	uint64_t b = ops[1];
	if(ops.size()==1) {
	  b = 2;
	}
	uint64_t c0 = (uint64_t)(a0>>b)<<0;
	uint64_t c1 = (uint64_t)(a1>>b)<<16;
	uint64_t c2 = (uint64_t)(a2>>b)<<32;
	uint64_t c3 = (uint64_t)(a3>>b)<<48;
	return c0 | c1 | c2 | c3;
	//return (uint64_t) _mm_adds_pu16((__m64)ops[0], (__m64)ops[1]);
}

inline uint64_t FRed32x2(std::array<uint64_t,2> ops) {
	uint32_t t_r0 = (ops[0]&0x00000000FFFFFFFF)>>0;
	uint32_t t_r1 = (ops[0]&0xFFFFFFFF00000000)>>32;
	
	float r0=as_float(t_r0);
	float r1=as_float(t_r1);
	
	float result;
	if(ops.size() > 1) { //additional op is acc
	  result = r0 + r1 + as_float((uint32_t)ops[1]);
	} else {
	  result = r0 + r1;
	}
	return (uint64_t)(as_uint32(result));
	
}

inline uint64_t FAdd32x2(std::array<uint64_t,2> ops) {
	uint32_t t_a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
	uint32_t t_a1 = (ops[0]&0xFFFFFFFF00000000)>>32;
	uint32_t t_b0 = (ops[1]&0x00000000FFFFFFFF)>>0;
	uint32_t t_b1 = (ops[1]&0xFFFFFFFF00000000)>>32;
	
	float a0=as_float(t_a0);
	float a1=as_float(t_a1);
	float b0=as_float(t_b0);
	float b1=as_float(t_b1);
	
	a0+=b0;
	a1+=b1;
	
	uint64_t c0 = (uint64_t)(as_uint32(a0))<<0;
	uint64_t c1 = (uint64_t)(as_uint32(a1))<<32;
	return c0 | c1;
	//return (uint64_t) _mm_adds_pu16((__m64)ops[0], (__m64)ops[1]);
}

inline uint64_t Acc64(std::array<uint64_t,2> ops) {
	accum+=ops[0];
	
	uint64_t ret = accum;
	
	if(ops[1]) {
	  accum=0;
	}
	
	return ret; 
}

inline uint64_t Red16x4(std::array<uint64_t,2> ops) {
	uint16_t r0 = (ops[0]&0x000000000000FFFF)>>0;
	uint16_t r1 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t r2 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t r3 = (ops[0]&0xFFFF000000000000)>>48;
	
	if(ops.size() > 1) { //additional op is acc
	  return (r0+r1+r2+r3+((uint16_t)ops[1]));
	} 
	return (r0+r1+r2+r3);
	
}

inline uint64_t Copy(std::array<uint64_t,2> ops) {
	return ops[0];
}

inline uint64_t And(std::array<uint64_t,2> ops) {
	return ops[0] & ops[1]; 
}

inline uint64_t Mul16x4(std::array<uint64_t,2> ops) {
	uint16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
	uint16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
	uint16_t b0 = (ops[1]&0x000000000000FFFF)>>0;
	uint16_t b1 = (ops[1]&0x00000000FFFF0000)>>16;
	uint16_t b2 = (ops[1]&0x0000FFFF00000000)>>32;
	uint16_t b3 = (ops[1]&0xFFFF000000000000)>>48;
	a0*=b0;
	a1*=b1;
	a2*=b2;
	a3*=b3;
	uint64_t c0 = (uint64_t)(a0)<<0;
	uint64_t c1 = (uint64_t)(a1)<<16;
	uint64_t c2 = (uint64_t)(a2)<<32;
	uint64_t c3 = (uint64_t)(a3)<<48;
	return c0 | c1 | c2 | c3;
	//return (uint64_t) _mm_mullo_pi16((__m64)ops[0], (__m64)ops[1]);
}

inline uint64_t Sub16x4(std::array<uint64_t,2> ops) {
	uint16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
	uint16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
	uint16_t b0 = (ops[1]&0x000000000000FFFF)>>0;
	uint16_t b1 = (ops[1]&0x00000000FFFF0000)>>16;
	uint16_t b2 = (ops[1]&0x0000FFFF00000000)>>32;
	uint16_t b3 = (ops[1]&0xFFFF000000000000)>>48;
	a0-=b0;
	a1-=b1;
	a2-=b2;
	a3-=b3;
	uint64_t c0 = (uint64_t)(a0)<<0;
	uint64_t c1 = (uint64_t)(a1)<<16;
	uint64_t c2 = (uint64_t)(a2)<<32;
	uint64_t c3 = (uint64_t)(a3)<<48;
	return c0 | c1 | c2 | c3;
	//return (uint64_t) _mm_adds_pu16((__m64)ops[0], (__m64)ops[1]);
}

inline uint64_t TAdd16x4(std::array<uint64_t,2> ops) {
	uint32_t a0 = (ops[0]&0x000000000000FFFF)>>0;
	uint32_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
	uint32_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
	uint32_t a3 = (ops[0]&0xFFFF000000000000)>>48;
	uint32_t b0 = (ops[1]&0x000000000000FFFF)>>0;
	uint32_t b1 = (ops[1]&0x00000000FFFF0000)>>16;
	uint32_t b2 = (ops[1]&0x0000FFFF00000000)>>32;
	uint32_t b3 = (ops[1]&0xFFFF000000000000)>>48;
	a0+=b0;
	a1+=b1;
	a2+=b2;
	a3+=b3;
	uint64_t c0 = (uint64_t)(a0&0x0000FFFF)<<0;
	uint64_t c1 = (uint64_t)(a1&0x0000FFFF)<<16;
	uint64_t c2 = (uint64_t)(a2&0x0000FFFF)<<32;
	uint64_t c3 = (uint64_t)(a3&0x0000FFFF)<<48;
	return c0 | c1 | c2 | c3;
	//return (uint64_t) _mm_adds_pu16((__m64)ops[0], (__m64)ops[1]);
}

inline uint64_t Add64(std::array<uint64_t,2> ops) {
	return ops[0] + ops[1]; 
}

inline uint64_t Add16x4(std::array<uint64_t,2> ops) {
	uint16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
	uint16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
	uint16_t b0 = (ops[1]&0x000000000000FFFF)>>0;
	uint16_t b1 = (ops[1]&0x00000000FFFF0000)>>16;
	uint16_t b2 = (ops[1]&0x0000FFFF00000000)>>32;
	uint16_t b3 = (ops[1]&0xFFFF000000000000)>>48;
	a0+=b0;
	a1+=b1;
	a2+=b2;
	a3+=b3;
	uint64_t c0 = (uint64_t)(a0)<<0;
	uint64_t c1 = (uint64_t)(a1)<<16;
	uint64_t c2 = (uint64_t)(a2)<<32;
	uint64_t c3 = (uint64_t)(a3)<<48;
	return c0 | c1 | c2 | c3;
	//return (uint64_t) _mm_adds_pu16((__m64)ops[0], (__m64)ops[1]);
}

inline uint64_t Div16x4(std::array<uint64_t,2> ops) {
	uint16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
	uint16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
	uint16_t b0 = (ops[1]&0x000000000000FFFF)>>0;
	uint16_t b1 = (ops[1]&0x00000000FFFF0000)>>16;
	uint16_t b2 = (ops[1]&0x0000FFFF00000000)>>32;
	uint16_t b3 = (ops[1]&0xFFFF000000000000)>>48;
	a0/=b0;
	a1/=b1;
	a2/=b2;
	a3/=b3;
	uint64_t c0 = (uint64_t)(a0)<<0;
	uint64_t c1 = (uint64_t)(a1)<<16;
	uint64_t c2 = (uint64_t)(a2)<<32;
	uint64_t c3 = (uint64_t)(a3)<<48;
	return c0 | c1 | c2 | c3;
}

inline uint64_t RedMax16x4(std::array<uint64_t,2> ops) {
	uint16_t r0 = (ops[0]&0x000000000000FFFF)>>0;
	uint16_t r1 = (ops[0]&0x00000000FFFF0000)>>16;
	uint16_t r2 = (ops[0]&0x0000FFFF00000000)>>32;
	uint16_t r3 = (ops[0]&0xFFFF000000000000)>>48;
	
	uint16_t x = r0;
	if(r1 > x) {x=r1;}
	if(r2 > x) {x=r2;}
	if(r3 > x) {x=r3;}
	
	if(ops.size() > 1) { //additional op is acc
	  uint16_t b = (uint16_t)ops[1];
	  if(b > x) {x=b;}
	} 
	return x;
	
}

inline uint64_t Sig16(std::array<uint64_t,2> ops) {
	#define SIG (op*1024/(1024+op))
	//#define SIG op
	
	uint16_t op = (uint16_t)ops[0];
	
	if(ops.size() > 1) {
	  if(ops[1]) {
	    return (uint64_t) SIG; 
	  } else {
	    return ops[0];
	  }
	}
	return (uint64_t) SIG;
}

#endif
