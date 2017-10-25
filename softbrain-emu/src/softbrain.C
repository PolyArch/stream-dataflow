#include <cstdlib>
#include <cassert>
#include <cstdio>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "sb.h"
#include <map>
#include <pthread.h>

using namespace std;
#if defined(SB_DEBUG_MSG) 
#define DEBUG_PRINTF(message, arg) \
printf(message, arg); 
#else
#define DEBUG_PRINTF(message, arg) ;
#endif

#if defined(SB_PERF_MSG) 
#define PERF_PRINTF(message, arg)			\
  printf(message, arg); 
#else
#define PERF_PRINTF(message, arg) ;
#endif

uint64_t accum = 0;
SoftBrain *sb_emu = NULL;
map<pthread_t, SoftBrain *>* softbrains = NULL;
pthread_mutex_t configlock;

SoftBrain::SoftBrain(sb_config mem_addr, long size) {
  //implement reading config
  assert(size > 0);
  dfg_func = mem_addr.dfg_func;
  num_inputs = mem_addr.num_inputs;
  work = mem_addr.work;
  span = mem_addr.span;
  iterations = 0;
  executions = 0;
  aggregate_iterations = 0;
  aggregate_executions = 0;
  pipeline_fill = span;
  recurrence_check = true;
  int input_entry_amt = 0;
  int output_entry_amt = 0;
  input_streams = new input_stream[num_inputs];
  for(int i = 0; i < num_inputs; i++) {
    input_streams[i].width = mem_addr.input_widths[i];
    input_entry_amt += input_streams[i].width;
    DEBUG_PRINTF("Input width is %i\n", input_streams[i].width);
  }
  num_outputs = mem_addr.num_outputs;
  output_streams = new output_stream[num_outputs];
  for(int i = 0; i < num_outputs; i++) {
    output_streams[i].width = mem_addr.output_widths[i];
    output_entry_amt += output_streams[i].width;
    DEBUG_PRINTF("Output width is %i\n", output_streams[i].width);
  }

  inputs = (uint64_t**) malloc(sizeof(uint64_t*)*num_inputs);
  input_temp = (uint64_t*) malloc(sizeof(uint64_t)*input_entry_amt);
  outputs = (uint64_t**) malloc(sizeof(uint64_t*)*num_outputs);
  output_temp = (uint64_t*) malloc(sizeof(uint64_t)*output_entry_amt);

  scratchpad = (uint8_t*) malloc(sizeof(uint8_t)*SCRATCHPAD_SIZE);
  scratchpad_tail = 0;
  saved_config = mem_addr;
}

SoftBrain::~SoftBrain() {
  delete[] input_streams;
  delete[] output_streams;
  free(inputs);
  free(input_temp);
  free(outputs);
  free(output_temp);
  free(scratchpad);
}

