# ---------------------------------------------------------------------------
# Author: LennartG 
# Copyright:   2018
# License:     GPL v3+
# ---------------------------------------------------------------------------
# Frontend2  v1.0.139.0   Author: Jon Gough 
# ---------------------------------------------------------------------------
## ----- When changing this file do NOT change the order in which sections occur        ----- ##
## ----- Changes should only be made between the section blocks that identify where     ----- ##
## ----- these changes should be. The whole configuration process relies on this        ----- ##
## ----- sequence to be successful                                                      ----- ##
##                                                                                      ----- ##
##----- Modify section below to include all the details for your plugin                 ----- ##

set(CMLOC "CMakeLists: ")

# define minimum cmake version
cmake_minimum_required(VERSION 3.1.1)
if(COMMAND cmake_policy)
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif(POLICY CMP0043)
    cmake_policy(SET CMP0048 NEW)
    if(POLICY CMP0077)
        cmake_policy(SET CMP0077 NEW)
    endif(POLICY CMP0077)
endif(COMMAND cmake_policy)

# define plugin name, owner and versions
set(VERBOSE_NAME "GPS_Odometer")
set(COMMON_NAME "GPS Odometer")
set(TITLE_NAME "GPSODOMETER")
set(PACKAGE_CONTACT "LennartG")
set(PACKAGE "gpsodometer")
set(SHORT_DESCRIPTION "GPS Odometer plugin for OpenCPN")
set(LONG_DESCRIPTION "GPS controlled Dashboard based Odometer plugin for OpenCPN, displays GPS calculated Log and Trip information")

set(VERSION_MAJOR "0")
set(VERSION_MINOR "3")
set(VERSION_PATCH "1")
set(VERSION_TWEAK "6")
set(VERSION_DATE "23/11/2020")
set(OCPN_MIN_VERSION "ov50")
set(OCPN_API_VERSION_MAJOR "1")
set(OCPN_API_VERSION_MINOR "16")

set(PARENT "opencpn")

# The next line allows setup of a local webserver with git for testing purposes.
# The default is shown below, but it is done in cmake/pluginconfigure.cmake now.
# This default is needed to build locally and to build flatpak.
set(GIT_REPOSITORY_SERVER "github.com")

# Specifies Cloudsmith upload repository suffix for each catalog
set(PROD "prod")    #Standard Repos
set(BETA "beta")    #Standard Repos
set(ALPHA "alpha")  #Standard Repos

# Match the cloudsmith org name, not the user.
set(CLOUDSMITH_USER "opencpn") 
#set(CLOUDSMITH_USER "LennartG-Sve")

# Set if yourCloudsmith Base Repository name does not match your Git Repository name.
set (CLOUDSMITH_BASE_REPOSITORY "gps-odometer")  #without the pi

## Preliminary, not yet applicable
set(XML_INFO_URL "https://opencpn.org/wiki/dokuwiki/doku.php?id=opencpn:opencpn_user_manual:plugins:other:odometer") 
set(XML_SUMMARY ${SHORT_DESCRIPTION})
set(XML_DESCRIPTION ${LONG_DESCRIPTION})

##
## ----- Modify section above to include all the details for your plugin ----- ##
## ----- Modify section below if there are special requirements for the plugin ----- ##
##

## GPS Odometer uses SVG graphics for the toolbar icons
option(PLUGIN_USE_SVG "Use SVG graphics" ON)

set(CMAKE_CXX_STANDARD 11)

# Prefer libGL.so to libOpenGL.so, see CMP0072
set(OpenGL_GL_PREFERENCE "LEGACY")
# Don't use local version of GLU library
set(USE_LOCAL_GLU FALSE)
option(USE_GL "Enable OpenGL support" OFF)
message(STATUS "${CMLOC}USE_GL: ${USE_GL}")

# Define the build type
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE
        "Release"
        CACHE STRING "Choose the type of build" FORCE)
endif("${CMAKE_BUILD_TYPE}" STREQUAL "")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "" "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
message(STATUS "${CMLOC}Build type: ${CMAKE_BUILD_TYPE}")

## ----- The statements below are used to setup standard variables that are required by the 
##       CMAKE process - do not remove ----- ##

project(${PACKAGE})
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;")

## ----- Next line is used to setup standard variables that are required by the 
##       CMAKE process - do not remove ----- ##
include("PluginSetup")

## ----- Modify section below if there are special requirements for the plugin ----- ##
set(CMAKE_VERBOSE_MAKEFILE ON)
option(Plugin_CXX11 "Use c++11" OFF)

## ----- Do not change next section - needed to configure build process ----- ##
include("PluginConfigure")

## ----- Change below to match project requirements for source, headers, etc. ----- ##
## add_definitions(-DUSE_S57)


SET(SRCS
    src/odometer_pi.cpp
    src/iirfilter.cpp
    src/instrument.cpp
    src/button.cpp
    src/dial.cpp
    src/speedometer.cpp
    src/icons.cpp
    src/nmea0183.cpp
    src/response.cpp
    src/sentence.cpp
    src/talkerid.cpp
    src/hexvalue.cpp
    src/expid.cpp
    src/lat.cpp
    src/latlong.cpp
    src/long.cpp
    src/gga.cpp
    src/gsv.cpp
    src/rmc.cpp
)

