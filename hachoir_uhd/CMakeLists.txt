cmake_minimum_required (VERSION 2.6) 
project (Hachoir_UHD)

find_package(Boost COMPONENTS program_options system thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

file(GLOB hachoir_uhd_src "*.cpp")
add_executable(hachoir_uhd ${hachoir_uhd_src})
target_link_libraries(hachoir_uhd ${Boost_LIBRARIES} "-luhd" )

message("Build type: ${CMAKE_BUILD_TYPE} (-DCMAKE_BUILD_TYPED=Debug|Release)")
