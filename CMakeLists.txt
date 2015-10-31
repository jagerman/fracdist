cmake_minimum_required(VERSION 2.8)

project(fracdist CXX)

set(fracdist_VMAJ 1)
set(fracdist_VMIN 0)
set(fracdist_VPAT 4)
set(fracdist_description "fractional unit roots/cointegration pvalue and critical value finder")
set(fracdist_author "Jason Rhinelander <jason@imaginary.ca>")
set(fracdist_homepage "https://github.com/jagerman/fracdist")

cmake_policy(SET CMP0046 OLD)
cmake_policy(SET CMP0042 NEW)

# fracdist library version (CURRENT.REVISION.AGE), which is totally separate
# from the above.  See http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
# Basic rules for updating these when releasing a new fracdist version:
# - If the new release has an API change:
#   - CURRENT gets updated
#   - REVISION gets set to 0
#   - If the API change was purely an addition, increment AGE
#   - Otherwise (i.e. an API change or removal), reset AGE to 0
# - Otherwise, if the release has no API change but has code changes:
#   - REVISION gets incremented
#   - (CURRENT and AGE stay the same)
# If there is no code change (e.g. the release is just a documentation update)
# then none of these change.
#
# (So something like 3.7.1 indicates the 8th revision of the libfracdist.so.3
# interface, and that code that links against libfracdist.so.2 can safely link
# against this version, but code that links against libfracdist.so.1 cannot.
set(libfracdist_CURRENT  "0")
set(libfracdist_AGE      "0")
set(libfracdist_REVISION "0")

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
    # Link to libgcc/libstdc++ statically (so that we don't need to carry around the extra DLLs)
    set(CMAKE_EXE_LINKER_FLAGS "-static -Wl,--allow-multiple-definition")
    set(CMAKE_SHARED_LINKER_FLAGS "-static")
    add_definitions(-DBOOST_DISABLE_THREADS)
endif()

message(STATUS "Downloading ${fracdist_data_url}...")
file(DOWNLOAD "${fracdist_data_url}" "${fracdist_data_zip}" STATUS zipdl_stat)

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

foreach(hpp fracdist/common.hpp fracdist/pvalue.hpp fracdist/critical.hpp fracdist/version.hpp)
    list(APPEND fracdist_headers "${CMAKE_CURRENT_SOURCE_DIR}/${hpp}")
endforeach()
list(APPEND fracdist_headers "${CMAKE_CURRENT_BINARY_DIR}/fracdist/data.hpp")
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

configure_file("${CMAKE_SOURCE_DIR}/fracdist/version.cpp.in" "${CMAKE_BINARY_DIR}/fracdist/version.cpp")
list(APPEND fracdist_source "${CMAKE_BINARY_DIR}/fracdist/version.cpp")

option(EIGEN_FORCE_LOCAL "Force use of local copy of Eigen" OFF)
# Try to find Eigen3 on the system first; if that fails, use the included copy
if (NOT EIGEN_FORCE_LOCAL)
    find_package(Eigen3)
endif()
if (NOT EIGEN3_FOUND)
    message(STATUS "Eigen3 not found on system; using bundled copy")
    set(EIGEN3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/eigen3")
    find_package(Eigen3 REQUIRED)
endif()
message(STATUS "Eigen version: ${EIGEN3_VERSION}")
include_directories(${EIGEN3_INCLUDE_DIR})

option(BOOST_FORCE_LOCAL "Force use of local copy of Boost" OFF)
# Try to find Boost on the system first; if that fails, use the included boost subset
if (NOT BOOST_FORCE_LOCAL)
    find_package(Boost)
endif()
if (NOT Boost_FOUND)
    message(STATUS "Boost not found on system; using bundled copy")
    set(Boost_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/boost")
    find_package(Boost REQUIRED)
endif()
include_directories(${Boost_INCLUDE_DIRS})

add_library(fracdist SHARED ${fracdist_source} "${CMAKE_BINARY_DIR}/fracdist/data.cpp")
set_target_properties(fracdist PROPERTIES
    VERSION "${libfracdist_CURRENT}.${libfracdist_AGE}.${libfracdist_REVISION}"
    SOVERSION "${libfracdist_CURRENT}"
)

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
        message(WARNING "doxygen not found: documentation generation and packaging disabled")
    endif()
endif()

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
    set(fracdist_docdir "doc")
else()
    set(fracdist_docdir "share/doc/fracdist")
endif()

if (DEFINED fracdist_PACKAGE_DOCS)
    add_subdirectory(doc)
endif()


set(CPACK_PACKAGE_VERSION_MAJOR ${fracdist_VMAJ})
set(CPACK_PACKAGE_VERSION_MINOR ${fracdist_VMIN})
set(CPACK_PACKAGE_VERSION_PATCH ${fracdist_VPAT})

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")


install(FILES ${fracdist_headers} DESTINATION include/fracdist COMPONENT headers)

install(FILES README.md CHANGELOG.md DESTINATION "${fracdist_docdir}" COMPONENT docs)

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
    # Windows binaries and lib go in the top-level dir:
    install(TARGETS ${fracdist_programs} COMPONENT binaries DESTINATION .)
    install(TARGETS fracdist COMPONENT library DESTINATION .)

    set(CPACK_GENERATOR ZIP NSIS)
    set(CPACK_PACKAGE_FILE_NAME "fracdist-${fracdist_VMAJ}.${fracdist_VMIN}.${fracdist_VPAT}-windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    # This is a bit gross, but basically we'll just stick the binaries/so in the top-level:
    install(TARGETS ${fracdist_programs} COMPONENT binaries DESTINATION .)
    install(TARGETS fracdist COMPONENT library DESTINATION .)
    set(CPACK_GENERATOR ZIP)
    set(CPACK_PACKAGE_FILE_NAME "fracdist-${fracdist_VMAJ}.${fracdist_VMIN}.${fracdist_VPAT}-macosx")
else()
    install(TARGETS fracdist COMPONENT library DESTINATION lib)
    install(TARGETS ${fracdist_programs} COMPONENT binaries DESTINATION bin)

    set(CPACK_GENERATOR DEB RPM)

    set(arch ${CMAKE_HOST_SYSTEM_PROCESSOR})
    if (arch STREQUAL x86_64)
        # Check that we aren't using a 32-bit userspace
        execute_process(COMMAND perl -MConfig -e "print \$Config{longsize}" OUTPUT_VARIABLE longsize)
        if (longsize EQUAL 4)
            set(arch i386)
        else()
            set(arch amd64)
        endif()
    elseif(arch STREQUAL x86)
        set(arch i386)
    elseif(arch MATCHES "i[3-6]86")
        set(arch i386)
    endif()
    message(STATUS "arch = ${arch}")

    set(CPACK_PACKAGE_FILE_NAME "fracdist-${fracdist_VMAJ}.${fracdist_VMIN}.${fracdist_VPAT}-${arch}")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${fracdist_author}")
    #set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${fracdist_homepage}")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${fracdist_description}")
endif()

set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)

include(CPack)

cpack_add_component(library REQUIRED DISPLAY_NAME "libfracdist" DESCRIPTION "The main fracdist library code.")
cpack_add_component(binaries DISPLAY_NAME "Binaries" DESCRIPTION "'fdpval' and 'fdcrit' provide a simple command-line interface to libfracdist functionality.")
cpack_add_component(headers DISABLED DISPLAY_NAME "C++ headers" DESCRIPTION "C++ headers needed to compile code linking to libfracdist.")
cpack_add_component(docs DISABLED DISPLAY_NAME "Documentation" DESCRIPTION "API documentation for libfracdist programming.")

