#ifndef SB_H
#define SB_H
#include <stdint.h>
#include <deque>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <typeinfo>
#include <type_traits>
#include <cstring>
using namespace std;

#if defined(SB_DEBUG_MSG) 
#define DEBUG_PRINTF(message, arg) \
printf(message, arg); 
#else
#define DEBUG_PRINTF(message, arg) ;
#endif

#define SCRATCHPAD_SIZE 8192
//Will need to read in the config file to set proper values for the 
//CGRA itself in terms of types, but will write that later
struct sb_config {
  void (*dfg_func)(uint64_t**, uint64_t**);
  int num_inputs;
  int* input_widths;
  int num_outputs;
  int* output_widths;
  int work;
  int span;
};

enum class InputMode {DATA, RECURRENCE};
enum class OutputMode {PTR_U, PTR_I, SHF16_U, SHF16_I, RECURRENCE, GARBAGE, SCRATCH};
enum class OutputType {ULL, ILL, UL, IL, U, I, UC, IC};
class SoftBrain {
 public: 
  SoftBrain();
  SoftBrain(sb_config mem_addr, long size);
  ~SoftBrain();
  void dma_read(void* mem_addr, long stride, long access_size, long num_strides, int port);
  void dma_write(int port, long stride, long access_size, long num_strides, void* mem_addr);
  void dma_write_shf16(int port, long stride, long access_size, long num_strides, void* mem_addr);
    void dma_scratch_load(void* mem_addr, long stride, long access_size, long num_strides, int scratch_addr);
    void scr_port_stream(int scr_addr, long stride, long access_size, long num_strides, int port); 
    void scratch_read(int scr_addr, long num_bytes, int port);
    void scratch_write(int port, long num_bytes, int scr_addr);
    template<typename T>
    void sb_const(int port, T val, int num);
    void wait_all();
    void recurrence(uint64_t output_port, int input_port, int num_strides);
    void garbage(int port, int num);
 private:
  void (*dfg_func)(uint64_t**, uint64_t**);
  void verify_lengths();
  int execute_dfg();
  void process_recurrence(uint64_t** outputs);

  uint64_t **inputs;
  uint64_t **outputs;
  uint64_t *input_temp;
  uint64_t *output_temp;

  int num_inputs;
  int num_outputs;
  int iterations;
  int executions;
  int aggregate_iterations;
  int aggregate_executions;
  int pipeline_fill;
  bool recurrence_check;
  int work;
  int span;
  long size;
  sb_config saved_config;
  //Unlike reading from memory, scratchpad MUST be able to be accessed at byte level
  uint8_t* scratchpad;
  int scratchpad_tail;
  //streams
  struct input_port_instance {
    InputMode mode; //Mode = 0 means second value is the value
    uint64_t* data;
  };

  struct input_stream {
    int width;
    deque<input_port_instance> fifo;
    input_stream() {
    }
  };

  struct output_port_instance {
    OutputMode mode; //Mode = 0 means second value is the value
    OutputType typing; //Which type to use for this output iteration in mem
    void** data;
  };

  struct output_stream {
    int width;
    deque<output_port_instance> fifo;
    output_stream() {
    }
  };

  input_stream* input_streams;
  output_stream* output_streams;
};

template<typename T>
void SoftBrain::sb_const(int port, T val, int num) {
  assert(port < num_inputs);
  assert((num % input_streams[port].width) == 0);
  //Need to copy the contents of val into an iteration
  uint64_t* ullval = (uint64_t*) malloc(sizeof(uint64_t));
  *ullval = 0;
  std::memcpy(ullval, &val, sizeof(T));
  for(int str = 0; str < (num/input_streams[port].width); str++) {
    input_port_instance next_instance;
    next_instance.mode = InputMode::DATA;
    next_instance.data = (uint64_t*) malloc(sizeof(uint64_t)*input_streams[port].width);
    for(int j = 0; j < input_streams[port].width; j++) {
      next_instance.data[j] = *ullval;
    }
    input_streams[port].fifo.push_back(next_instance);
  }
  free(ullval);
  while(execute_dfg());
}

#endif
