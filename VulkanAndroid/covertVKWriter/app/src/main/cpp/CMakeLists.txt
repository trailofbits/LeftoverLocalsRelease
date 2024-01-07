cmake_minimum_required(VERSION 3.18.1)

project("vkwriter")

add_library( # Sets the name of the library.
        vkwriter

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        CovertVKWriter.cpp easyvk.cpp)

find_library(
        log-lib

        log)

find_library(
        android-lib

        android )

target_link_libraries(
        vkwriter

        ${log-lib} ${android-lib} vulkan)


