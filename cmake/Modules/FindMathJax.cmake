# Tries to find MathJax.js.  Once done this will define:
#  MATHJAX_FOUND - System has MathJax
#  MATHJAX_PATH - the directory containing MathJax.js
#  MATHJAX_VERSION - the version of MathJax.js
#
# The following will be searched, in order of precedence:
# - MATHJAX_ROOT cmake variable
# - MATHJAX_ROOT environment variable
# - ${CMAKE_PREFIX_PATH}/share/javascript/mathjax
# - /usr/share/javascript/mathjax
#

include(FindPackageHandleStandardArgs)

find_path(MATHJAX_PATH MathJax.js
    PATHS ${MATHJAX_ROOT}
        $ENV{MATHJAX_ROOT}
        "${CMAKE_PREFIX_PATH}/share/javascript/mathjax"
        "${CMAKE_INSTALL_DATADIR}/javascript/mathjax"
        "/usr/share/javascript/mathjax"
    DOC "Root path of MathJax.js")

# Extract the version number from MathJax.js, which may or may not be minified.
if (NOT MATHJAX_PATH STREQUAL MATHJAX_PATH-NOTFOUND)
    file(READ "${MATHJAX_PATH}/MathJax.js" _mathjax_js_source)
    string(REGEX MATCH "MathJax\\.version *= *\"[0-9]+\\.[0-9]+\";" _mathjax_js_vers "${_mathjax_js_source}")
    unset(_mathjax_js_source)
    string(REGEX REPLACE ".*([0-9]+\\.[0-9]+).*" "\\1" MATHJAX_VERSION "${_mathjax_js_vers}")
    unset(_mathjax_js_vers)
endif()

find_package_handle_standard_args(MathJax
    REQUIRED_VARS MATHJAX_PATH
    VERSION_VAR MATHJAX_VERSION
    )

mark_as_advanced(MATHJAX_PATH)

