#include "common.h"

__kernel void covertWriter(__global volatile uint *a, __global volatile uint *b, __global volatile uint *c, __global volatile uint* canary) {
  local volatile uint lm[SHARED_MEMORY_SIZE];
  uint id = get_global_id(0);
  
  // some silliness here to make sure compilers don't optimize storing the canary
  
  for (uint i = get_local_id(0); i < SHARED_MEMORY_SIZE_TRAVERSED; i+=get_local_size(0)) {
    lm[i] = *canary;
    a[id] = 123;	    
  }

  // So that the compiler doesn't optimize away the local memory
  for (uint i = get_local_id(0); i < SHARED_MEMORY_SIZE_TRAVERSED; i+=get_local_size(0)) {
    c[id] = lm[id];
    b[id] = lm[id];
  }    
}