int SoftBrain::execute_dfg() {
	//Check to see that all inputs and outputs have at least one entry
	//First, verify inputs and sum total type
	int input_entry_amt = 0;
	int output_entry_amt = 0;
	for(int i = 0; i < num_inputs; i++) {
		if(input_streams[i].fifo.empty()) {
			return 0;
		}
		input_entry_amt += input_streams[i].width;
	}
	//Next, outputs
	for(int i = 0; i < num_outputs; i++) {
		if(output_streams[i].fifo.empty()) {
			return 0;
		}
		output_entry_amt += output_streams[i].width;
	}
	//Package all of the data into pointer structs expected by the dfg function call
	input_entry_amt = 0;
	for(int i = 0; i < num_inputs; i++) {
		if(i == 0) {
			inputs[i] = &input_temp[0];
		} else {
			inputs[i] = &input_temp[input_entry_amt];
		}
		input_entry_amt += input_streams[i].width;
		input_port_instance next = input_streams[i].fifo.front();
		if(next.mode != InputMode::DATA) {
			printf("Next instance is not data when attempting to execute iteration of a dfg\n");
			assert(0);
		}
		DEBUG_PRINTF("Setting up DFG call. Iteration port %i", i);
		for(int j = 0; j < input_streams[i].width; j++) {
			DEBUG_PRINTF(" Place %i:", j);
			DEBUG_PRINTF(" %llu, ", next.data[j]);
			//inputs[i][j] = next.data[j];
			std::memcpy((void*) inputs[i], (void*) next.data, input_streams[i].width*8);
		}
		DEBUG_PRINTF(" -- next after port %i\n", i);
		free(input_streams[i].fifo.front().data);
		input_streams[i].fifo.pop_front();
	}
	//Initialize output data pointer
	output_entry_amt = 0;
	for(int i = 0; i < num_outputs; i++) {
		if(i == 0) {
			outputs[i] = &output_temp[0];
		} else {
			outputs[i] = &output_temp[output_entry_amt];
		}
		output_entry_amt += output_streams[i].width;
	}
	//call the dfg
	DEBUG_PRINTF("Calling iteration %i of the DFG.\n", iterations);
	iterations++;
	dfg_func(inputs, outputs);
	//write to outputs 
	//pop what the output should do and write it. 
	for(int i = 0; i < num_outputs; i++) {
	  if(!output_streams[i].fifo.empty()) {
	    output_port_instance next = output_streams[i].fifo.front();
	    if(next.mode == OutputMode::PTR_U || next.mode == OutputMode::SCRATCH || next.mode == OutputMode::SHF16_U || next.mode == OutputMode::SHF16_I) {
	      int sourceSizing = 4;
	      int dataIter = 0;
	      for(int j = 0; j < output_streams[i].width; j++) {
		for(int k = 0; k < sourceSizing; k++) { 
		  DEBUG_PRINTF("Output writing... source needs %i writes per place\n", sourceSizing);
		  if(next.mode == OutputMode::SHF16_U) {
		    if(k == 0) {
		      std::memcpy(output_streams[i].fifo.front().data[j], static_cast<void*>(&outputs[i][j]), 2);
		    } else {
		      //static_cast<uint16_t*>(output_streams[i].fifo.front().data[j])[k] = 0;
		    }
		  } else {
		    std::memcpy((void*) &(static_cast<uint16_t*>(output_streams[i].fifo.front().data[j])[k]), (void*) &(static_cast<uint16_t*>(static_cast<void*>(&outputs[i][j]))[k]), 2);
		  }
		  DEBUG_PRINTF("Void pointer address of 0x%p\n",output_streams[i].fifo.front().data[j]); 
		  DEBUG_PRINTF("Printing out to a 16 bit unsigned with val %u\n", static_cast<uint16_t*>(static_cast<void*>(&outputs[i][j]))[k]);
		  DEBUG_PRINTF("Printed out to a 16 bit unsigned with val %u\n", static_cast<uint16_t*>(output_streams[i].fifo.front().data[j])[k]);
		  DEBUG_PRINTF("Printed out to a 64 bit unsigned with val %llu\n", static_cast<uint64_t*>(output_streams[i].fifo.front().data[j])[0]);
		}
	      }
	    } 
	  }
	}
	//call recurrence to copy any outputs to the proper inputs
	process_recurrence(outputs);
	for(int i = 0; i < num_outputs; i++) {
	  //Pop everything once
	  DEBUG_PRINTF("Popping output entry from %i\n", i);
	  if(!output_streams[i].fifo.empty()) {
	    // free only if allocated with malloc i.e. PTR_U
	    if (output_streams[i].fifo.front().mode == OutputMode::PTR_U) 
	      free(output_streams[i].fifo.front().data); 
	    output_streams[i].fifo.pop_front();
	  }
	}
	return 1;
}

