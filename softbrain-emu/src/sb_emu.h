#ifndef SB_EMU_H
#define SB_EMU_H
#ifndef _REENTRANT
#include <stdint.h>
#include <type_traits>
#include "sb.h"

using namespace std;
//Will need to read in the config file to set proper values for the 
//CGRA itself in terms of types, but will write that later

//Goal here is to have each define as used by the SB map to a call
//in a class file that does what the softbrain does. Hence, each define 
//maps to a matching call on the class SoftBrain.
extern SoftBrain *sb_emu;

//Stream in the Config
#define SB_CONFIG(mem_addr, size) \
  if(sb_emu == NULL) { \
    sb_emu = new SoftBrain(mem_addr, size);	\
  } else { \
    delete sb_emu; \
    sb_emu = new SoftBrain(mem_addr, size);	\
  }

//Fill the scratchpad from DMA (from memory or cache)
//Note that scratch_addr will be written linearly
#define SB_DMA_SCRATCH_LOAD(mem_addr, stride, access_size, num_strides, scratch_addr) \
  sb_emu->dma_scratch_load(static_cast<void*>(mem_addr), stride, access_size, num_strides, scratch_addr);

//Read from scratch into a cgra port
#define SB_SCR_PORT_STREAM(scr_addr, stride, access_size, num_strides, port ) \
  sb_emu->scr_port_stream(scr_addr, stride, access_size, num_strides, port); 

//A convienience CMD if you want to read linearly
#define SB_SCRATCH_READ(scr_addr, num_bytes, port) \
  sb_emu->scratch_read(scr_addr, num_bytes, port);

//Read from DMA into a port
#define SB_DMA_READ(mem_addr, stride, access_size, num_strides, port ) \
  sb_emu->dma_read(static_cast<void*>(mem_addr), stride, access_size, num_strides, port);

//Throw away some outputs.  We will add a proper instruction for this at some point, rather then writing to memory
#define SB_GARBAGE(output_port, num_elem) \
  sb_emu->garbage(output_port, num_elem); 

//Write to DMA.
#define SB_DMA_WRITE(output_port, stride, access_size, num_strides, mem_addr) \
  sb_emu->dma_write(output_port, stride, access_size, num_strides, static_cast<void*>(mem_addr));

//Write to DMA, but throw away all but the last 16-bits from each word
#define SB_DMA_WRITE_SHF16(output_port, stride, access_size, num_strides, mem_addr) \
  sb_emu->dma_write_shf16(output_port, stride, access_size, num_strides, static_cast<void*>(mem_addr));

//Write to DMA, but throw away all but the last 16-bits from each word
//WARNING -- (NOT IMPLEMENTED IN SIMULTOR YET)
//#define SB_DMA_WRITE_SHF32(output_port, stride, access_size, num_strides, mem_addr) \
  __asm__ __volatile__("sb_stride   %0, %1" : : "r"(stride), "r"(access_size)); \
  __asm__ __volatile__("sb_wr_dma   %0, %1, %2"   : : "r"(mem_addr),  "r"(num_stirides), "i"(output_port|0x80)); 

//  __asm__ __volatile__("sb_dma_addr %0, %1" : : "r"(access_size), "r"(stride)); \
//  __asm__ __volatile__("sb_wr %0 "          : : "i"(output_port)); \
//  __asm__ __volatile__("sb_stride   %0, %1" : : "r"(mem_addr), "r"(stride)); \
//  __asm__ __volatile__("sb_dma_addr_p %0, %1, " #output_port : : "r"(mem_addr), "r"(stride_size)); \
//  __asm__ __volatile__("sb_dma_wr   %0, " : : "r"(num_strides)); 

//Send a constant value, repetated num_elements times to a port
#define SB_CONST(port, val, num_elements) \
  sb_emu->sb_const(port, val, num_elements); 

//Write to Scratch from a CGRA output port.  Note that only linear writes are currently allowed
#define SB_SCRATCH_WRITE(output_port, num_bytes, scratch_addr) \
  sb_emu->scratch_write(output_port, num_bytes, scratch_addr); 

//Write from output to input port
#define SB_RECURRENCE(output_port, input_port, num_strides) \
  sb_emu->recurrence(static_cast<uint64_t>(output_port), input_port, num_strides);

