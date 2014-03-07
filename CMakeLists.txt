cmake_minimum_required(VERSION 2.8)

project(fracdist C)

set(fracdist_VMAJ 1)
set(fracdist_VMIN 0)
set(fracdist_VPAT 0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)
find_package(Perl REQUIRED)

# No in-source building
include(MacroEnsureOutOfSourceBuild)
macro_ensure_out_of_source_build("${PROJECT_NAME} requires an out-of-source build.  Create a build directory and run 'cmake ${CMAKE_SOURCE_DIR} [options]'.")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_C_FLAGS "-O3 -Wall -Wextra ${CMAKE_C_FLAGS} -std=c99")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")

file(GLOB fracdist_datafiles data/*.txt)

add_custom_command(OUTPUT fracdist-data.c fracdist-data.h
    COMMAND ${PERL_EXECUTABLE} "-I${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/build-data.pl" "${CMAKE_SOURCE_DIR}/data"
    DEPENDS build-data.pl DataParser.pm ${fracdist_datafiles}
)


if (CMAKE_SYSTEM_NAME STREQUAL Windows)
    set(GSL_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/gsl/include")
    set(GSL_LIBRARY_DIRS "${CMAKE_BINARY_DIR}/gsl/lib")
    set(GSL_LIBRARIES -l:gsl/bin/libgsl-0.dll -l:gsl/bin/libgslcblas-0.dll -lm)
    file(COPY "${CMAKE_BINARY_DIR}/gsl/bin/libgsl-0.dll" "${CMAKE_BINARY_DIR}/gsl/bin/libgslcblas-0.dll" DESTINATION .)
else()
    find_package(GSL REQUIRED)
endif()

include_directories(${GSL_INCLUDE_DIRS})
link_directories(${GSL_LIBRARY_DIRS})
add_definitions(-DHAVE_INLINE)

add_library(fracdist SHARED fracdist.c fracdist-data.c)
target_link_libraries(fracdist ${GSL_LIBRARIES})

set(fd_execs fdpval fdcrit)
foreach(exec ${fd_execs})
    add_executable(${exec} ${exec}.c)
    target_link_libraries(${exec} fracdist)
    target_link_libraries(${exec} ${GSL_LIBRARIES})
endforeach()

set(CPACK_PACKAGE_VERSION_MAJOR ${fracdist_VMAJ})
set(CPACK_PACKAGE_VERSION_MINOR ${fracdist_VMIN})
set(CPACK_PACKAGE_VERSION_PATCH ${fracdist_VPAT})

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
    # Windows users are scared of version numbers:
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "fracdist")

    install(TARGETS fracdist ${fd_execs} DESTINATION .)
    install(
        FILES fracdist.h "${CMAKE_BINARY_DIR}/fracdist-data.h"
        "${CMAKE_BINARY_DIR}/gsl/bin/libgsl-0.dll" "${CMAKE_BINARY_DIR}/gsl/bin/libgslcblas-0.dll"
        DESTINATION .)

    set(CPACK_GENERATOR ZIP NSIS)
    set(CPACK_PACKAGE_FILE_NAME "fracdist-${fracdist_VMAJ}.${fracdist_VMIN}.${fracdist_VPAT}-windows")
else()
    install(TARGETS fracdist ${fd_execs} LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
    install(FILES fracdist.h DESTINATION include)
    install(FILES "${CMAKE_BINARY_DIR}/fracdist-data.h" DESTINATION include)

    set(CPACK_GENERATOR DEB RPM)

    set(arch ${CMAKE_HOST_SYSTEM_PROCESSOR})
    if (arch STREQUAL x86_64)
        set(arch amd64)
    endif()
    set(CPACK_PACKAGE_FILE_NAME "fracdist-${fracdist_VMAJ}.${fracdist_VMIN}.${fracdist_VPAT}-${arch}")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jason Rhinelander <jason@imaginary.ca>")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/jagerman/fracdist")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "fractional unit roots/cointegration pvalue and critical value finder")
endif()
include(CPack)
