# ---------------------------------------------------------------------------
# Author: LennartG 
# Copyright:   2018 - 2023
# License:     GPL v3+
# ---------------------------------------------------------------------------
# Frontend2  v1.0.278.11   Author: Jon Gough
# ---------------------------------------------------------------------------
## ----- When changing this file do NOT change the order in which sections occur        ----- ##
## ----- Changes should only be made between the section blocks that identify where     ----- ##
## ----- these changes should be. The whole configuration process relies on this        ----- ##
## ----- sequence to be successful                                                      ----- ##
##                                                                                      ----- ##
##----- Modify section below to include all the details for your plugin                 ----- ##

set(CMLOC "CMakeLists: ")

# define minimum cmake version
cmake_minimum_required(VERSION 3.5.1)
if(COMMAND cmake_policy)
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif(POLICY CMP0043)
    cmake_policy(SET CMP0048 NEW)
    if(POLICY CMP0076)
        cmake_policy(SET CMP0076 NEW)
    endif(POLICY CMP0076)
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
set(SHORT_DESCRIPTION "GPS/GNSS based for release 5.8 and above")
set(LONG_DESCRIPTION "Using GPS/GNSS satellite data to calculate log and trip information as well as deparure and arrival times")

set(VERSION_MAJOR "0")
set(VERSION_MINOR "8")
set(VERSION_PATCH "2")
set(VERSION_TWEAK "0")
set(VERSION_DATE "18/01/2024")  # DD/MM/YYYY format
set(OCPN_MIN_VERSION "ov58")
set(OCPN_API_VERSION_MAJOR "1")
set(OCPN_API_VERSION_MINOR "18")

set(PARENT "opencpn")

# The next line allows setup of a local webserver with git for testing purposes.
# The default is github.com.
#set(GIT_REPOSITORY_SERVER "github.com")

# Specifies Cloudsmith upload repository suffix for each catalog
set(PROD "prod")    #Standard Repos
set(BETA "beta")    #Standard Repos
set(ALPHA "alpha")  #Standard Repos

# Set if your Cloudsmith Base Repository name does not match your Git Repository name.
set (CLOUDSMITH_BASE_REPOSITORY "gps-odometer")  #without the pi

# Set the following to use OpenCPN cloudsmith repositories instead of own repositories
set(CLOUDSMITH_USER "opencpn") 
#set(CLOUDSMITH_USER "LennartG-Sve")

set(XML_INFO_URL "https://opencpn-manuals.github.io/main/gps-odometer/index.html")
set(XML_SUMMARY ${SHORT_DESCRIPTION})
set(XML_DESCRIPTION ${LONG_DESCRIPTION})

##
## ----- Modify section above to include all the details for your plugin ----- ##
##

##
## ----- Modify section below if there are special requirements for the plugin ----- ##
##

## GPS Odometer uses SVG graphics for the toolbar icons
option(PLUGIN_USE_SVG "Use SVG graphics" ON)

set(CMAKE_CXX_STANDARD 11)

# Use local version of GLU library
# requires libs/glu directory
set(USE_LOCAL_GLU TRUE)
# Prefer libGL.so to libOpenGL.so, see CMP0072
set(OpenGL_GL_PREFERENCE "LEGACY")

# No need to use OpenGL for GPS-Odometer
option(USE_GL "Enable OpenGL support" OFF)
message(STATUS "${CMLOC}USE_GL: ${USE_GL}")

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif("${CMAKE_BUILD_TYPE}" STREQUAL "")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "" "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
message(STATUS "${CMLOC}Build type: ${CMAKE_BUILD_TYPE}")

##
## ----- The statements below are used to setup standard variables that are required by the 
##       CMAKE process - do not remove ----- ##

project(${PACKAGE})

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;")

include("PluginSetup")

##
## ----- The above is used to setup standard variables that are required by the CMAKE process - do not remove ----- ##
##

##
## ----- Modify section below if there are special requirements for the plugin ----- ##
##

set(CMAKE_VERBOSE_MAKEFILE "Activate verbose mode for make files" ON)

option(Plugin_CXX11 "Use c++11" OFF)

##
## ----- Modify section above if there are special requirements for the plugin ----- ##
##

##
## ----- do not change next section - needed to configure build process ----- ##
##

include("PluginConfigure")

##
## ----- do not change section above - needed to configure build process ----- ##
##

#
# ----- Change below to match project requirements for source, headers, etc. ----- ##
#

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
    src/rmc.cpp
#    libs/wxJSON/src/jsonval.cpp
#    libs/wxJSON/src/jsonreader.cpp
#    libs/wxJSON/src/jsonwriter.cpp
    libs/N2KParser/src/N2kMessages.cpp
    libs/N2KParser/src/N2kMsg.cpp
    libs/N2KParser/src/N2KParser.cpp
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
    include/nmea0183.hpp
    include/SatInfo.h
)	

