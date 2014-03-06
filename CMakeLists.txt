cmake_minimum_required(VERSION 2.8)

project(fracdist C)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)
find_package(Perl REQUIRED)

find_package(GSL REQUIRED)

include_directories(${GSL_INCLUDE_DIRS})
link_directories(${GSL_LIBRARY_DIRS})

add_definitions(-DHAVE_INLINE)

# No in-source building
include(MacroEnsureOutOfSourceBuild)
macro_ensure_out_of_source_build("${PROJECT_NAME} requires an out-of-source build.  Create a build directory and run 'cmake ${CMAKE_SOURCE_DIR} [options]'.")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_C_FLAGS "-O3 -Wall -Wextra ${CMAKE_CXX_FLAGS} -std=c99")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

file(GLOB fracdist_datafiles data/*.txt)

add_custom_command(OUTPUT fracdist-data.c fracdist-data.h
    COMMAND ${PERL_EXECUTABLE} "-I${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/build-data.pl" "${CMAKE_SOURCE_DIR}/data"
    DEPENDS build-data.pl DataParser.pm ${fracdist_datafiles}
)

add_library(fracdist SHARED fracdist.c fracdist-data.c)

foreach(exec fdpval fdcrit)
    add_executable(${exec} ${exec}.c)
    target_link_libraries(${exec} fracdist)
    target_link_libraries(${exec} ${GSL_LIBRARIES})
endforeach()

install(TARGETS fracdist LIBRARY DESTINATION lib)
install(TARGETS fdpval fdcrit RUNTIME DESTINATION bin)
install(FILES fracdist-data.h fracdist.h DESTINATION include)
