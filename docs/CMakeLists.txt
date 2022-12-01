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

#
# In case of GitHub docs we only need a configuration file to be 
# generated, so we quit here in this case
#
if (NTP_BUILD_GH_DOCS)
    message(NOTICE "[${CMAKE_PROJECT_NAME}] Doxyfile is generated, but NTP_BUILD_GH_DOCS is set, nothing else will be performed")
    message(NOTICE "[${CMAKE_PROJECT_NAME}] Doxyfile is located here: '${NTP_DOCS_DOXYGEN_OUT}'")
    return()
endif (NTP_BUILD_GH_DOCS)

#
# Otherwise we need to find Doxygen
#
find_package(Doxygen)

#
# Check if we have Doxygen installed
#
if (NOT DOXYGEN_FOUND )
    message(WARNING "[${CMAKE_PROJECT_NAME}] Doxygen need to be installed to generate the doxygen documentation")
    return()
endif() 

#
# Custom target for building documentation
#
add_custom_target(ntp_docs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${NTP_DOCS_DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating documentation with Doxygen"
    VERBATIM)