# ---------------------------------------------------------------------------
# Author: LennartG 
# Copyright:   2018
# License:     GPL v3+
# ---------------------------------------------------------------------------
# TP Frontend2  v1.0.230   Author: Jon Gough  05/03/2023") # DD/MM/YYYY format
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
# Need cmake 3.13 to implement this, but raspbian builds are at 3.7.2 and bionic at 3.10 and xenial builds at 3.5.1
#    cmake_policy(SET CMP0076 NEW)
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
set(VERSION_MINOR "6")
set(VERSION_PATCH "6")
set(VERSION_TWEAK "6")
set(VERSION_DATE "25/03/2023")  # DD/MM/YYYY format
set(OCPN_MIN_VERSION "ov571")
set(OCPN_API_VERSION_MAJOR "1")
set(OCPN_API_VERSION_MINOR "16")

set(PARENT "opencpn")

# The next line allows setup of a local webserver with git for testing purposes.
# The default is github.com.
#set(GIT_REPOSITORY_SERVER "github.com")

# Specifies Cloudsmith upload repository suffix for each catalog
set(PROD "prod")    #Standard Repos
set(BETA "beta")    #Standard Repos
set(ALPHA "alpha")  #Standard Repos

# Set if yourCloudsmith Base Repository name does not match your Git Repository name.
set (CLOUDSMITH_BASE_REPOSITORY "gps-odometer")  #without the pi
# Match the cloudsmith org name, not the user.
set(CLOUDSMITH_USER "opencpn") 
#set(CLOUDSMITH_USER "LennartG-Sve")
# set(XML_INFO_URL "https://opencpn.org/wiki/dokuwiki/doku.php?id=opencpn:opencpn_user_manual:plugins:other:odometer") 
set(XML_INFO_URL "https://opencpn-manuals.github.io/plugins/gps-odometer/0.1/index.html")
set(XML_SUMMARY ${SHORT_DESCRIPTION})
set(XML_DESCRIPTION ${LONG_DESCRIPTION})

##
## ----- Modify section above to include all the details for your plugin ----- ##
##

##
## ----- Modify section below if there are special requirements for the plugin ----- ##
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

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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
    src/rmc.cpp
    src/wxJSON/jsonval.cpp
    src/wxJSON/jsonreader.cpp
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
    include/wx/json_defs.h
    include/wx/jsonreader.h
    include/wx/jsonval.h
    include/wx/jsonwriter.h
)	

## GPS Odometer plugin uses SVG for toolbar icons
add_definitions(-DPLUGIN_USE_SVG)

## Need ocpn-api for ocpn_plugin.h
##INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/api-16)
INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_SOURCE_DIR}/include)

include_directories(BEFORE ${PROJECT_SOURCE_DIR}/libs/)

include_directories(BEFORE ${PROJECT_SOURCE_DIR}/libs/ocpn-api)


#
# ----- Change below to match project requirements for android build ----- ##
#

if(QT_ANDROID)
    include_directories( ${PROJECT_SOURCE_DIR}/extinclude/GLES2)
endif(QT_ANDROID)

##
## ----- Change above to match project requirements for android build ----- ##
##
## Statement below is required to collect all the set (headers and SRCS) - Adjust as required

target_sources(${PACKAGE_NAME} PUBLIC ${SRCS} ${HDRS})

if(NOT OCPN_FLATPAK_CONFIG)
    # Build environment not available when flatpak is being configured so following statements will not work
    message(STATUS "${CMLOC}Adding target link libraries to ${PACKAGE_NAME}")

    add_subdirectory(libs/tinyxml)

    target_link_libraries(${PACKAGE_NAME} ocpn::tinyxml)
endif(NOT OCPN_FLATPAK_CONFIG)

add_definitions(-DTIXML_USE_STL)

## ----- Do not change next section - needed to configure build process ----- ##
# Not using JSON validation


##
## ----- Change above to match project requirements ----- ##
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

## ----- do not change section above - needed to configure build process ----- ##

message(STATUS "${CMLOC}include directories: ")
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
    message(STATUS "${CMLOC}dir='${dir}'")
endforeach()