void SoftBrain::dma_read(void* mem_addr, long stride, long access_size, long num_strides, int port) {
  //assume mem addr is a pointer to a 64 bit region
  /* int source_size = sizeof(T); */
  /* T* mem_ptr = mem_addr; */
  int source_size = sizeof(uint16_t);
  //cast just for incrementing the stride
  char* mem_ptr = (char*) mem_addr;
  bool partial = false;
  int partialIndex = 0;
  int bytesLeft = 0;
  int accessLeft = access_size;
  DEBUG_PRINTF("Size of source is %i\n", source_size);
  //Verify the access size is a multiple of the size of the port. 
  int width_in_bytes = sizeof(uint64_t)*input_streams[port].width;
  DEBUG_PRINTF("Reading in to port %i", port);
  DEBUG_PRINTF(" with a stride of %i", stride);
  DEBUG_PRINTF(" an access size of %i", access_size);
  DEBUG_PRINTF(" and a number of strides of %i\n", num_strides);
  if((access_size*num_strides)%width_in_bytes != 0) {
    printf("Access size is not a multiple of the width of the port.\n");
    assert((access_size*num_strides)%width_in_bytes == 0);
  }
  //Verified that it can be read in.
  input_port_instance* next_instance = new input_port_instance;
  next_instance->mode = InputMode::DATA;
  next_instance->data = (uint64_t*) malloc(sizeof(uint64_t)*input_streams[port].width);
  for(int str = 0; str < num_strides; str++) {
    char* temp_ptr = mem_ptr;
    while(accessLeft != 0) {
      if(accessLeft >= width_in_bytes && !partial) {
	std::memcpy((void*) next_instance->data, (void*) mem_ptr, width_in_bytes);
	accessLeft -= width_in_bytes;
	input_streams[port].fifo.push_back(*next_instance);
	delete next_instance; // copied when push_back'd
	next_instance = new input_port_instance;
	next_instance->mode = InputMode::DATA;
	next_instance->data = (uint64_t*) malloc(sizeof(uint64_t)*input_streams[port].width);
	mem_ptr = &mem_ptr[width_in_bytes];
      } else {
	//Could be the beginning or end of a partial
	if(partial) {
	  //Always assume that we can finish a 64 bit at least
	  if(accessLeft >= bytesLeft) {
	    std::memcpy((void*) &(next_instance->data[partialIndex]), (void*) mem_ptr, bytesLeft);
	    input_streams[port].fifo.push_back(*next_instance);
	    delete next_instance;
	    next_instance = new input_port_instance;
	    next_instance->mode = InputMode::DATA;
	    next_instance->data = (uint64_t*) malloc(sizeof(uint64_t)*input_streams[port].width);
	    mem_ptr = &mem_ptr[bytesLeft];
	    accessLeft -= bytesLeft;
	    partialIndex = 0;
	    bytesLeft = 0;
	    partial = false;
	  } else {
	    //Continue partial build
	    std::memcpy((void*) &(next_instance->data[partialIndex]), (void*) mem_ptr, accessLeft);
	    mem_ptr = &mem_ptr[accessLeft];
	    partialIndex = partialIndex + accessLeft/8;
	    bytesLeft = bytesLeft-accessLeft;
	    accessLeft = 0;
	  }
	} else {
	  partial = true;
	  //Copy what remains
	  std::memcpy((void*) next_instance->data, (void*) mem_ptr, accessLeft);
	  mem_ptr = &mem_ptr[accessLeft];
	  partialIndex = accessLeft/8;
	  bytesLeft = width_in_bytes-accessLeft;
	  accessLeft = 0;
	}
      }
    }
    mem_ptr = &temp_ptr[stride];
    accessLeft = access_size;
  }
  free(next_instance->data);
  delete next_instance;
  while(execute_dfg());
}

