project(covertListener)

find_package(Vulkan REQUIRED)

add_executable(covertListener covertListener.cpp ${CMAKE_SOURCE_DIR}/../../ext/easyvk/easyvk.cpp)

target_include_directories(covertListener PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(covertListener PRIVATE ${CMAKE_SOURCE_DIR}/../../ext/cxxopts/include/)
target_include_directories(covertListener PRIVATE ${CMAKE_SOURCE_DIR}/../../ext/easyvk)
target_include_directories(covertListener PUBLIC ${Vulkan_INCLUDE_DIRS})

target_link_libraries (covertListener ${Vulkan_LIBRARIES})

