cmake_minimum_required (VERSION 2.6) 
project (Hachoir_UHD)

link_directories (${HELLO_BINARY_DIR}/Hello) 
add_executable (hachoir_uhd hachoir_uhd.cpp) 
target_link_libraries (hachoir_uhd "-luhd") 