void SoftBrain::dma_scratch_load(void* mem_addr, long stride, long access_size, long num_strides, int scratch_addr) {
    /* T* mem_ptr = mem_addr; */
    /* int source_size = sizeof(T); */
  uint8_t* mem_ptr = (uint8_t*) (mem_addr);
  scratchpad_tail = scratch_addr;
  DEBUG_PRINTF("Reading into scratchpad with a stride of %i", stride);
    DEBUG_PRINTF(" an access size of %i", access_size);
    DEBUG_PRINTF(" and a number of strides of %i\n", num_strides);
    if((access_size*num_strides) > SCRATCHPAD_SIZE) {
      printf("Too many bytes to be read in to scratchpad.\n");
      assert(access_size*num_strides <= SCRATCHPAD_SIZE);
    }
    for(int str = 0; str < num_strides; str++) {
      //scratchpad[scratchpad_tail] = (uint8_t) (data >> ((shift-1)*8));
      //Read in the entire access size
      std::memcpy((void*) &scratchpad[scratchpad_tail], mem_ptr, access_size);
      //DEBUG_PRINTF("Data to be pushed was %llu\n", data);
      //DEBUG_PRINTF("Pushing a byte into the scratchpad. Value is %#x\n", scratchpad[scratchpad_tail]);
      //(scratchpad_tail == SCRATCHPAD_SIZE-1) ? scratchpad_tail = 0 : scratchpad_tail++;
      scratchpad_tail += access_size;
      if(scratchpad_tail >= SCRATCHPAD_SIZE)
	scratchpad_tail -= SCRATCHPAD_SIZE;
      mem_ptr = &mem_ptr[access_size];
    }
    //No call to execute dfg. This doesn't actually read to ports at all
}

/* template<class T> */
/* void SoftBrain::dma_write(int port, long stride, long access_size, long num_strides, T* mem_addr) { */
void SoftBrain::dma_write(int port, long stride, long access_size, long num_strides, void* mem_addr) {
  //Store the location to write to similarly to dma read
  DEBUG_PRINTF("DMA write of address %i\n", ((uint16_t*) mem_addr));
  DEBUG_PRINTF("    value current of  %i\n", ((uint16_t*) mem_addr)[0]);
  /* T* mem_ptr = mem_addr; */
  /* int sizeOfType = sizeof(T); */
  uint16_t* mem_ptr = static_cast<uint16_t*>(mem_addr);
  int sizeOfType = sizeof(uint16_t);
  int width_in_bytes = sizeof(uint64_t)*output_streams[port].width;
  OutputType type;
  OutputMode mode;
  int maxForPort = output_streams[port].width;
  int iterationIndex = 0;
  int interIter = 0;
  type = OutputType::U;
  mode = OutputMode::PTR_U;
  if((access_size*num_strides)%width_in_bytes != 0) {
    printf("Access size * strides is not a multiple of the width of the port.\n");
    assert((access_size*num_strides)%width_in_bytes == 0);
  }
  //Verified that it can be read in.
  output_port_instance* next_instance = new output_port_instance;
  next_instance->mode = mode;
  next_instance->typing = type; 
  next_instance->data = (void**) malloc(sizeof(void*)*output_streams[port].width);
  for(int str = 0; str < num_strides; str++) {
    //T* temp_ptr = mem_ptr;
    uint16_t* temp_ptr = mem_ptr;
    for(int i = 0; i < access_size/8; i++) {
      //The "data" is merely the starting location for the write
      next_instance->data[interIter++] = (void*) temp_ptr;
      temp_ptr = &temp_ptr[8/sizeOfType];
      DEBUG_PRINTF("Address is %i ", temp_ptr);
      DEBUG_PRINTF(" current value of: %i\n", *temp_ptr);
      if(interIter == output_streams[port].width) {
	//Done reading in this particular instance. 
	interIter = 0;
	output_streams[port].fifo.push_back(*next_instance);
	delete next_instance;
	next_instance = new output_port_instance;
	next_instance->mode = mode;
	next_instance->typing = type; 
	next_instance->data = (void**) malloc(sizeof(void*)*output_streams[port].width);
      }
    }
    mem_ptr = &mem_ptr[stride/sizeOfType];
  }
  free(next_instance->data);
  delete next_instance;
  //Now that it is in, call potential dfg_func
  while(execute_dfg());
}

