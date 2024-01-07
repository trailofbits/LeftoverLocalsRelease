cmake_minimum_required(VERSION 3.18.1)

project("vklistener")

add_library(
        vklistener
        SHARED
        cppListener.cpp easyvk.cpp)

find_library(
        log-lib
        log)

find_library(
        android-lib
        android )

target_link_libraries(
        vklistener
        ${log-lib} ${android-lib} vulkan)

