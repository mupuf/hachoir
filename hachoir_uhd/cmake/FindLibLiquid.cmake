# - Try to find Libliquid
# Once done this will define
#  LIBLIQUID_FOUND - System has Libliquid
#  LIBLIQUID_INCLUDE_DIRS - The Libliquid include directories
#  LIBLIQUID_LIBRARIES - The libraries needed to use Libliquid

find_path(LIBLIQUID_INCLUDE_DIR liquid/liquid.h)
find_library(LIBLIQUID_LIBRARY_DIR NAMES libliquid.so)
find_library(LIBLIQUID_LIBRARY NAMES libliquid.so)

set(LIBLIQUID_LIBRARIES ${LIBLIQUID_LIBRARY} )
set(LIBLIQUID_INCLUDE_DIRS ${LIBLIQUID_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBLIQUID_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LibLiquid  DEFAULT_MSG
                                  LIBLIQUID_LIBRARY LIBLIQUID_LIBRARY_DIR
                                  LIBLIQUID_INCLUDE_DIR)

mark_as_advanced(LIBLIQUID_INCLUDE_DIR LIBLIQUID_LIBRARY )