void SoftBrain::verify_lengths() {
  //iterate through each input and output port and verify theyt are the same length
  int length = -1;
  for(int i = 0; i < num_inputs; i++) {
    DEBUG_PRINTF("Input port %i ", i);
    DEBUG_PRINTF("has size %i\n", input_streams[i].fifo.size());
		if(length == -1) {
			length = input_streams[i].fifo.size();
		}
		else if(length != input_streams[i].fifo.size()) {
			printf("Wait called and streams are not equal, deadlock.\n");
			assert(0);
		}
	}
	//Next, outputs
	for(int i = 0; i < num_outputs; i++) {
		DEBUG_PRINTF("Output port %i ", i);
		DEBUG_PRINTF("has size %i\n", output_streams[i].fifo.size());
		if(length != output_streams[i].fifo.size()) {
			printf("Wait called and streams are not equal, deadlock.\n");
			assert(0);
		}
	}
}

void SoftBrain::scratch_write(int port, long num_bytes, int scr_addr) {
  //This should be MUCH simpler. 
       //Store the location to write to similarly to dma read
     DEBUG_PRINTF("Scratch write of address %i\n", scr_addr);
     DEBUG_PRINTF("    value current of  %i\n", scratchpad[scr_addr]);
     int width_in_bytes = sizeof(uint64_t)*output_streams[port].width;
     OutputType type;
     OutputMode mode;
     int maxForPort = output_streams[port].width;
     int iterationIndex = 0;
     int interIter = 0;
     type = OutputType::U;
     mode = OutputMode::SCRATCH;
     if(num_bytes%width_in_bytes != 0) {
       printf("Number of bytes to write to scratch is not a multiple of the width of the port.\n");
       assert(num_bytes%width_in_bytes == 0);
     }
     //Verified that it can be read in.
     output_port_instance* next_instance = new output_port_instance;
     next_instance->mode = mode;
     next_instance->typing = type; 
     next_instance->data = (void**) malloc(sizeof(void*)*output_streams[port].width);
     int temp_addr = scr_addr;
     for(int str = 0; str < num_bytes/width_in_bytes; str++) {
	 //The "data" is merely the starting location for the write
	 next_instance->data[interIter++] = (void*) &scratchpad[temp_addr];
	 temp_addr = temp_addr+8;
	 DEBUG_PRINTF("Address is %i ", temp_addr);
	 DEBUG_PRINTF(" current value of: %i\n", scratchpad[temp_addr]);
	 if(interIter == output_streams[port].width) {
	   //Done reading in this particular instance. 
	   interIter = 0;
	   output_streams[port].fifo.push_back(*next_instance);
	   delete next_instance;
	   next_instance = new output_port_instance;
	   next_instance->mode = mode;
	   next_instance->typing = type; 
	   next_instance->data = (void**) malloc(sizeof(void*)*output_streams[port].width);
	 }
     }
     //Now that it is in, call potential dfg_func
     while(execute_dfg());
}

