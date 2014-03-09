cmake_minimum_required (VERSION 2.6)
project (Hachoir_UHD)

INCLUDE(FindPkgConfig)

add_definitions("-std=c++11")

find_package(Boost COMPONENTS program_options system thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

# Find all the common files
file(GLOB common_src "*.cpp" "*.h")

pkg_search_module(libuhd uhd)
option(APPS_UHD_ENABLE "Enable applications depending on the libuhd" ON)
if(libuhd_FOUND AND APPS_UHD_ENABLE)
	include_directories(${libuhd_INCLUDE_DIRS})
	link_directories(${libuhd_LIBRARY_DIRS})

	add_executable(hachoir_uhd ${common_src} "uhd/hachoir_uhd.cpp")
	target_link_libraries(hachoir_uhd ${Boost_LIBRARIES} ${libuhd_LIBRARIES})

	add_executable(tx_samples_from_file "uhd/tx_samples_from_file.cpp")
	target_link_libraries(tx_samples_from_file ${Boost_LIBRARIES} ${libuhd_LIBRARIES})

	add_executable(rx_samples_to_file "uhd/rx_samples_to_file.cpp")
	target_link_libraries(rx_samples_to_file ${Boost_LIBRARIES} ${libuhd_LIBRARIES})
else()
	set(APPS_UHD_ENABLE "OFF")
endif()

pkg_search_module(librtl librtlsdr)
option(APPS_RTL_ENABLE "Enable applications depending on the librtlsdr" ON)
if(librtl_FOUND AND APPS_RTL_ENABLE)
	include_directories(${librtl_INCLUDE_DIRS})
	link_directories(${librtl_LIBRARY_DIRS})

	add_executable(hachoir_rtl ${common_src} "rtl/hachoir_rtl.cpp")
	target_link_libraries(hachoir_rtl ${Boost_LIBRARIES} ${librtl_LIBRARIES})
else()
	set(APPS_RTL_ENABLE "OFF")
endif()

message("")
message("== Summary ==")
message("")
message("Build options:")
message("	Build type: ${CMAKE_BUILD_TYPE} (-DCMAKE_BUILD_TYPED=Debug|Release)")
message("")
message("Applications:")
message("	UHD applications: ${APPS_UHD_ENABLE} (-DAPPS_UHD_ENABLE=(ON|OFF))")
message("	RTL applications: ${APPS_RTL_ENABLE} (-DAPPS_RTL_ENABLE=(ON|OFF))")
message("")
message("== /Summary ==")
message("")
