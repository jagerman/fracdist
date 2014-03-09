cmake_minimum_required(VERSION 2.8)

project(fracdist CXX)

set(fracdist_VMAJ 1)
set(fracdist_VMIN 0)
set(fracdist_VPAT 0)
set(fracdist_description "fractional unit roots/cointegration pvalue and critical value finder")
set(fracdist_author "Jason Rhinelander <jason@imaginary.ca>")
set(fracdist_homepage "https://github.com/jagerman/fracdist")

set(fracdist_data_zip_basename mn-files.zip)
set(fracdist_data_zip "${CMAKE_CURRENT_BINARY_DIR}/data/${fracdist_data_zip_basename}")
set(fracdist_data_url "http://qed.econ.queensu.ca/jae/datasets/mackinnon004/${fracdist_data_zip_basename}")
set(fracdist_data_generated "${CMAKE_BINARY_DIR}/fracdist/data.cpp" "${CMAKE_BINARY_DIR}/fracdist/data.hpp")

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

message(STATUS "Downloading ${fracdist_data_url}...")
file(DOWNLOAD "${fracdist_data_url}" "${fracdist_data_zip}" STATUS zipdl_stat EXPECTED_MD5 "d9033e3f941aed5c7ccf5f5ce2756b79")

list(GET zipdl_stat 0 errcode)
if (errcode)
    list(GET zipdl_stat 1 errmsg)
    message(FATAL_ERROR "Unable to download data files from ${fracdist_data_url}: ${errmsg}")
endif()
message(STATUS "${fracdist_data_zip} downloaded.")

foreach(type c m)
    foreach (n RANGE 1 12)
        if (n LESS 10)
            set(n "0${n}")
        endif()
        list(APPEND fracdist_datafiles "${CMAKE_CURRENT_BINARY_DIR}/data/fr${type}app${n}.txt")
    endforeach()
endforeach()

add_custom_command(OUTPUT ${fracdist_datafiles}
    COMMAND ${CMAKE_COMMAND} -E tar xf "${fracdist_data_zip}"
    COMMAND ${CMAKE_COMMAND} -E remove "${CMAKE_CURRENT_BINARY_DIR}/data/fracdist.f" "${CMAKE_CURRENT_BINARY_DIR}/data/fracdist.f-win"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/data"
    COMMENT "Extracting data/frcapp*.txt, frmapp*.txt from ${fracdist_data_zip_basename}")

foreach(hpp fracdist/common.hpp fracdist/pvalue.hpp fracdist/critical.hpp)
    list(APPEND fracdist_headers "${CMAKE_CURRENT_SOURCE_DIR}/${hpp}")
endforeach()
foreach(cpp fracdist/pvalue.cpp fracdist/critical.cpp fracdist/common.cpp)
    list(APPEND fracdist_source "${CMAKE_CURRENT_SOURCE_DIR}/${cpp}")
endforeach()
set(fracdist_programs fdpval fdcrit)

add_custom_command(OUTPUT ${fracdist_data_generated}
    COMMAND ${PERL_EXECUTABLE} "-I${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/build-data.pl" "${CMAKE_BINARY_DIR}/data"
    DEPENDS build-data.pl DataParser.pm ${fracdist_datafiles}
    COMMENT "Generating fracdist/data.{cpp,hpp} from data/*.txt"
)
add_custom_target(data DEPENDS ${fracdist_data_generated})

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

add_library(fracdist SHARED ${fracdist_source} "${CMAKE_BINARY_DIR}/fracdist/data.cpp")

add_dependencies(fracdist data)

foreach(exec ${fracdist_programs})
    add_executable(${exec} ${exec}.cpp)
    target_link_libraries(${exec} fracdist)
endforeach()

# If fracdist_PACKAGE_DOCS is not set, include it only if doxygen is found
if (NOT DEFINED fracdist_PACKAGE_DOCS)
    find_package(Doxygen 1.8.2)
    if (DOXYGEN_FOUND)
        message(STATUS "doxygen found: enabling documentation generation")
        set(fracdist_PACKAGE_DOCS 1)
    else()
        message(STATUS "doxygen not found: documentation generation disabled")
    endif()
endif()

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
    set(fracdist_docdir "doc")
else()
    set(fracdist_docdir "share/doc/fracdist")
endif()

if (fracdist_PACKAGE_DOCS)
    add_subdirectory(doc)
else()
    add_subdirectory(doc EXCLUDE_FROM_ALL)
endif()


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
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${fracdist_author}")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${fracdist_homepage}")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${fracdist_description}")
endif()
include(CPack)

