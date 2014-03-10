
# Need at least version 1.8.2 (as that was when c++11 support was added to doxygen)
find_package(Doxygen 1.8.2 REQUIRED)

if(NOT DOXYGEN_DOT_FOUND)
    message(FATAL_ERROR "Could NOT find dot (required by doxygen)")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

# Use a custom command to output doxygen's generated index file.  Doing it this way ensures
# that 'make doc' only runs doxygen when something in src or include has changed.
add_custom_command(OUTPUT "api/index.html"
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    DEPENDS "Doxyfile.in" ${fracdist_headers} ${fracdist_source} data ${fracdist_data_generated} 
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating API documentation (using doxygen)"
    )

add_custom_target(api_doc ALL DEPENDS "api/index.html" data)

set_property(DIRECTORY APPEND PROPERTY
    ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_CURRENT_BINARY_DIR}/api")

add_dependencies(api_doc ${fracdist_data_generated})

install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/api"
    DESTINATION "${fracdist_docdir}"
    COMPONENT docs)
