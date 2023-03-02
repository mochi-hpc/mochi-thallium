#
# CMakeLists.txt  top-level cmake file for mdcs
# 13-Oct-2017  mdorier@anl.gov
#

#
#  general cmake flags:
#    -DCMAKE_INSTALL_PREFIX=/usr/local     -- the prefix for installing
#    -DCMAKE_BUILD_TYPE=type               -- type can be Debug, Release, ...
#    -DCMAKE_PREFIX_PATH=/dir              -- external packages
#
#     note that CMAKE_PREFIX_PATH can be a list of directories:
#      -DCMAKE_PREFIX_PATH='/dir1;/dir2;/dir3'
#

cmake_minimum_required (VERSION 3.7)
project (thallium CXX)
enable_testing ()

set (CMAKE_CXX_STANDARD 14)

# setup cache variables for ccmake
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release
         CACHE STRING "Choose the type of build." FORCE)
    set_property (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                  "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif ()
option (ENABLE_CEREAL "Enable cereal serialization" ON)

include_directories (${CMAKE_BINARY_DIR}/include)

# packages we depend on
if (ENABLE_CEREAL)
    find_package (cereal CONFIG REQUIRED)
    set (CEREAL_TARGET cereal::cereal)
    get_target_property (CEREAL_INC cereal::cereal INTERFACE_INCLUDE_DIRECTORIES)
    message (STATUS "Found cereal library")
endif (ENABLE_CEREAL)

find_package (PkgConfig REQUIRED)
pkg_check_modules (margo REQUIRED IMPORTED_TARGET margo)

add_subdirectory (src)
if (ENABLE_TESTS)
    add_subdirectory (test)
endif (ENABLE_TESTS)
if (ENABLE_EXAMPLES)
    add_subdirectory (examples)
endif (ENABLE_EXAMPLES)

configure_file (include/thallium/config.hpp.in ${CMAKE_BINARY_DIR}/include/thallium/config.hpp)
