
# Need at least version 1.8.2 (as that was when c++11 support was added to doxygen)
find_package(Doxygen 1.8.2 REQUIRED)

if(NOT DOXYGEN_DOT_FOUND)
    message(FATAL_ERROR "Could NOT find dot (required for doxygen documentation generation)")
endif()

# Can set NO_MATHJAX to true to skip looking for the system MathJax, in which
# case the default (MathJax via public CDN) will be used by doxygen.
set(mathjax_usable_path "")
if (MATHJAX_PATH)
    set(mathjax_usable_path "${MATHJAX_PATH}")
elseif (NOT NO_MATHJAX)
    find_package(MathJax 2.3)
    if(MATHJAX_FOUND)
        set(mathjax_usable_path "${MATHJAX_PATH}")
    else()
        message(STATUS "System MathJax not found: using public MathJax CDN instead.  If MathJax is installed, try setting MATHJAX_ROOT")
    endif()
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

# Use a custom command to output doxygen's generated index file.  Doing it this way ensures
# that 'make api_doc' only runs doxygen when something in src or include has changed.
add_custom_command(OUTPUT "api/index.html"
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    DEPENDS "Doxyfile.in" ${fracdist_headers} ${fracdist_source} data ${fracdist_data_generated} 
        ${CMAKE_SOURCE_DIR}/README.md ${CMAKE_SOURCE_DIR}/CHANGELOG.md
        ${CMAKE_SOURCE_DIR}/BUILDING.md
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating API documentation (using doxygen)"
    )

add_custom_target(api_doc ALL DEPENDS "api/index.html" data)

set_property(DIRECTORY APPEND PROPERTY
    ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_CURRENT_BINARY_DIR}/api")

add_dependencies(api_doc ${fracdist_data_generated})

install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/api"
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
    COMPONENT docs)
