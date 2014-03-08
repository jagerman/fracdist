# Building notes

This documents shows the steps used to compiled fracdist from source.

## Linux, OS X, and similar

From the fracdist directory:

    mkdir build
    cd build
    cmake ..
    make

You can install directly to the system (under /usr/local) using:

    make install

or alternatively build a .deb or .rpm package to install using:

    make package

followed by an appropriate package command (Such as, on Debian/Ubuntu: dpkg -i
fracdist-x.y.z-amd64.deb).

## Windows executables (built on a Linux system)

Requirements:
- cmake
- working MinGW build environment (e.g. the mingw-w64 virtual package on
  Debian/Ubuntu)
- nsis (only if building a installable package)

The procedure is essentially the same as the above, except for the initial
cmake command:

    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Windows-mingw32.cmake ..
    make

which will give you the binaries and dlls.  To build this into a zip use:

    make package

will will put everything into a fracdist-x.y.z.zip file.