void SoftBrain::process_recurrence(uint64_t** outputs) {
	//Iterate through inputs and see if the front of any of them are recurrence mode
	int recurrenceDone[num_outputs] = {0};
	int lowest_depth_of_use = span+1;
	DEBUG_PRINTF("%i: Beginning recurrence processing.\n", 0);
	for(int i = 0; i < num_inputs; i++) {
	  int depth_of_use = 0;
	  DEBUG_PRINTF("Checking recurrence on %i\n", i);
		for(auto iter = input_streams[i].fifo.begin(); iter != input_streams[i].fifo.end(); iter++) {
		  depth_of_use++;
		  if(iter->mode == InputMode::RECURRENCE) {
		    //Assure that the output port has enough data to fill the input port
		    DEBUG_PRINTF("Recurrence found. Output port is %i\n", *(iter->data));
		    uint64_t output_port = *(iter->data);
		    assert(output_port <= num_outputs);
		    if(output_streams[output_port].fifo.front().mode == OutputMode::RECURRENCE) {
		      if(recurrenceDone[output_port] == 0) {
			assert(!output_streams[output_port].fifo.empty());
			//It is possible that the recurrence has not yet reached the head. Try and wait until it is
			assert(output_streams[output_port].fifo.front().mode == OutputMode::RECURRENCE);
			//output_streams[output_port].fifo.pop_front();
			recurrenceDone[output_port] = 1;
			DEBUG_PRINTF("Popped recurrence from %i.\n", output_port);
		      }
		      if(depth_of_use < lowest_depth_of_use) {
			//The depth of the reuse is less than the span
			//Incur a penalty in terms of pipelining the depth
			lowest_depth_of_use = depth_of_use;
		      }
		      assert(output_port <= num_outputs);
		      free (iter->data);
		      assert(input_streams[i].width >= output_streams[output_port].width);
		      iter->data = (uint64_t*) malloc(sizeof(uint64_t)*input_streams[i].width);
		      for(int j = 0; j < input_streams[i].width; j++) {
			//assure this is copying correctly
			//iter->data[j] = outputs[output_port][j];
			std::memcpy((void*) iter->data, (void*) outputs[output_port], input_streams[i].width*8);
		      }
		      iter->mode = InputMode::DATA;
		      break;
		    } else {
		      if(iter == input_streams[i].fifo.begin()) {
			assert(output_streams[output_port].fifo.front().mode == OutputMode::RECURRENCE);
		      }
		    }
		  }
		}
	}
	//Done recurrence processing. As this is the last step, handle performance estimation
	if(lowest_depth_of_use < span && pipeline_fill == 0) {
	  //Not fully pipelined, so only add executions equal to this pipeline amount
	  //Incur penalty forward. 
	  executions += work;
	  //Add dead iterations
	  iterations += span-lowest_depth_of_use;
	  if(recurrence_check) {
	    PERF_PRINTF("%i: Warning, execution has a reuse less than the depth of the configured CGRA.\n", 0);
	    recurrence_check = false;
	  }
	} else if(pipeline_fill != 0) {
	  //Smaller execution amount for first as it needs to fill the pipeline. 
	  executions += work;
	  //Add dead iterations
	  iterations += pipeline_fill-1;
	  pipeline_fill = 0;
	} else {
	  //No pipeline fill penalties or recurrence penalties
	  executions += work;
	}
}


void SoftBrain::garbage(int port, int num) {
  assert(port < num_outputs);
  for(int str = 0; str < (num/output_streams[port].width); str++) {
    output_port_instance next_instance;
    next_instance.mode = OutputMode::GARBAGE;
    output_streams[port].fifo.push_back(next_instance);
  }
  while(execute_dfg());
}

void SoftBrain::wait_all() {
  PERF_PRINTF("%i--- Wait Call Perf Dump ---\n", 0);
  PERF_PRINTF("Number of executions in this region: %i\n", executions);
  PERF_PRINTF("Number of iterations in this region: %i\n", iterations);
  PERF_PRINTF("Average parallelism in this region: %f\n", (((float)executions)/((float)iterations)));
  aggregate_iterations += iterations;
  aggregate_executions += executions;
  iterations = 0;
  executions = 0;
  PERF_PRINTF("Number of executions overall: %i\n", aggregate_executions);
  PERF_PRINTF("Number of iterations overall: %i\n", aggregate_iterations);
  PERF_PRINTF("Average parallelism overall: %f\n", (((float)aggregate_executions)/((float)aggregate_iterations)));
  
  verify_lengths();
}

