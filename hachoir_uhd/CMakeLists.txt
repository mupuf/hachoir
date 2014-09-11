cmake_minimum_required (VERSION 2.6)
project (Hachoir_UHD)

include(FindPkgConfig)

include_directories(${CMAKE_SOURCE_DIR})

# Enable C++'11
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
	message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

add_definitions("-Wall -Wno-format")

find_package(Boost COMPONENTS program_options system thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

# Modulations / Demodulations
include(cmake/FindLibLiquid.cmake)
option(USE_LIBLIQUID "Enable support for libliquid functions" ON)
if(LIBLIQUID_FOUND AND USE_LIBLIQUID)
	include_directories(${LIBLIQUID_INCLUDE_DIRS})
	link_directories(${LIBLIQUID_LIBRARY_DIRS})
	add_definitions("-DHAS_LIBLIQUID")
else(${LIBLIQUID_FOUND} AND ${LIB_LIQUID_ENABLE})
	set(LIBLIQUID_LIBRARY "")
	set(USE_LIBLIQUID OFF)
endif(${LIBLIQUID_FOUND} AND ${LIB_LIQUID_ENABLE})

# Find all the common files
file(GLOB_RECURSE common_src "demodulations/*.cpp" "demodulations/*.h"
			     "modulations/*.cpp" "modulations/*.h"
			     "utils/*.cpp" "utils/*.h")
set(common_libs ${Boost_LIBRARIES} ${LIBLIQUID_LIBRARY})

# Backends
pkg_search_module(libuhd uhd)
option(APPS_UHD_ENABLE "Enable applications depending on the libuhd" ON)
if(libuhd_FOUND AND APPS_UHD_ENABLE)
	include_directories(${libuhd_INCLUDE_DIRS})
	link_directories(${libuhd_LIBRARY_DIRS})

	add_executable(hachoir_uhd ${common_src} "drivers/uhd/hachoir_uhd.cpp")
        target_link_libraries(hachoir_uhd ${common_libs} ${libuhd_LIBRARIES})

	add_executable(tx_samples_from_file ${common_src} "drivers/uhd/tx_samples_from_file.cpp")
        target_link_libraries(tx_samples_from_file ${common_libs} ${libuhd_LIBRARIES})

	add_executable(rx_samples_to_file ${common_src} "drivers/uhd/rx_samples_to_file.cpp")
        target_link_libraries(rx_samples_to_file ${common_libs} ${libuhd_LIBRARIES})
else()
	set(APPS_UHD_ENABLE "OFF")
endif()

pkg_search_module(librtl librtlsdr)
option(APPS_RTL_ENABLE "Enable applications depending on the librtlsdr" ON)
if(librtl_FOUND AND APPS_RTL_ENABLE)
	include_directories(${librtl_INCLUDE_DIRS})
	link_directories(${librtl_LIBRARY_DIRS})

	add_executable(hachoir_rtl ${common_src} "drivers/rtl/hachoir_rtl.cpp")
        target_link_libraries(hachoir_rtl ${common_libs} ${librtl_LIBRARIES})
else()
	set(APPS_RTL_ENABLE "OFF")
endif()

pkg_search_module(libbladerf libbladeRF)
option(APPS_BRF_ENABLE "Enable applications depending on the libbladeRF" ON)
if(libbladerf_FOUND AND APPS_BRF_ENABLE)
	include_directories(${libbladerf_INCLUDE_DIRS})
	link_directories(${libbladerf_LIBRARY_DIRS})

	add_executable(hachoir_brf ${common_src}
				   "drivers/bladerf/common.cpp"
				   "drivers/bladerf/hachoir_brf.cpp")
        target_link_libraries(hachoir_brf ${common_libs} ${libbladerf_LIBRARIES})

	add_executable(hachoir_brf_tx ${common_src}
				   "drivers/bladerf/common.cpp"
				   "drivers/bladerf/hachoir_brf_tx.cpp")
        target_link_libraries(hachoir_brf_tx ${common_libs} ${libbladerf_LIBRARIES})
else()
	set(APPS_BRF_ENABLE "OFF")
endif()

option(APPS_TEST_ENABLE "Enable test applications" ON)
if(APPS_TEST_ENABLE)
	add_executable(replay_samples ${common_src} "drivers/tests/replay_samples.cpp")
        target_link_libraries(replay_samples ${common_libs})
endif()


message("")
message("== Summary ==")
message("")
message("Build options:")
message("	Build type: ${CMAKE_BUILD_TYPE} (-DCMAKE_BUILD_TYPED=Debug|Release)")
message("")
message("Applications:")
message("	UHD applications:  ${APPS_UHD_ENABLE} (-DAPPS_UHD_ENABLE=(ON|OFF))")
message("	RTL applications:  ${APPS_RTL_ENABLE} (-DAPPS_RTL_ENABLE=(ON|OFF))")
message("	BRF applications:  ${APPS_BRF_ENABLE} (-DAPPS_BRF_ENABLE=(ON|OFF))")
message("	Test applications: ${APPS_TEST_ENABLE} (-DAPPS_TEST_ENABLE=(ON|OFF))")
message("")
message("Mods/Demods:")
message("	liquid DSP:        ${USE_LIBLIQUID} (-DUSE_LIBLIQUID=(ON|OFF))")
message("")
message("== /Summary ==")
message("")
