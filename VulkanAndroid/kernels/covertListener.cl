#include "common.h"

__kernel void covertListener(__global volatile uint *a, __global volatile uint *b, __global volatile uint *c, __global volatile uint* canary) {
  local volatile uint lm[SHARED_MEMORY_SIZE];

  // gotta use all the args or CLSPV just optimizes them out
  for (int i = get_local_id(0); i < SHARED_MEMORY_SIZE_TRAVERSED; i+= get_local_size(0)) {
    c[(SHARED_MEMORY_SIZE_TRAVERSED * get_group_id(0)) + i] = lm[i];
    a[(SHARED_MEMORY_SIZE_TRAVERSED * get_group_id(0)) + i] = *canary;
    b[(SHARED_MEMORY_SIZE_TRAVERSED * get_group_id(0)) + i] = 123;
    
  }
    
}
