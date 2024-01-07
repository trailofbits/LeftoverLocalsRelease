# LeftoverLocals OpenCLCLI
Tests if one process can see GPU local memory values from another process. This project is written in C++ and utilizes the OpenCL GPU programming framework.

This project is nearly exactly the same as the VulkanCLI project, except it goes through OpenCL instead of Vulkan. This is useful to test different frameworks, and also because OpenCL has a more accessible programming interface and kernel language. 

We refer you to the documentation for the VulkanCLI project. The only difference is that this project does not contain a CMake build script. It only supports Make, and assumes that the OpenCL library and include directory is in your library path and include path, respectively.
