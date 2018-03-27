#
# CMakeLists.txt  cmake file for thallium library
# 13-Oct-2017  mdorier@anl.gov 
#

# list of source files
set(thallium-src bulk.cpp
		 callable_remote_procedure.cpp
		 endpoint.cpp 
                 engine.cpp 
		 margo_exception.cpp
		 pool.cpp
		 proc_buffer.cpp
                 remote_procedure.cpp 
		 request.cpp
		 remote_bulk.cpp
		 scheduler.cpp
		 task.cpp
		 thread.cpp)

# load package helper for generating cmake CONFIG packages
include (CMakePackageConfigHelpers)

# where to install files for "find_package"
set (thallium-pkg "share/cmake/thallium")

#
# library version set here (e.g. for shared libs).
#
set (THALLIUM_VERSION_MAJOR 1)
set (THALLIUM_VERSION_MINOR 0)
set (THALLIUM_VERSION_PATCH 0)
set (thallium-vers "${THALLIUM_VERSION_MAJOR}.${THALLIUM_VERSION_MINOR}")
set (THALLIUM_VERSION "${thallium-vers}.${THALLIUM_VERSION_PATCH}")

add_library(thallium ${thallium-src})
target_link_libraries (thallium mercury margo)
target_include_directories (thallium PUBLIC $<INSTALL_INTERFACE:include>)

# local include's BEFORE, in case old incompatable .hpp files in prefix/include
target_include_directories (thallium BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>)

# for shared libs, establish the lib version
set_target_properties (thallium
    PROPERTIES VERSION ${THALLIUM_VERSION}
    SOVERSION ${THALLIUM_VERSION_MAJOR})

#
# installation stuff (packaging and install commands)
#
write_basic_package_version_file(
    "thallium-config-version.cmake"
    VERSION ${THALLIUM_VERSION}
    COMPATIBILITY AnyNewerVersion)

# generate our config file for find_package()
configure_file (thallium-config.cmake.in thallium-config.cmake @ONLY)

#
# "make install" rules
#
install (TARGETS thallium EXPORT thallium-targets
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib)
install (EXPORT thallium-targets
         DESTINATION ${thallium-pkg}
         FILE "thallium-targets.cmake")
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/thallium-config.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/thallium-config-version.cmake"
               "../cmake/xpkg-import.cmake"
         DESTINATION ${thallium-pkg} )
install (DIRECTORY ../include/thallium
         DESTINATION include
         FILES_MATCHING PATTERN "*.hpp")
install (FILES ../include/thallium.hpp
	 DESTINATION include)
