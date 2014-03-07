# Toolchain for crosscompiling Win32 binaries using mingw64
#
# To build this on a linux system, run (in a new build directory):
# cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Windows-mingw32.cmake

set(CMAKE_SYSTEM_NAME Windows)

# Set the compilers needed
set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)

# Set the target environment
set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32 ${CMAKE_SOURCE_DIR}/win32)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

