# Building notes

This documents shows the steps used to compiled fracdist from source.

## Linux and similar

On a Linux or similar system, install GSL (including its headers; on
Debian/Ubuntu this means installing the libgsl0-dev package) and cmake, then
from the fracdist directory:

    mkdir build
    cd build
    cmake ..
    make

optionally followed by:

    make package

if you want installable .deb and .rpm files for the package, or else

    make install

to install directly to your system.

## Windows executables from a Linux system

Requirements:
- cmake
- [GSL source code](http://www.gnu.org/software/gsl)
- working MinGW build environment (e.g. the mingw-w64 virtual package on
  Debian/Ubuntu)

### Step 1: Create build directory

    mkdir build-win32

### Step 2: Compile GSL

Download and extract the GSL source code somewhere, and build it with:

    ./configure --prefix=$HOME/fracdist/build-win32/gsl --host=i686-w64-mingw32 --build="`config.guess`" --enable-shared --enable-static
    make
    make install

(change the --prefix=$HOME/fracdist part to wherever the fracdist source is
located).  You can add -j4 or -j8 to the 'make' command to make the compilation
use more CPUs (and thus go faster).

### Step 3: Compile fracdist

From the fracdist/build-win32 directory, invoke cmake and compile using:

    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Windows-mingw32.cmake ..
    make

which will give you the binaries and dlls.  To build this into a zip use:

    make package

will will put everything into a fracdist-x.y.z.zip file.
