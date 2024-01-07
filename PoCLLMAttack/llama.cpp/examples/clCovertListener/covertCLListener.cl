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

// Should be in common.h but it doesn't like to compile for AMD
// devices. 

// common.h start

#define MAX_SHMEM_SIZE 65536

#if !defined(SHARED_MEMORY_SIZE) 
#define SHARED_MEMORY_SIZE_INT (MAX_SHMEM_SIZE/4)
#endif

// In case we want to try out of bounds accesses
#if !defined(SHARED_MEMORY_SIZE_TRAVERSED) 
#define SHARED_MEMORY_SIZE_TRAVERSED (SHARED_MEMORY_SIZE_INT)
#endif

// common.h end


__kernel void covertListener(__global volatile float *A, __global volatile float *B, __global volatile float *C) {
  local volatile float lm[SHARED_MEMORY_SIZE_INT];

  for (int i = get_local_id(0); i < SHARED_MEMORY_SIZE_TRAVERSED; i+= get_local_size(0)) {
    C[(SHARED_MEMORY_SIZE_TRAVERSED * get_group_id(0)) + i] = lm[i];
    lm[i] = 0.0f;
  }
    
}