#set(OCPNHDRS
#    ocpninclude/cutil.h
#    ocpninclude/pluginmanager.h
#    ocpninclude/shaders.h
#    ocpninclude/TexFont.h
#    ocpninclude/vector2D.h
#    libs/ocpn-api/ocpn_plugin.h
#)

#set(EXTINCLUDE
#    extinclude/ODAPI.h
#    extinclude/ODJSONSchemas.h
#)

set(LIBHDRS
    libs/GL/gl.h
    libs/GL/gl_private.h
    libs/GL/glext.h
    libs/GL/glu.h
#    libs/wxJSON/include/json_defs.h
#    libs/wxJSON/include/jsonreader.h
#    libs/wxJSON/include/jsonval.h
#    libs/wxJSON/include/jsonwriter.h
    libs/N2KParser/include/N2kDef.h
    libs/N2KParser/include/N2kMessages.h
    libs/N2KParser/include/N2kMsg.h
    libs/N2KParser/include/N2KParser.h
    libs/N2KParser/include/N2kTypes.h
    libs/N2KParser/include/NMEA2000StdTypes.h
)

## GPS Odometer plugin uses SVG for toolbar icons
add_definitions(-DPLUGIN_USE_SVG)

##set(EXTINCLUDE_DIR ${EXTINCLUDE_DIR} extinclude libs/ocpn-api/)
## set(EXTINCLUDE_DIR libs/ocpn-api/)

##
## ----- JSON validation is not used, section skipped ----- ##
##

INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/include)
## INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/ocpninclude)
## include_directories(BEFORE ${PROJECT_SOURCE_DIR}/${EXTINCLUDE_DIR})
## include_directories(BEFORE ${PROJECT_SOURCE_DIR}/libs/)
include_directories(BEFORE ${PROJECT_SOURCE_DIR}/libs/GL)
## include_directories(BEFORE ${PROJECT_SOURCE_DIR}/libs/ocpn-api)
## include_directories(BEFORE ${PROJECT_SOURCE_DIR}/libs/wxJSON/include)

##INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/api-16)
INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/src)

#
# ----- Change below to match project requirements for android build ----- ##
#

# if(QT_ANDROID)
#     include_directories( ${PROJECT_SOURCE_DIR}/extinclude/GLES2)
# endif(QT_ANDROID)

##
## ----- Change above to match project requirements for android build ----- ##
##

#target_sources(${PACKAGE_NAME} PUBLIC ${SRCS} ${HDRS} ${EXTSRC} ${LIBHDRS})
target_sources(${PACKAGE_NAME} PUBLIC ${SRCS} ${HDRS} ${LIBHDRS})

if(NOT OCPN_FLATPAK_CONFIG)
    # Build environment not available when flatpak is being configured
    # so following statements will not work
    message(STATUS "${CMLOC}Adding target link libraries to ${PACKAGE_NAME}")

    if (WIN32)
        add_subdirectory(opencpn-libs/WindowsHeaders)
        target_link_libraries(${PACKAGE_NAME} windows::headers)
    endif (WIN32)

    add_subdirectory(opencpn-libs/api-18)
    target_link_libraries(${PACKAGE_NAME} ocpn::api)

    add_subdirectory(opencpn-libs/tinyxml)
    target_link_libraries(${PACKAGE_NAME} ocpn::tinyxml)

##    add_subdirectory(opencpn-libs/odapi)
##    target_link_libraries(${PACKAGE_NAME} ocpn::odapi)

##    add_subdirectory(opencpn-libs/jsonlib)
##    target_link_libraries(${PACKAGE_NAME} ocpn::jsonlib)

    add_subdirectory(opencpn-libs/wxJSON)
    target_link_libraries(${PACKAGE_NAME} ocpn::wxjson)

##    add_subdirectory(opencpn-libs/plugin_dc)
##    target_link_libraries(${PACKAGE_NAME} ocpn::plugin-dc)

endif(NOT OCPN_FLATPAK_CONFIG)

add_definitions(-DTIXML_USE_STL)


##
## ----- JSON validation is not used, section skipped ----- ##
##


##
## ----- do not change next section - needed to configure build process ----- ##
##

# Needed for android builds
if(QT_ANDROID)
    include_directories(BEFORE ${qt_android_include})
endif(QT_ANDROID)

# Needed for all builds
include("PluginInstall")
include("PluginLocalization")
include("PluginPackage")

##
## ----- do not change section above - needed to configure build process ----- ##
##
message(STATUS "${CMLOC}include directories: ")
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
    message(STATUS "${CMLOC}dir='${dir}'")
endforeach()
