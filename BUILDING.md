# Building notes

This documents shows the steps used to compiled fracdist from source.

## Linux, OS X, and similar

From the fracdist directory:

    mkdir build
    cd build
    cmake ..
    make -j4

You can install directly to the system (under /usr/local) using:

    make install

or alternatively build a .deb and .rpm package to install using one of:

    cpack -G DEB
    cpack -G RPM

followed by an appropriate package command to install the package (for example,
on Debian/Ubuntu: dpkg -i fracdist-x.y.z-amd64.deb).

You can also use either of:

    make package
    cpack

to build both the .deb and .rpm.

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

which will give you the binaries and dlls.  To build this into a zip use:

    cpack -G ZIP

or, for an exe installer:

    cpack -G NSIS

(Requires that nsis be installed).

You can build both at once using either:

    make package
    cpack
