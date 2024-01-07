# LeftoverLocals VulkanCLI
Tests if one process can see GPU local memory values from another process. This project is written in C++ and utilizes the Vulkan GPU programming framework.

## Overview
This project contains two applications: 

* *covertListener*: repeatedly outputs a dump of uninitialized GPU shared memory
* *covertWriter*: repeatedly writes a canary value (123 by default) to GPU shared memory

To check for LeftoverLocals, run covertWriter on one process, and run CovertListener on another process. The covertWriter will write a canary value to memory, the CovertListener should try to observe the canary value.

## Building

### Makefile (Linux)

On a linux system, simply navigate to an application directory (i.e., `covertListener` or `covertWriter`) and run `make`. This assumes that you have the Vulkan library and include directory in your library and include paths, respectively.

### CMake (Linux)

To build navigate to the application directory (i.e., `covertListener` or `covertWriter`). Then follow the standard CMake process.

1. `mkdir build`
2. `cd build`
3. `cmake ../`
4. `make` 

### CMake (Windows)

#### Prereqs

Please make sure you have a C++ compiler for Windows. For example, download "Build tools for Visual Studio 2022" (under All Downloads > Tools for VS) from [here](https://visualstudio.microsoft.com/downloads/) Install it and choose "C++" on the wizard to get the compilers on your system.

Next install the Vulkan SDK from [here](https://vulkan.lunarg.com/sdk/home).

We will assume the next steps are performed from the "x64 native tools command prompt for VS 2022", which can be opened from the start menu. This has all the compilers, cmake, etc ready to use in `PATH`.

#### Building

`cd` to the covertListener or Writer directory on that cmd window, then:

```
mkdir build
cd build
cmake ..
cmake --build .
```

Built exe should be in `Debug/`

#### Executing

To execute the application, run the following two commands.

```
cd ..
.\build\Debug\covertListener.exe
```

### Building SPIR-V kernels

This project has prebuilt SPIR-V binaries. If you want to change the GPU kernel, it requires some effort. We don't want to write SPIR-V directly, so instead, we write OpenCL (i.e., the `.cl` files in `covertListener` and `covertWriter`). These files are then compiled with [clspv](https://github.com/google/clspv). This utility is not provided in this repo and should be obtained seperately and added to your path. At that point. the Makefile will recompile the kernel if you modify the `.cl` file.

Finally, the code uses the [easyVK](https://github.com/ucsc-chpl/easyvk/) header from Tyler Sorensen's group at UCSC, which makes writing Vulkan applications a little less painful (but potentially removes some expressivity). 

## Executing

To execute the covertListener, change directory into `covertListener` and run: `./build/covertListener`

This will repeatedly dump the contents of GPU shared memory. The dump is in the form of a histogram, with the top 10 most seen values printed. The values are printed as a pair `(OBSERVATION, NUMBER OF OBSERVATIONS)`, e.g., the output (123, 500) means that the value 123 was seen 500 times. The histogram is sorted to show the most commonly seen values first. This will execute until the process is killed (e.g. with ctrl-c).

To execute the covertWriter, change directory into `covertWriter` and run: `./build/covertWriter`

This will repeatedly write a canary value in GPU shared memory. This will execute until the process is killed (e.g. with ctrl-c). The application will print one dot (.) every 1K iterations, just as a visual indicator that it is making progress.

## Options

Both applications take the following flags:
* `-h` : Show a list of options
* `-l, --list` : List the GPUs that are visible to Vulkan and exit
* `-d, --device` INT: Select a GPU (using an integer ID from the order printed from `-l`
*  `--wgs` INT: Number of threads in the workgroup (default: 256)
*  `--gs` INT:  Number of workgroups per grid (default: 32)

The covertWriter additionally takes `-c, --canary`, which can specify the canary value to be written (default: 123).

## Examples

### Dumping Local Memory

To take a dump of local memory, simply run the covertListener. That is, change directory into `covertListener` and run: `./build/covertListener`.

If the GPU is zero'ing out local memory, you should only see zero values. For example, here is an output from the Nvidia GeForce RTX 4070. You can see that this device sees 0. Specifically, it sees the value `0` 131072 times, which is how many memory locations it checked. This example shows two iterations, or two local memory dumps. 

```
$ ./build/covertCLListener -d 1
using device: NVIDIA GeForce RTX 470
printing out a histogram of observations.
------------
top 10 observations:
(0,131072)
------------
Next iteration starting
top 10 observations:
(0,131072)
```

If we run the same program on a program that has the LeftoverLocals vulnerability, we see lots of different values in Local memory. Here is an example of 2 iterations on the AMD Radeon RX 7900 XT. Here we see many values in local memory. e.g., in the first iteration the value `4255591666` was observed 2 times, etc.

```
Using device: AMD Radeon RX 7900 XT
printing out a histogram of observations.
---------
top 10 observations:
(4255591666,2)
(402937040,2)
(47614872,1)
(3506451760,1)
(3670046378,1)
(299397396,1)
(199442122,1)
(1065584991,1)
(2690245749,1)
(3022974051,1)
------------
Next iteration starting
top 10 observations:
(3944144013,2)
(3426197498,2)
(4293898099,2)
(3623271101,2)
(3789525439,2)
(215997478,2)
(70526976,2)
(1735809015,1)
(2686013077,1)
(2285036842,1)
```

### Adding the Writer

To test for LeftoverLocals, start two command line instances, potentially in different users or Docker containers. Execute the covertWriter on one command line: change directory into `covertWriter` and run: `./build/covertWriter`

In the other command line, run the covertListener. If the covertListener only reports zeros (e.g., see the example from the Nvidia GPU above), then the GPU does not show evidence of the LeftoverLocals vulnerability, at least through the Vulkan framework.

However, if the covertListener observes the canary value (default 123), then we say that the GPU does show evidence of the LeftoverLocals vulnerability. For example, when targetting the AMD Radeon RX 7900 XT, if we run the covertWriter in one process, and the covertListener in another, we see the following output. Notice how we now exclusively see instances of the value `123`, which was written by the covertWriter.

```
Using device: AMD Radeon RX 7900 XT
printing out a histogram of observations.
---------
top 10 observations:
(123,131072)
------------
Next iteration starting
top 10 observations:
(123,131072)
```

If the covertListener does not observe zero values, but also does not observe the canary, we still flag that observation as interesting, but it is not clear evidence of LeftoverLocals. This was the case with the ARM devices, e.g., the Pixel 6.

### Canary Value Observations

For some GPUs, like the AMD Radeon RX 7900 XT discussed above, we might only observe the canary value in local memory. In other instances, e.g., the AMD Ryzen 7 5700G integrated GPU, we see the canary value in fewer iterations, and it does not appear as every value in the memory dump. Similarly, the amount of canary values observed depends on several factors:

1. The amount of local memory per workgroup, which is defined in `common.h` for each application.
2. The amount of workgroups and size of workgroups.

These two values will determine how much of the physical local memory will be written to and read from. Optimal values will be different for each chip, and we have set values that appear to work well across a variety of devices. 

We advise a user to run the covertListener (with the covertWriter running in the background) for many iterations and output the observations to a file. The file can then be searched for the canary value.



### Video

A video provided in LeftoverLocalsVulkanCLI.mp4 shows a demo of the application pair running on an AMD Radeon RX 7900 XT, which has the LeftoverLocals vulnerability.


