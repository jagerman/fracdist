# Major version changes (for change details, see https://github.com/jagerman/fracdist)

## 1.0.3

- Added --linear flag to fdpval and fdcrit that uses linear B interpolation
  between the two nearest dataset B values instead of the 5-9 nearest points
  for a quadratic interpolation.
- Added version and licence info to fdpval and fdcrit output
- Added version variables to library (in fracdist/version.hpp, .cpp)
- Removed md5sum check from mn-files.zip download; it broke every time the data
  set download changed (from the recent fracdist.f fixes), making the package
  unbuildable.

## 1.0.2

- Added versioning to libfracdist shared library.
- Improved numerical stability of matrix inversion used for quadratic
  approximations regressions.  This improves results significantly for large
  critical values (which occur, for example, with larger q values q=7).

## 1.0.1

- Initial release binary packages were installing headers in the wrong place
  (include/common.hpp, etc. instead of include/fracdist/common.hpp), and were
  missing the fracdist/data.hpp header.
- Added component configurations for Windows installers so that Windows users
  can elect to not install the headers and/or docs.

## 1.0.0

- Initial release.
