accum+=ops[0];

uint64_t ret = accum;

if(ops[1]) {
  accum=0;
}

return ret; 
