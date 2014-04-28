# fracdist

## Description

This is a library and pair of command-line programs to calculate p-values and
critical values for tests of fractional unit roots and cointegration.

This program is modelled on that described in James G. MacKinnon and Morten
Ørregaard Nielsen, "Numerical Distribution Functions of Fractional Unit Root
and Cointegration Tests," *Journal of Applied Econometrics*, Vol. 29, No. 1,
2014, pp.161-171.  The data from the data archive associated with the paper,
http://qed.econ.queensu.ca/jae/datasets/mackinnon004/, is automatically
downloaded when compiling this software.

This program consists of two command-line binaries: `fdpval` and `fdcrit`.  The
former calculates p-values for test statistics, the latter calculates critical
test statistic values from p-values.  Running each program without arguments
gives usage information for the programs.

The latest version of the source code of this library is available at
https://github.com/jagerman/fracdist.  Binary releases are available at
https://github.com/jagerman/fracdist/releases.

A generated version of the library API (which may be out of date) is available
at https://imaginary.ca/fracdist-api/.

## Requirements

Binary releases of this program have no special requirements, and are available
from https://github.com/jagerman/fracdist/releases.

Compiling this program requires CMake, perl, and a C++ compiler supporting the
C++11 standard (recent versions of clang and g++ will certainly work).  See
[BUILDING.md](BUILDING.md) for details.

## License

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see <http://www.gnu.org/licenses/>.

## Compiling

See [BUILDING.md](BUILDING.md).

## Changes

See [CHANGELOG.md](CHANGELOG.md).

## Author

[Jason Rhinelander](Jason Rhinelander) <jason@imaginary.ca>

## Copyright

© 2014 Jason Rhinelander <jason@imaginary.ca>