SET(HDRS
    include/button.h
	include/dial.h
	include/icons.h
	include/iirfilter.h
	include/instrument.h
	include/odometer_pi.h
	include/speedometer.h
	include/nmea0183.h
	include/SatInfo.h
)	

##set(OCPNHDRS ocpninclude/ocpn_plugin.h ocpninclude/pluginmanager.h ocpninclude/TexFont.h)
##set(EXTSRC)
##set(EXTINCLUDE extinclude/nlohmann/json.hpp extinclude/ODAPI.h)
##set(LIBSSRC libs/wxJSON/src/jsonreader.cpp libs/wxJSON/src/jsonval.cpp libs/wxJSON/src/jsonwriter.cpp)

set(LIBHDRS
    libs/GL/include/gl.h
    libs/GL/include/gl_private.h
    libs/GL/include/glext.h
    libs/GL/include/glu.h
    libs/wx/include/json_defs.h
    libs/wx/include/jsonreader.h
    libs/wx/include/jsonval.h
    libs/wx/include/jsonwriter.h)

## GPS Odometer plugin uses SVG for toolbar icons
add_definitions(-DPLUGIN_USE_SVG)

## GPS Odometer don't use JSON
##if(APPLE)
##    option(PI_JSON_SCHEMA_VALIDATOR "Use JSON Schema validator" OFF)
##else(APPLE)
##    option(PI_JSON_SCHEMA_VALIDATOR "Use JSON Schema validator" ON)
##endif(APPLE)

##if(PI_JSON_SCHEMA_VALIDATOR)
##    message(STATUS "${CMLOC}Using JSON Schema validation - warning this changes the 'flavour' of the executable and it may not be compatible with OpenCPN")
##    add_definitions(-DPI_JSON_SCHEMA_VALIDATOR)
##    message(STATUS "${CMLOC}Adding JSON Schema validation")

##    set(EXTSRC ${EXTSRC} extsrc/json-schema-validator/json-schema-draft7.json.cpp extsrc/json-schema-validator/json-uri.cpp extsrc/json-schema-validator/json-validator.cpp)
##    set(EXTINCLUDE ${EXTINCLUDE} extinclude/nlohmann/json.hpp extinclude/json-schema-validator/json-schema.hpp)
##    include_directories(BEFORE ${PROJECT_SOURCE_DIR}/extinclude/json-schema-validator)
##else(PI_JSON_SCHEMA_VALIDATOR)
##    message(STATUS "${CMLOC}Not using JSON Schema validation - warning invalid JSON 'may' cause unexpected crashes")
##endif(PI_JSON_SCHEMA_VALIDATOR)

## Need api-16 for ocpn_plugin.h
##include_directories(BEFORE ${PROJECT_SOURCE_DIR}/ocpninclude)
INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/api-16)
INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_SOURCE_DIR}/include)
##include_directories(BEFORE ${PROJECT_SOURCE_DIR}/extinclude)
##include_directories(BEFORE ${PROJECT_SOURCE_DIR}/libs/)
##include_directories(BEFORE ${PROJECT_SOURCE_DIR}/libs/wx/include)

## Statement below is required to collect all the set (headers and SRCS) - Adjust as required
##add_library(${PACKAGE_NAME} SHARED ${SRCS} ${HDRS} ${EXTSRC} ${LIBSSRC})
add_library(${PACKAGE_NAME} SHARED ${SRCS} ${HDRS})

add_definitions(-DTIXML_USE_STL)

##if(UNIX AND NOT APPLE)
##    if(PI_JSON_SCHEMA_VALIDATOR)
##        # Change OS/ABI to be UNIX - System V to make it run with OCPN
##        set(lib_name lib${PACKAGE_NAME}.so)
##        message(STATUS "${CMLOC}lib-name: ${lib_name}")
##        find_file(elfedit elfedit)
##        if(NOT (${elfedit} STREQUAL "elfedit-NOTFOUND"))
##            message(STATUS "${CMLOC}Will ensure OS/ABI is set to 0 to match OCPN")
##            add_custom_command(
##                TARGET ${PACKAGE_NAME}
##                POST_BUILD
##                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
##                DEPENDS ${PACKAGE_NAME}
##                COMMENT " Running post build action on ${lib_name}."
##                COMMAND sh -c 'elfedit --input-osabi=Linux --output-osabi=none ${lib_name} || (echo 0) ')
##        else(NOT (${elfedit} STREQUAL "elfedit-NOTFOUND"))
##            message(STATUS "${CMLOC}Cannot correct OS/ABI to match OCPN if it is generated incorrectly. Package binutils needed")
##        endif(NOT (${elfedit} STREQUAL "elfedit-NOTFOUND"))
##    endif(PI_JSON_SCHEMA_VALIDATOR)
##endif(UNIX AND NOT APPLE)


## ----- Do not change next section - needed to configure build process ----- ##
include("PluginInstall")
include("PluginLocalization")
include("PluginPackage")
## ----- do not change section above - needed to configure build process ----- ##
