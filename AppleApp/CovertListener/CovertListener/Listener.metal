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
//
//  Listener.metal

#include <metal_stdlib>

using namespace metal;

template<typename T, size_t N>
struct tint_array {
    const constant T& operator[](size_t i) const constant { return elements[i]; }
    device T& operator[](size_t i) device { return elements[i]; }
    const device T& operator[](size_t i) const device { return elements[i]; }
    thread T& operator[](size_t i) thread { return elements[i]; }
    const thread T& operator[](size_t i) const thread { return elements[i]; }
    threadgroup T& operator[](size_t i) threadgroup { return elements[i]; }
    const threadgroup T& operator[](size_t i) const threadgroup { return elements[i]; }
    T elements[N];
};

struct tint_symbol_5 {
  /* 0x0000 */ tint_array<uint, 1> arr;
};

void tint_symbol_inner(uint local_id, uint3 workgroup_id, threadgroup tint_array<uint, 2>* const tint_symbol_1, device tint_array<uint, 1>* const tint_symbol_2) {
  if ((local_id < 2u)) {
    uint const i_1 = local_id;
    (*(tint_symbol_1))[i_1] = 0u;
  }
  threadgroup_barrier(mem_flags::mem_threadgroup);
  for(uint i = local_id; (i < 8192u); i = (i + 256u)) {
    (*(tint_symbol_2))[((8192u * workgroup_id[0]) + i)] = (*(tint_symbol_1))[i];
    //(*(tint_symbol_1))[i] = 1;
  }
}

kernel void covertListenerKernel(device tint_symbol_5* tint_symbol_4 [[buffer(0)]], uint local_id [[thread_index_in_threadgroup]], uint3 workgroup_id [[threadgroup_position_in_grid]]) {
  threadgroup tint_array<uint, 2> tint_symbol_3;
  tint_symbol_inner(local_id, workgroup_id, &(tint_symbol_3), &((*(tint_symbol_4)).arr));
  return;
}