void SoftBrain::recurrence(uint64_t output_port, int input_port, int num_strides) {
  assert(input_port < num_inputs);
  assert(output_port < num_outputs);
  assert(input_streams[input_port].width == output_streams[output_port].width);
  DEBUG_PRINTF("Recurrence with number of strides of %i\n", num_strides);
  for(int i = 0; i < (num_strides/input_streams[input_port].width); i++) {
  //for(int i = 0; i < num_strides; i++) {
    input_port_instance next_instance;
    next_instance.mode = InputMode::RECURRENCE;
    next_instance.data = (uint64_t*) malloc(sizeof(uint64_t));
    *(next_instance.data) = output_port;
    assert(output_port <= num_outputs);
    assert((*(next_instance).data) <= num_inputs);
    input_streams[input_port].fifo.push_back(next_instance);
    output_port_instance out_instance;
    out_instance.mode = OutputMode::RECURRENCE;
    output_streams[output_port].fifo.push_back(out_instance);
    DEBUG_PRINTF("Pushing recurrence onto input %i from output ", input_port);
    DEBUG_PRINTF("%i\n", output_port);
  }
  while(execute_dfg());
}

void SoftBrain::scr_port_stream(int scr_addr, long stride, long access_size, long num_strides, int port) {
  //Verify the access size is a multiple of the size of the port. 
	int source_size = 1;
	int scratch_address = scr_addr;
	bool partial = false;
	int partialIndex = 0;
	int bytesLeft = 0;
	int accessLeft = access_size;
	if(scratch_address >= SCRATCHPAD_SIZE) {
		scratch_address = scratch_address%SCRATCHPAD_SIZE;
	}
	int width_in_bytes = sizeof(uint64_t)*input_streams[port].width;
	DEBUG_PRINTF("Reading in to port %i from scratchpad", port);
	DEBUG_PRINTF(" from scratchpad address %i", scratch_address);
	DEBUG_PRINTF(" with a stride of %i", stride);
	DEBUG_PRINTF(" an access size of %i", access_size);
	DEBUG_PRINTF(" and a number of strides of %i\n", num_strides);
	if((access_size*num_strides)%width_in_bytes != 0) {
	  printf("Access size is not a multiple of the width of the port.\n");
	  assert((access_size*num_strides)%width_in_bytes == 0);
	}
	//Verified that it can be read in.
	input_port_instance* next_instance = new input_port_instance;
	next_instance->mode = InputMode::DATA;
	next_instance->data = (uint64_t*) malloc(sizeof(uint64_t)*input_streams[port].width);
	for(int str = 0; str < num_strides; str++) {
	  int temp_address = scratch_address;
	  while(accessLeft != 0) {
	    if(accessLeft >= width_in_bytes && !partial) {
	      std::memcpy((void*) next_instance->data, (void*) &scratchpad[scratch_address], width_in_bytes);
	      accessLeft -= width_in_bytes;
	      input_streams[port].fifo.push_back(*next_instance);
	      delete next_instance;
	      next_instance = new input_port_instance;
	      next_instance->mode = InputMode::DATA;
	      next_instance->data = (uint64_t*) malloc(sizeof(uint64_t)*input_streams[port].width);
	      scratch_address = scratch_address+access_size;
	      if(scratch_address >= SCRATCHPAD_SIZE) {
		scratch_address -= SCRATCHPAD_SIZE;
	      }
	    } else {
	      //Could be the beginning or end of a partial
	      if(partial) {
		//Always assume that we can finish a 64 bit at least
		if(accessLeft >= bytesLeft) {
		  std::memcpy((void*) &(next_instance->data[partialIndex]), (void*) &scratchpad[scratch_address], bytesLeft);
		  input_streams[port].fifo.push_back(*next_instance);
		  delete next_instance;
		  next_instance = new input_port_instance;
		  next_instance->mode = InputMode::DATA;
		  next_instance->data = (uint64_t*) malloc(sizeof(uint64_t)*input_streams[port].width);
		  scratch_address = scratch_address+bytesLeft;
		  if(scratch_address >= SCRATCHPAD_SIZE) {
		    scratch_address -= SCRATCHPAD_SIZE;
		  }
		  accessLeft -= bytesLeft;
		  partialIndex = 0;
		  bytesLeft = 0;
		  partial = false;
		} else {
		  //Continue partial build
		  std::memcpy((void*) &(next_instance->data[partialIndex]), (void*) &scratchpad[scratch_address], accessLeft);
		  scratch_address = scratch_address+accessLeft;
		  if(scratch_address >= SCRATCHPAD_SIZE) {
		    scratch_address -= SCRATCHPAD_SIZE;
		  }
		  partialIndex = partialIndex + accessLeft/8;
		  bytesLeft = bytesLeft-accessLeft;
		  accessLeft = 0;
		}
	      } else {
		partial = true;
		//Copy what remains
		std::memcpy((void*) next_instance->data, (void*) &scratchpad[scratch_address], accessLeft);
		scratch_address = scratch_address+accessLeft;
		if(scratch_address >= SCRATCHPAD_SIZE) {
		  scratch_address -= SCRATCHPAD_SIZE;
		}
		partialIndex = accessLeft/8;
		bytesLeft = width_in_bytes-accessLeft;
		accessLeft = 0;
	      }
	    }
	  }
	  scratch_address = temp_address + stride;
	  if(scratch_address >= SCRATCHPAD_SIZE) {
	    scratch_address -= SCRATCHPAD_SIZE;
	  }
	  accessLeft = access_size;
	}
	while(execute_dfg());
}