//Wait with custom bit vector -- probably don't need to use
//#define SB_WAIT(bit_vec) \
  __asm__ __volatile__("sb_wait t0, t0, " #bit_vec); \

//Wait for all softbrain commands to be done -- This will block the processor indefinately if there is
//unbalanced commands
#define SB_WAIT_ALL() \
  sb_emu->wait_all();

//For now, cast wait to wait all
#define SB_WAIT(wait_amt) \
  ;

//Wait for all prior scratch writes to be complete.
#define SB_WAIT_SCR_WR() ;
  //Do nothing for a wait			 \

//wait for everything except outputs to be complete. (useful for debugging)
#define SB_WAIT_COMPUTE() ;
//__asm__ __volatile__("sb_wait t0, t0, 2");	\

//wait for all prior scratch reads to be complete (NOT IMPLEMENTED IN SIMULTOR YET)
#define SB_WAIT_SCR_RD() ;
//__asm__ __volatile__("sb_wait t0, t0, 4");	\

#endif

#ifdef _REENTRANT
#include <stdint.h>
#include <type_traits>
#include "sb.h"
#include <map>
#include <pthread.h>


using namespace std;
//Will need to read in the config file to set proper values for the 
//CGRA itself in terms of types, but will write that later

//Goal here is to have each define as used by the SB map to a call
//in a class file that does what the softbrain does. Hence, each define 
//maps to a matching call on the class SoftBrain.
extern map<pthread_t, SoftBrain*>* softbrains;
extern pthread_mutex_t configlock; 

//Stream in the Config
#define SB_CONFIG(mem_addr, size) \
  pthread_mutex_lock(&configlock); \
  if(softbrains == NULL) { \
    softbrains = new map<pthread_t, SoftBrain*>(); \
  } \
  auto sb = softbrains->find(pthread_self()); \
  if(sb == softbrains->end()) {			   \
    softbrains->insert(make_pair(pthread_self(), new SoftBrain(mem_addr, size))); \
  } else { \
    delete sb->second; \
    sb->second = new SoftBrain(mem_addr, size);	\
  } \
  pthread_mutex_unlock(&configlock);

//Fill the scratchpad from DMA (from memory or cache)
//Note that scratch_addr will be written linearly
#define SB_DMA_SCRATCH_LOAD(mem_addr, stride, access_size, num_strides, scratch_addr) \
  softbrains->find(pthread_self())->second->dma_scratch_load(static_cast<void*>(mem_addr), stride, access_size, num_strides, scratch_addr);

//Read from scratch into a cgra port
#define SB_SCR_PORT_STREAM(scr_addr, stride, access_size, num_strides, port ) \
  softbrains->find(pthread_self())->second->scr_port_stream(scr_addr, stride, access_size, num_strides, port); 

//A convienience CMD if you want to read linearly
#define SB_SCRATCH_READ(scr_addr, num_bytes, port) \
  softbrains->find(pthread_self())->second->scratch_read(scr_addr, num_bytes, port);

//Read from DMA into a port
#define SB_DMA_READ(mem_addr, stride, access_size, num_strides, port ) \
  softbrains->find(pthread_self())->second->dma_read(static_cast<void*>(mem_addr), stride, access_size, num_strides, port);

//Throw away some outputs.  We will add a proper instruction for this at some point, rather then writing to memory
#define SB_GARBAGE(output_port, num_elem) \
  softbrains->find(pthread_self())->second->garbage(output_port, num_elem); 

//Write to DMA.
#define SB_DMA_WRITE(output_port, stride, access_size, num_strides, mem_addr) \
  softbrains->find(pthread_self())->second->dma_write(output_port, stride, access_size, num_strides, static_cast<void*>(mem_addr));

//Write to DMA, but throw away all but the last 16-bits from each word
#define SB_DMA_WRITE_SHF16(output_port, stride, access_size, num_strides, mem_addr) \
  softbrains->find(pthread_self())->second->dma_write_shf16(output_port, stride, access_size, num_strides, static_cast<void*>(mem_addr));

//Write to DMA, but throw away all but the last 16-bits from each word
//WARNING -- (NOT IMPLEMENTED IN SIMULTOR YET)
//#define SB_DMA_WRITE_SHF32(output_port, stride, access_size, num_strides, mem_addr) \
  __asm__ __volatile__("sb_stride   %0, %1" : : "r"(stride), "r"(access_size)); \
  __asm__ __volatile__("sb_wr_dma   %0, %1, %2"   : : "r"(mem_addr),  "r"(num_stirides), "i"(output_port|0x80)); 

//  __asm__ __volatile__("sb_dma_addr %0, %1" : : "r"(access_size), "r"(stride)); \
//  __asm__ __volatile__("sb_wr %0 "          : : "i"(output_port)); \
//  __asm__ __volatile__("sb_stride   %0, %1" : : "r"(mem_addr), "r"(stride)); \
//  __asm__ __volatile__("sb_dma_addr_p %0, %1, " #output_port : : "r"(mem_addr), "r"(stride_size)); \
//  __asm__ __volatile__("sb_dma_wr   %0, " : : "r"(num_strides)); 

//Send a constant value, repetated num_elements times to a port
#define SB_CONST(port, val, num_elements) \
  softbrains->find(pthread_self())->second->sb_const(port, val, num_elements); 

//Write to Scratch from a CGRA output port.  Note that only linear writes are currently allowed
#define SB_SCRATCH_WRITE(output_port, num_bytes, scratch_addr) \
  softbrains->find(pthread_self())->second->scratch_write(output_port, num_bytes, scratch_addr); 

//Write from output to input port
#define SB_RECURRENCE(output_port, input_port, num_strides) \
  softbrains->find(pthread_self())->second->recurrence(static_cast<uint64_t>(output_port), input_port, num_strides);

//Wait with custom bit vector -- probably don't need to use
//#define SB_WAIT(bit_vec) \
  __asm__ __volatile__("sb_wait t0, t0, " #bit_vec); \

//Wait for all softbrain commands to be done -- This will block the processor indefinately if there is
//unbalanced commands
#define SB_WAIT_ALL() \
  softbrains->find(pthread_self())->second->wait_all();

//For now, cast wait to wait all
#define SB_WAIT(wait_amt) \
  ;

//Wait for all prior scratch writes to be complete.
#define SB_WAIT_SCR_WR() ;
  //Do nothing for a wait			 \

//wait for everything except outputs to be complete. (useful for debugging)
#define SB_WAIT_COMPUTE() ;
//__asm__ __volatile__("sb_wait t0, t0, 2");	\

//wait for all prior scratch reads to be complete (NOT IMPLEMENTED IN SIMULTOR YET)
#define SB_WAIT_SCR_RD() ;
//__asm__ __volatile__("sb_wait t0, t0, 4");	\

#endif

#endif
