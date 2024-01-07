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
//  Writer.metal

#include <metal_stdlib>
using namespace metal;
#define TGMEMSIZE_INT (8*1024)


// test chunked_2t_2i_2 from the forward progress testing work.
// This one seems to cause issues.
kernel void covertListenerKernel(device uint * x,
                                 device uint * messageSize,
                                 device uint * fake,
                                 uint gid_x [[thread_position_in_grid]],
                                 uint total_threads [[threads_per_grid]],
                                 uint lane [[ thread_index_in_simdgroup ]],
                                 uint simd_width [[ threads_per_simdgroup ]],
                                 uint sid_x [[simdgroup_index_in_threadgroup]],
                                 uint tgid [[thread_position_in_threadgroup]],
                                 uint tg_size [[threads_per_threadgroup]],
                                 uint tg_id [[threadgroup_position_in_grid]]) {
    
    threadgroup int local_memory[TGMEMSIZE_INT];
    fake[0] = local_memory[0];
    for (int i = tgid; i < TGMEMSIZE_INT; i+= tg_size) {
        local_memory[i] = x[i%messageSize[0]];
    }
   
}
