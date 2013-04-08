# Copyright 2011,2012 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

########################################################################
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})

list(APPEND gnuradio_gtsrc_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/fftwindow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fft.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fftaverage.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hachoir_c_impl.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/ringbuffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/samplesringbuffer.h
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_library(gnuradio-gtsrc SHARED ${gnuradio_gtsrc_sources})
target_link_libraries(gnuradio-gtsrc ${Boost_LIBRARIES} ${GRUEL_LIBRARIES} ${GNURADIO_CORE_LIBRARIES})
set_target_properties(gnuradio-gtsrc PROPERTIES DEFINE_SYMBOL "gnuradio_gtsrc_EXPORTS")

########################################################################
# Install built library files
########################################################################
install(TARGETS gnuradio-gtsrc
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so/.dylib file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)

########################################################################
# Build and register unit test
########################################################################
include(GrTest)

include_directories(${CPPUNIT_INCLUDE_DIRS})

list(APPEND test_gtsrc_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gtsrc.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/qa_gtsrc.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/qa_hachoir_c.cc
)

add_executable(test-gtsrc ${test_gtsrc_sources})

target_link_libraries(
  test-gtsrc
  ${GNURADIO_CORE_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CPPUNIT_LIBRARIES}
  gnuradio-gtsrc
)

GR_ADD_TEST(test_gtsrc test-gtsrc)
