# ---------------------------------------------------------------------------
# Author: LennartG 
# Copyright:   2018
# License:     GPL v3+
# ---------------------------------------------------------------------------
# Frontend2  v1.0.137.0   Author: Jon Gough 
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
set(PACKAGE "gps-odometer")
set(SHORT_DESCRIPTION "GPS Odometer plugin for OpenCPN")
set(LONG_DESCRIPTION "GPS controlled Dashboard based Odometer plugin for OpenCPN, displays GPS calculated Log and Trip information")

set(VERSION_MAJOR "0")
set(VERSION_MINOR "2")
set(VERSION_PATCH "4")
set(VERSION_TWEAK "0")
set(VERSION_DATE "25/10/2020")
set(OCPN_MIN_VERSION "ov50")
set(OCPN_API_VERSION_MAJOR "1")
set(OCPN_API_VERSION_MINOR "16")

set(PARENT "opencpn")

# The next line allows setup of a local webserver with git for testing purposes.
# The default is shown below, but it is done in cmake/pluginconfigure.cmake now.
# This default is needed to build locally and to build flatpak.
set(GIT_REPOSITORY_SERVER "github.com")

#No longer needed, queried and found by PluginConfigure.cmake.
#set(GIT_USER "rgleason")
#set(GIT_REPOSITORY_NAME "testplugin_pi")						 

# Specifies Cloudsmith upload repository suffix for each catalog
set(PROD "prod")    #Standard Repos
set(BETA "beta")    #Standard Repos
set(ALPHA "alpha")  #Standard Repos

#set(CLOUDSMITH_USER "LennartG-Sve")
set(CLOUDSMITH_USER "opencpn") #Match the cloudsmith org name, not the user.

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

##
## ----- The statements below are used to setup standard variables that are required by the 
##       CMAKE process - do not remove ----- ##
##

project(${PACKAGE})

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;")

##=======================================
include("PluginSetup")
##=======================================
## ----- Above is used to setup standard variables that are required by the CMAKE process - do not remove ----- ##
## ----- Modify section below if there are special requirements for the plugin ----- ##
##
set(CMAKE_VERBOSE_MAKEFILE ON)

option(Plugin_CXX11 "Use c++11" OFF)

##
## ----- Modify section above if there are special requirements for the plugin ----- ##
## ----- do not change next section - needed to configure build process ----- ##
##
##=======================================
include("PluginConfigure")
##=======================================
##
## ----- do not change section above - needed to configure build process ----- ##
## ----- Change below to match project requirements for source, headers, etc. ----- ##
##
## GPS Odometer plugin does not understand/use this definition
##add_definitions(-DUSE_S57)

## GPS Odometer plugin uses SVG for toolbar icons
add_definitions(-DPLUGIN_USE_SVG)

SET(SRC_ODOMETER
    src/odometer_pi.cpp
    src/iirfilter.cpp
    src/instrument.cpp
    src/button.cpp
    src/dial.cpp
    src/speedometer.cpp
    src/icons.cpp
    api-16/ocpn_plugin.h
)

SET(SRC_NMEA0183
    src/nmea0183/nmea0183.cpp
    src/nmea0183/response.cpp
    src/nmea0183/sentence.cpp
    src/nmea0183/talkerid.cpp
    src/nmea0183/hexvalue.cpp
    src/nmea0183/expid.cpp
    src/nmea0183/lat.cpp
    src/nmea0183/latlong.cpp
    src/nmea0183/long.cpp
    src/nmea0183/gga.cpp
    src/nmea0183/gsv.cpp
    src/nmea0183/rmc.cpp
)

SET(HDRS
    src/button.h
	src/dial.h
	src/icons.h
	src/iirfilter.h
	src/instrument.h
	src/odometer_pi.h
	src/speedometer.h
	src/nmea0183/nmea0183.h
	src/nmea0183/SatInfo.h
)	

## Need api-16 for ocpn_plugin.h
INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/api-16)
INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/src/nmea0183)
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_SOURCE_DIR}/include)

#INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src/nmea0183 ${CMAKE_SOURCE_DIR}/src)
##
## Statement below is required to collect all the set( headers and SRCS  - Adjust as required
## This add_library  can be used in conditionals below.
##
## SET(EXTSRC  ${EXTSRC}   etc   Can be in an if statement as below
##
##==========================================================================
ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_ODOMETER} ${SRC_NMEA0183} ${HDRS} )
##==========================================================================

add_definitions(-DTIXML_USE_STL)

## These are NOT used in Testplugin, used for other plugins
##=======================================
#include("PluginJSON")
#include("PluginXML")
#include("PluginGL")
##=======================================
##
## ----- Change above to match project requirements ----- ##
## ----- do not change next section - needed to configure build process ----- ##
##  
##=======================================
include("PluginInstall")
include("PluginLocalization")
include("PluginPackage")
##=======================================													  
##
## ----- do not change section above - needed to configure build process ----- ##
##


