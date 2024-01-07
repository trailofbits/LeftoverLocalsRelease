# LeftoverLocals
A repo containing PoC code for the LeftoverLocals GPU memory leaking project. 

Each directory contains a different project, e.g., testing for LeftoverLocals using a different GPU framework. We also include a PoC of a co-resident attack on an LLM.

We summarize each project below, and then provide a list of devices/platforms we have tested. Each project has a more detailed README inside of it's directory. 

LeftoverLocals can be illustrated by setting up a [covert communication channel](https://en.wikipedia.org/wiki/Covert_channel) between two processes, and so much of our code utilizes that terminology, e.g., _covert listener_ for a process that recieves information through a covert channel and _covert writer_ for a process that provides information through the covert channel. In this case, we are using GPU local memory as the covert communication mechanism.

## Projects

### `AppleApp`
This tests for LeftoverLocals using two Xcode projects: a covert listener and a covert writer. The code can be deployed as two mobile apps (on an iPhone or iPad), or as a GUI app on a Mac OS device (a laptop). The apps can do a dump of GPU memory, but also synchronize on an agreed prefix canary. This allows the writer to send messages to the listener. The app utilizes Apple's Metal GPU programming framework and communicates through threadgroup memory.

### `VulkanCLI`
This contains two command line C++ programs that set up a covert communication channel, similar to the AppleApp. In this case, the Vulkan GPU programming framework is used. The listener repeatedly dumps a histogram of observed values. The writer repeatedly writes a canary value (123). The output of the listener can be parsed to search for the canary.

### `OpenCLCLI`
This is the same as VulkanCLI, but it goes through the OpenCL GPU programming framework.

### `VulkanAndroid`
This is similar to the AppleApp, but written for Android. It does not have the same features (e.g., the ability to search for a prefix), but instead it simply performs a dump of local memory. It goes through the Vulkan GPU programming framework.

### `WebGPUListener`
This is a simple webpage that dumps GPU local memory through WebGPU. We did not observe anything other than zeros, so we did not write the listener/writer combo programs here.

### `PoCLLMAttack`
This PoC shows how a co-resident attacker can listen to the output of an LLM. It is a fork of llama.cpp, with the listener of OpenCLCLI adapted to search for certain patterns. 

## Tested Devices/Platforms
If you test device/platform that isn't on this list, please make a PR with your results!

### Observed LeftoverLocals

These device/framework combinations revealed clear evidence of the LeftoverLocals vulnerability 

| Device (GPU)                    	           | GPU Framework 	| OS/Driver/Build system                                            | Project  	    | 
|---------------------------------	           |--------------	|------------------------------------------------------------------	|----------	    |
| iPhone 12 Pro (A14 Bionic)                   | Metal        	| iOS 16.6, Xcode Version 14.3.1 (14E300c)                         	| AppleApp 	    | 
| iPad Air 3rd Gen (A12 Bionic)                | Metal        	| iOS 16.5.1, Xcode Version 14.3.1 (14E300c)                       	| AppleApp 	    | 
| MacBook Air (M2)                	           | Metal        	| Mac OS (13.4.1 (22F770830e)), Xcode Version 14.3.1 (14E300c) 	    | AppleApp 	    | 
| AMD Radeon RX 7900 XT                        | Vulkan         | Mesa 23.1.4                                                       | VulkanCLI     |
| AMD Radeon RX 7900 XT                        | OpenCL         | OpenCL 2.1 AMD-APP.dbg (3570.0)                                   | OpenCLCLI     |
| AMD Ryzen 7 5700G (integrated GPU)           | Vulkan         | Mesa 23.1.4                                                       | VulkanCLI     |
| HTC OnePlus 11 (Qualcomm Snapdragon 8 Gen 2) | Vulkan         | Android 13, Android Studio Giraffe (2022.3.1)                     | VulkanAndroid |

### Observed Non-zero Local Memory

These device/framework combinations reveal non-zero values when local memory is dumped, but the canary values are not observed

| Device (GPU)                    	           | GPU Framework 	| OS/Driver/Build system                                            | Project  	    | 
|---------------------------------	           |--------------	|------------------------------------------------------------------	|----------	    |
| Google Pixel 6 (Arm Mali G78)                | Vulkan         | Android 13, Android Studio Giraffe (2022.3.1)                     | VulkanAndroid |
| Google Pixel 6 (Arm Mali G71)                | Vulkan         | Android 11, Android Studio Giraffe (2022.3.1)                     | VulkanAndroid |

### Observed All Zeros

These device/framework combinations only revealed zeros when local memory is dumped.

| Device (GPU)                    	           | GPU Framework 	| OS/Driver/Build system                                            | Project  	     | 
|---------------------------------	           |--------------	|------------------------------------------------------------------	|----------	     |
| Nvidia GeForce RTX 4070                      | Vulkan         | Arch Linux, Mesa 23.1.4                                           | VulkanCLI      |
| Nvidia GeForce RTX 4070                      | OpenCL         | Arch Linux, OpenCL 3.0 CUDA 12.2.128 (535.86.05)                  | OpenCLCLI      |
| MacBook Air (M2)                	           | WebGPU        	| Mac OS (13.4.1 (22F770830e)), Chrome 116.0.5845.110             	| WebGPUListener | 
| Intel NUC (NUC10I5FNK)                       | Vulkan         | Mesa 20.3.2                                                       | VulkanCLI      |
| Intel NUC (NUC10I5FNK)                       | OpenCL         | OpenCL 3.0 NEO (22.31.23852)                                      | OpenCLCLI      |
| Motorola Moto G (Imagination PowerVR GE8320) | Vulkan         | Android 11, Android Studio Giraffe (2022.3.1)                     | VulkanAndroid  |

### PoC LLM Attack

These device/framework combinations were used to show the LLM PoC attack. We have only one device because it is the GPU that is (1) most likely to be used in a server setting to accelerate an LLM; and (2) contains the LeftoverLocals vulnerability.

| Device (GPU)                    	           | GPU Framework 	| OS/Driver/Build system                                            | Project  	    | 
|---------------------------------	           |--------------	|------------------------------------------------------------------	|----------	    |
| AMD Radeon RX 7900 XT                        | OpenCL         | OpenCL 2.1 AMD-APP.dbg (3570.0)                                   | PoCLLMAttack     |

## Dependencies

The `ext` directory two projects: 

* [cxxopts](https://github.com/jarro2783/cxxopts): a C++ command line parsing utility, which is provided with a permissive license
* [EasyVK](https://github.com/ucsc-chpl/easyvk), a Vulkan wrapper library from Tyler Sorensen's research group, which is provided with the Apache License 2.0 

## License

All of the projects in this repo are licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

Some of the files in the VulkanAndroid App are included from [The Android Open Source Project](https://source.android.com/), which are also licensed under the Apache License 2.0. These files maintain their copyright notice at the top.

The exception is the PoC LLM attack; it is a fork of [llama.cpp](https://github.com/ggerganov/llama.cpp), and thus, maintains its MIT license.
