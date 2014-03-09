
# Need at least version 1.8.2 (as that was when c++11 support was added to doxygen)
find_package(Doxygen 1.8.2 REQUIRED)

if(NOT DOXYGEN_DOT_FOUND)
    message(FATAL_ERROR "Could NOT find dot (required by doxygen)")
endif()

add_custom_target(doc)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/DoxygenLayout.xml.in ${CMAKE_CURRENT_BINARY_DIR}/DoxygenLayout.xml @ONLY)

# Use a custom command to output doxygen's generated index file.  Doing it this way ensures
# that 'make doc' only runs doxygen when something in src or include has changed.
add_custom_command(OUTPUT "api/index.xhtml"
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    DEPENDS "Doxyfile.in" ${fracdist_headers} ${fracdist_source} data 
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating API documentation (using doxygen)"
    )

add_custom_target(api_doc DEPENDS "api/index.xhtml")

set_property(DIRECTORY APPEND PROPERTY
    ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_CURRENT_BINARY_DIR}/api")
#set_property(DIRECTORY APPEND PROPERTY
#    ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_CURRENT_BINARY_DIR}/api-latex")

add_dependencies(api_doc data)

add_dependencies(doc api_doc)

install(DIRECTORY api
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    COMPONENT doc)

