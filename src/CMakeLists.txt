#
# CMakeLists.txt  cmake file for thallium library
# 13-Oct-2017  mdorier@anl.gov
#

# load package helper for generating cmake CONFIG packages
include (CMakePackageConfigHelpers)

# where to install files for "find_package"
set (thallium-pkg "share/cmake/thallium")

#
# library version set here (e.g. for shared libs).
#
set (THALLIUM_VERSION_MAJOR 0)
set (THALLIUM_VERSION_MINOR 14)
set (THALLIUM_VERSION_PATCH 0)
set (thallium-vers "${THALLIUM_VERSION_MAJOR}.${THALLIUM_VERSION_MINOR}")
set (THALLIUM_VERSION "${thallium-vers}.${THALLIUM_VERSION_PATCH}")

add_library (thallium INTERFACE)
target_link_libraries (thallium INTERFACE PkgConfig::margo cereal::cereal)
target_include_directories (thallium INTERFACE $<INSTALL_INTERFACE:include>)

# local include's BEFORE, in case old incompatable .hpp files in prefix/include
target_include_directories (thallium INTERFACE
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>)

#
# installation stuff (packaging and install commands)
#
set (INCLUDE_INSTALL_DIR include/)
set (LIB_INSTALL_DIR lib/)
include (CMakePackageConfigHelpers)
configure_package_config_file (thallium-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/thallium-config.cmake
  INSTALL_DESTINATION ${LIB_INSTALL_DIR}/thallium/cmake
  PATH_VARS INCLUDE_INSTALL_DIR)
write_basic_package_version_file (
    "thallium-config-version.cmake"
    VERSION ${THALLIUM_VERSION}
    COMPATIBILITY AnyNewerVersion)

# some bits for the pkg-config file
set (DEST_DIR "${CMAKE_INSTALL_PREFIX}")
CONFIGURE_FILE ("thallium.pc.in" "thallium.pc" @ONLY)

# Interface library that adds -DTHALLIUM_DEBUG_RPC_TYPES
add_library (thallium_check_types INTERFACE)
target_compile_definitions (thallium_check_types INTERFACE THALLIUM_DEBUG_RPC_TYPES)

#
# "make install" rules
#
install (TARGETS thallium thallium_check_types EXPORT thallium-targets
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib)
install (EXPORT thallium-targets
         DESTINATION ${thallium-pkg}
         FILE "thallium-targets.cmake")
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/thallium-config.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/thallium-config-version.cmake"
         DESTINATION ${thallium-pkg} )
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/thallium.pc"
         DESTINATION "lib/pkgconfig/")
install (DIRECTORY ../include/thallium
         DESTINATION include
         FILES_MATCHING PATTERN "*.hpp")
install (FILES ../include/thallium.hpp
         DESTINATION include)
