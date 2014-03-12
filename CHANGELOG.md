# Major version changes (for change details, see https://github.com/jagerman/fracdist)

## 1.0.2

- Added versioning to libfracdist shared library.

## 1.0.1

- Initial release binary packages were installing headers in the wrong place
  (include/common.hpp, etc. instead of include/fracdist/common.hpp), and were
  missing the fracdist/data.hpp header.
- Added component configurations for Windows installers so that Windows users
  can elect to not install the headers and/or docs.

## 1.0.0

- Initial release.
