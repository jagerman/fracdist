# Building notes

This documents shows the steps used to compiled fracdist from source.

## Requirements

Compiling this program requires a minimum of CMake, perl, and a C++ compiler
supporting the C++11 standard (recent versions of clang and g++ will certainly
work).

Compiling also *optionally* requires `doxygen` and `dot` (from the `graphviz`
package) for generating API HTML documentation--the documentation won't be
generated if the packages are not installed.  Additionally, if the system has
an installation of the Eigen3 and boost headers they will be used; if not,
internal copies of the required headers will be used.  If you want to generate
the Windows installer, you'll also need the `nsis` package to be installed.

## Compiling on a debian-derived system

The easiest way to compile and generate debs on a Debian-derived system is to
use the "debian" branch of the repository, which contains the debian packaging
files.  From this branch you can use:

    dpkg-buildpackage -uc -us -b

to generate the .debs.

## Linux, OS X, and similar

To compile on a unix-like system, from the fracdist directory do:

    mkdir build
    cd build
    cmake ..
    make

You can install directly to the system (usually under /usr/local) using:

    make install

## Windows executables (built on a Linux system using mingw)

Requirements:
- cmake
- working MinGW build environment (e.g. the mingw-w64 virtual package on
  Debian/Ubuntu)
- nsis (only if building a installable package)

The procedure is essentially the same as the above, except for the initial
cmake command:

    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Windows-mingw32.cmake ..
    make -j4

which will build the binaries and dlls.  To build this into a zip use:

    cpack -G ZIP

or, for an exe installer:

    cpack -G NSIS

(Requires that the `nsis` package be installed).

You can build both at once using either:

    make package
    cpack
