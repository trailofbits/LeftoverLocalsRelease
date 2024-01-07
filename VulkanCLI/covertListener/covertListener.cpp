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
#include <easyvk.h>
#include <cassert>
#include <vector>
#include <algorithm>

// For command line
#include <cxxopts.hpp>

// Some common sizes across main and the kernel
#include "common.h"

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

    ("h,help", "Print usage");


  auto result = options.parse(argc, argv);

  if (result.count("help")) {
      std::cout << options.help() << std::endl;
      exit(0);
  }

  // Initialize instance
  auto instance = easyvk::Instance(false);
  
  // Get list of available physical devices.
  auto physicalDevices = instance.physicalDevices();

  if (result.count("list")) {
    int i = 0;
    for (auto d: physicalDevices) {
      auto device = easyvk::Device(instance, d);
      std:: cout << i << ": " << device.properties.deviceName << "\n";
      i++;
    }
    exit(0);
  }
 
  int deviceID = result["device"].as<int>();
    
  // Create device from first physical device.
  auto device = easyvk::Device(instance, physicalDevices.at(deviceID));

  std::vector<uint32_t> spvCode =
#include "./spir-v/covertListener.cinit"
    ;	
  
  std::cout << "Using device: " << device.properties.deviceName << "\n";

  int size = SHARED_MEMORY_SIZE_TRAVERSED * result["grid-size"].as<int>();
  
  // Create some GPU buffers. They are needed so that the compiler
  // doesn't just optimize away the kernel
  auto a = easyvk::Buffer(device, size);
  auto b = easyvk::Buffer(device, size);
  auto c = easyvk::Buffer(device, size);
  auto canary = easyvk::Buffer(device, 1);

  std::cout << "printing out a histogram of observations. "<< std::endl;

  canary.store(0,result["grid-size"].as<int>());
  //std::cout << result["grid-size"].as<int>() << std::endl;

  std::cout << "---------" << std::endl;

  long iterations = 0;
  
  // write indefinitely 
  while (1) {
    iterations++;
    //if (iterations % 1000 == 0) {
    //  std::cout << "." <<  std::flush;
    //}
  
    // Write initial values to the buffers.
    for (int i = 0; i < size; i++) {
      a.store(i, 0);
      b.store(i, 0);
      c.store(i, 0);
    }
            
    std::vector<easyvk::Buffer> bufs = {a, b, c, canary};
    
    auto program = easyvk::Program(device, spvCode, bufs);
    
    // Dispatch 4 work groups of size 1 to carry out the work.
    program.setWorkgroups(result["grid-size"].as<int>());
    program.setWorkgroupSize(result["workgroup-size"].as<int>());
    
    // Run the kernel.
    program.initialize("covertListener");
    program.run();

    // Check the return values
    std::unordered_map<int, int> observations;
    //int nonZeros = 0;
    for(int i = 0; i < SHARED_MEMORY_SIZE_TRAVERSED*result["grid-size"].as<int>(); i++) {
      unsigned v = c.load(i);
    
      
      if (observations.find(v) == observations.end()) {
	observations[v] = 1;
      }
      else {
	observations[v]++;
      }
    }

    std::vector<std::pair<unsigned int, unsigned int> > keyValues;
    
    for (auto i : observations) {
      keyValues.push_back(i);
    }

    std::sort(keyValues.begin(), keyValues.end(), cmp);

    printf("top 10 observations:\n");
    int obs = 0;
    for (auto i : keyValues) {
      if (obs < 10) {
	std::cout << "(" << i.first << "," << i.second << ")" << std::endl;
        obs++;
      }
      else {
        break;
      }
    }

    program.teardown();

    std::cout << "------------\nNext iteration starting" << std::endl;


  }
  
  // Cleanup.
  
  a.teardown();
  b.teardown();
  c.teardown();
  device.teardown();
  instance.teardown();
  return 0;
}
