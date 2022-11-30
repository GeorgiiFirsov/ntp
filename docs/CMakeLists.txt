#
# Firstly we need to find Doxygen
#
find_package(Doxygen)

#
# Check if we have Doxygen installed
#
if (NOT DOXYGEN_FOUND)
    message(WARNING "[${CMAKE_PROJECT_NAME}] Doxygen need to be installed to generate the doxygen documentation")
    return()
endif() 

#
# Setup variables
#
set(NTP_DOCS_ROOT        ${NTP_ROOT}/docs)
set(NTP_DOCS_STYLES_ROOT ${NTP_DOCS_ROOT}/doxygen-awesome-css)
set(NTP_DOCS_DOXYGEN_IN  ${NTP_DOCS_ROOT}/Doxyfile.in)
set(NTP_DOCS_DOXYGEN_OUT ${NTP_DOCS_ROOT}/Doxyfile)

#
# And here we go...
#
message("[${CMAKE_PROJECT_NAME}] Configuring doxygen...")

configure_file(${NTP_DOCS_DOXYGEN_IN} ${NTP_DOCS_DOXYGEN_OUT} @ONLY)
add_custom_target(ntp_docs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${NTP_DOCS_DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating documentation with Doxygen"
    VERBATIM)