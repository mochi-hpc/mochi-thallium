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

cmake_minimum_required (VERSION 3.10)
project (thallium C CXX)
enable_testing ()

set (CMAKE_CXX_STANDARD 14)

# setup cache variables for ccmake
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release
         CACHE STRING "Choose the type of build." FORCE)
    set_property (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                  "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif ()

set (THALLIUM_VERSION_MAJOR 0)
set (THALLIUM_VERSION_MINOR 15)
set (THALLIUM_VERSION_PATCH 1)
set (thallium-vers "${THALLIUM_VERSION_MAJOR}.${THALLIUM_VERSION_MINOR}")
set (THALLIUM_VERSION "${thallium-vers}.${THALLIUM_VERSION_PATCH}")
math (EXPR THALLIUM_VERSION_NUM "${THALLIUM_VERSION_MAJOR}*1000000 + ${THALLIUM_VERSION_MINOR}*1000 + ${THALLIUM_VERSION_PATCH}")

option (ENABLE_TESTS "Enable tests" OFF)
option (ENABLE_EXAMPLES "Enable examples" OFF)
option (ENABLE_COVERAGE "Enable code coverage" OFF)

include_directories (${CMAKE_BINARY_DIR}/include)

if (ENABLE_COVERAGE)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -O0 -fno-inline -fno-inline-small-functions -fno-default-inline")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage -lgcov")
endif ()

find_package (cereal CONFIG REQUIRED)
get_target_property (CEREAL_INC cereal::cereal INTERFACE_INCLUDE_DIRECTORIES)

find_package (PkgConfig REQUIRED)
pkg_check_modules (margo REQUIRED IMPORTED_TARGET margo)

add_subdirectory (src)
if (ENABLE_TESTS)
    add_subdirectory (tests)
endif (ENABLE_TESTS)
if (ENABLE_EXAMPLES)
    add_subdirectory (examples)
endif (ENABLE_EXAMPLES)

configure_file (include/thallium/config.hpp.in ${CMAKE_BINARY_DIR}/include/thallium/config.hpp @ONLY)
