# fracdist

## Description

This is a library to calculate p-values and critical values for tests of
fractional unit roots and cointegration.

This program is modelled on that described in James G. MacKinnon and Morten
Ørregaard Nielsen, "Numerical Distribution Functions of Fractional Unit Root
and Cointegration Tests," *Journal of Applied Econometrics*, Vol. 29, No. 1,
2014, pp.161-171.  The data in this repository is exactly that from the data
archive associated with the paper,
http://qed.econ.queensu.ca/jae/datasets/mackinnon004/

## Requirements

This program requires the GNU Scientific Library, and requires perl and CMake
to compile.

## Building

From the project root, do:

$ mkdir build
$ cd build
$ cmake ..
$ make

If all goes well, you'll now have fdpval and fdcrit executables and a
libfracdist.so shared library.

## Author

Jason Rhinelander <jason@imaginary.ca>

## Copyright

Jason Rhinelander <jason@imaginary.ca> (application)

James MacKinnon & Morten Ørregaard Nielsen (data files)
