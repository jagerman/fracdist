cmake_minimum_required(VERSION 2.8)

project(fracdist CXX)

set(fracdist_VMAJ 1)
set(fracdist_VMIN 1)
set(fracdist_VPAT 0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)
find_package(Perl REQUIRED)

# No in-source building
include(MacroEnsureOutOfSourceBuild)
macro_ensure_out_of_source_build("${PROJECT_NAME} requires an out-of-source build.  Create a build directory and run 'cmake ${CMAKE_SOURCE_DIR} [options]'.")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "-O3 ${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra ${CMAKE_CXX_FLAGS_DEBUG} -O0")

if (MINGW)
    #add_definitions(-static-libgcc -static-libstdc++)
    # Link to libgcc/libstdc++ statically (so that we don't need to carry around the extra DLLs)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
    #    set(LINK_FLAGS "${LINK_FLAGS} -static-libgcc -static-libstdc++")
    #    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_EXE_LINKER_FLAGS "-static -Wl,--allow-multiple-definition")
    set(CMAKE_SHARED_LINKER_FLAGS "-static")
    add_definitions(-DBOOST_DISABLE_THREADS)
endif()


file(GLOB fracdist_datafiles data/*.txt)
set(fracdist_headers fracdist/common.hpp fracdist/pvalue.hpp fracdist/critical.hpp)
set(fracdist_header_data "${CMAKE_BINARY_DIR}/fracdist/data.hpp")
set(fracdist_source fracdist/pvalue.cpp fracdist/critical.cpp fracdist/common.cpp fracdist/data.cpp)
set(fracdist_programs fdpval fdcrit)

add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/fracdist/data.cpp" "${CMAKE_BINARY_DIR}/fracdist/data.hpp"
    COMMAND ${PERL_EXECUTABLE} "-I${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/build-data.pl" "${CMAKE_SOURCE_DIR}/data"
    DEPENDS build-data.pl DataParser.pm ${fracdist_datafiles}
)
add_custom_target(data DEPENDS
    "${CMAKE_BINARY_DIR}/fracdist/data.cpp" "${CMAKE_BINARY_DIR}/fracdist/data.hpp")

# Try to find Eigen3 on the system first; if that fails, use the included copy
find_package(Eigen3)
if (NOT EIGEN3_FOUND)
    message(STATUS "Eigen3 not found on system; using bundled copy")
    set(EIGEN3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/eigen3")
    find_package(Eigen3 REQUIRED)
endif()
include_directories(${EIGEN3_INCLUDE_DIR})

# Try to find Boost on the system first; if that fails, use the included boost subset
find_package(Boost)
if (NOT Boost_FOUND)
    message(STATUS "Boost not found on system; using bundled copy")
    set(Boost_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/boost")
    find_package(Boost REQUIRED)
endif()
include_directories(${Boost_INCLUDE_DIRS})

add_library(fracdist SHARED ${fracdist_source})

add_dependencies(fracdist data)

foreach(exec ${fracdist_programs})
    add_executable(${exec} ${exec}.cpp)
    target_link_libraries(${exec} fracdist)
endforeach()

set(CPACK_PACKAGE_VERSION_MAJOR ${fracdist_VMAJ})
set(CPACK_PACKAGE_VERSION_MINOR ${fracdist_VMIN})
set(CPACK_PACKAGE_VERSION_PATCH ${fracdist_VPAT})

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")


if (CMAKE_SYSTEM_NAME STREQUAL Windows)
    # Windows users are scared of version numbers:
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "fracdist")

    install(TARGETS fracdist ${fracdist_programs} DESTINATION .)
    install(
        FILES ${fracdist_headers} ${fracdist_header_data}
        DESTINATION include/fracdist)

    set(CPACK_GENERATOR ZIP NSIS)
    set(CPACK_PACKAGE_FILE_NAME "fracdist-${fracdist_VMAJ}.${fracdist_VMIN}.${fracdist_VPAT}-windows")
else()
    install(TARGETS fracdist ${fracdist_programs} LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
    install(FILES ${fracdist_headers} DESTINATION include)

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