void SoftBrain::dma_write_shf16(int port, long stride, long access_size, long num_strides, void* mem_addr) {
  //Store the location to write to similarly to dma read
  DEBUG_PRINTF("DMA write 16 of address %i\n", ((uint16_t*) mem_addr));
  DEBUG_PRINTF("    value current of  %i\n", ((uint16_t*) mem_addr)[0]);
  /* T* mem_ptr = mem_addr; */
  /* int sizeOfType = sizeof(T); */
  uint16_t* mem_ptr = static_cast<uint16_t*>(mem_addr);
  int sizeOfType = sizeof(uint16_t);
  int width_in_bytes = sizeof(uint64_t)*output_streams[port].width;
  OutputType type;
  OutputMode mode;
  int maxForPort = output_streams[port].width;
  int iterationIndex = 0;
  int interIter = 0;
  type = OutputType::U;
  mode = OutputMode::SHF16_U;
  if((access_size*num_strides*4)%width_in_bytes != 0) {
    printf("Access size * strides is not a multiple of the width of the port.\n");
    assert((access_size*num_strides*4)%width_in_bytes == 0);
  }
  //Verified that it can be read in.
  output_port_instance* next_instance = new output_port_instance;
  next_instance->mode = mode;
  next_instance->typing = type; 
  next_instance->data = (void**) malloc(sizeof(void*)*output_streams[port].width);
  for(int str = 0; str < num_strides; str++) {
    //T* temp_ptr = mem_ptr;
    uint16_t* temp_ptr = mem_ptr;
    for(int i = 0; i < access_size/2; i++) {
      //The "data" is merely the starting location for the write
      next_instance->data[interIter++] = (void*) temp_ptr;
      temp_ptr = &temp_ptr[1];
      DEBUG_PRINTF("Address is %i ", temp_ptr);
      DEBUG_PRINTF(" current value of: %i\n", *temp_ptr);
      if(interIter == output_streams[port].width) {
	//Done reading in this particular instance. 
	interIter = 0;
	output_streams[port].fifo.push_back(*next_instance);
	delete next_instance;
	next_instance = new output_port_instance;
	next_instance->mode = mode;
	next_instance->typing = type; 
	next_instance->data = (void**) malloc(sizeof(void*)*output_streams[port].width);
      }
    }
    mem_ptr = &mem_ptr[stride/sizeOfType];
  }
  free(next_instance->data);
  delete next_instance;
  //Now that it is in, call potential dfg_func
  while(execute_dfg());
}

void SoftBrain::scratch_read(int scr_addr, long num_bytes, int port) {
	//Just a call to the port stream
	int width_in_bytes = sizeof(uint64_t)*input_streams[port].width;
	scr_port_stream(scr_addr, width_in_bytes, width_in_bytes, num_bytes/width_in_bytes, port);
}
