/*
   Copyright 2023 Trail of Bits

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "common.h"

__kernel void covertWriter(__global volatile uint *a, __global volatile uint *b, __global volatile uint *c, __global volatile uint* canary) {
  local volatile uint lm[SHARED_MEMORY_SIZE];
  uint id = get_global_id(0);
  
  // some silliness here to make sure compilers don't optimize storing the canary
  
  for (uint i = get_local_id(0); i < SHARED_MEMORY_SIZE_TRAVERSED; i+=get_local_size(0)) {
    lm[i] = *canary;
    a[id] = i;	    
  }

  // So that the compiler doesn't optimize away the local memory
  for (uint i = get_local_id(0); i < SHARED_MEMORY_SIZE_TRAVERSED; i+=get_local_size(0)) {
    c[id] = lm[id];
    b[id] = lm[id];
  }    
}
