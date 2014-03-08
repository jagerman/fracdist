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

This program requires CMake and perl to compile.  It also requires a C++
compiler supporting the C++11 standard (recent versions of clang and g++
will certainly work).

It also uses Eigen3 and Boost, but includes copies of these libraries for
systems that do not have them (locally installed versions will be used instead
of the included versions, if found).

## Building

See [BUILDING.md](BUILDING.md).

## Author

Jason Rhinelander <jason@imaginary.ca>

## Copyright

Jason Rhinelander <jason@imaginary.ca> (application)

James MacKinnon & Morten Ørregaard Nielsen (data files)
