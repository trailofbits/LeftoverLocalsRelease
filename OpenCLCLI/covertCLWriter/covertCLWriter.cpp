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

#include <vector>
#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include <fstream>

// For command line
#include "cxxopts.hpp"

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

#include <CL/opencl.hpp>
using namespace cl;

// for sorting the histogram
bool cmp(std::pair<int, int> a,
	 std::pair<int, int> b) {
  return a.second > b.second;
}

int main(int argc, char* argv[]) {
  
  cxxopts::Options options("covertListener", "reads values from GPU memory to search for canaries written by the covert listener");

  // Could think about shared memory size as a parameter, but it would
  // probably need to be declared in a higher-level script because it needs
  // to modify the kernel
  options.add_options()
    ("l,list", "List devices") // a bool parameter
    ("wgs, workgroup-size", "Number of threads in the workgroup", cxxopts::value<int>()->default_value("256"))
    ("gs, grid-size", "Number of workgroups per grid", cxxopts::value<int>()->default_value("32"))
    ("d,device", "Device id (int)", cxxopts::value<int>()->default_value("0"))
    ("c,canary", "Canary value (int)", cxxopts::value<int>()->default_value("123"))
    ("h,help", "Print usage");
  
  
  auto result = options.parse(argc, argv);
  
  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    exit(0);
  }

  vector<Platform> platforms;
  Platform::get(&platforms);

  std::vector<cl::Device> all_devices;
  int id = 0;

  for (auto p: platforms) {
    std::vector<Device> local_devices;
    p.getDevices(CL_DEVICE_TYPE_ALL, &local_devices);
    for (auto d: local_devices) {
      all_devices.push_back(d);
      id++;
    }
  }     

  id = 0;
  if (result.count("list")) {
    for (auto p: platforms) {
      std::cout << "platform: " << p.getInfo<CL_PLATFORM_NAME>() << std::endl;
      std::vector<Device> local_devices;
      p.getDevices(CL_DEVICE_TYPE_ALL, &local_devices);
      for (auto d: local_devices) {
	std::cout << "  device " << id << ": " << d.getInfo<CL_DEVICE_NAME>() << std::endl;
	std::cout << "   local memory size: " << d.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << " type: " << d.getInfo<CL_DEVICE_LOCAL_MEM_TYPE>() << std::endl;
	all_devices.push_back(d);
	id++;
      }
      std::cout << std::endl;      
    }    
    exit(0);
  }

  int deviceID = result["device"].as<int>();
  Device d = all_devices[deviceID];

  std::cout << "using device: " << d.getInfo<CL_DEVICE_NAME>() << std::endl;

  Context context({d});
  Program::Sources sources;

  std::ifstream t("covertCLWriter.cl");
  std::stringstream buffer;
  buffer << t.rdbuf();

  sources.push_back({buffer.str().c_str(),buffer.str().length()});

  Program program(context,sources);
  if(program.build({d})!=CL_SUCCESS){
    std::cout<<" Error building: "<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(d)<<"\n";
    exit(1);
  }

  int gridSize = result["grid-size"].as<int>();
  int workgroupSize = result["workgroup-size"].as<int>();
  int globalSize = gridSize*workgroupSize;
  int size = MAX_SHMEM_SIZE * gridSize;
  int size_float = size/4;

  // create buffers on the device
  Buffer buffer_A(context,CL_MEM_READ_WRITE,size);
  Buffer buffer_B(context,CL_MEM_READ_WRITE,size);
  Buffer buffer_C(context,CL_MEM_READ_WRITE,size);

  int * A = (int*) malloc(size);
  int * B = (int*) malloc(size);
  int * C = (int*) malloc(size);

  CommandQueue queue(context,d);

  string fname = "readValues";
  int iters = 0;

  Kernel kernel_add=cl::Kernel(program,"covertWriter");
  kernel_add.setArg(0,buffer_A);
  kernel_add.setArg(1,buffer_B);
  kernel_add.setArg(2,buffer_C);
  
  int canary = result["canary"].as<int>();

  std::cout<< "writing canary value: " << result["canary"].as<int>() << std::endl;

  std::cout << "entering writing loop. Printing one dot (.) for every 1K iterations. Kill with ctrl-c to stop the writer" << std::endl;

  std::cout << "---------" << std::endl;
  while (1) {

    iters++;
    if (iters % 1000 == 0) {
      std::cout << "." <<  std::flush;
    }

    for (int i = 0; i < size_float; i++) {
      A[i] = 0;
      B[i] = C[i] = canary;
    }

    queue.enqueueWriteBuffer(buffer_A, CL_TRUE, 0, size, A);
    queue.enqueueWriteBuffer(buffer_B, CL_TRUE, 0, size ,B);
    queue.enqueueWriteBuffer(buffer_C, CL_TRUE, 0, size ,C);

    queue.enqueueNDRangeKernel(kernel_add, 0, NDRange(globalSize),NDRange(workgroupSize));
    queue.finish();

  }
  
}
