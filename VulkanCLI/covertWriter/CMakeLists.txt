project(covertWriter)

find_package(Vulkan REQUIRED)

add_executable(covertWriter covertWriter.cpp ${CMAKE_SOURCE_DIR}/../../ext/easyvk/easyvk.cpp)

target_include_directories(covertWriter PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(covertWriter PRIVATE ${CMAKE_SOURCE_DIR}/../../ext/cxxopts/include/)
target_include_directories(covertWriter PRIVATE ${CMAKE_SOURCE_DIR}/../../ext/easyvk)
target_include_directories(covertWriter PUBLIC ${Vulkan_INCLUDE_DIRS})

target_link_libraries (covertWriter ${Vulkan_LIBRARIES})

