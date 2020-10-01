##---------------------------------------------------------------------------
## Author:      Steven Adler (based on standard OpenCPN Plug-In CMAKE commands)
## Copyright:   2018
## License:     GPL v3+
##---------------------------------------------------------------------------

# define minimum cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(odometer_pi)

SET(PACKAGE_NAME odometer_pi)
SET(VERBOSE_NAME ODOMETER)
SET(TITLE_NAME ODOMETER)
SET(CPACK_PACKAGE_CONTACT "TwoCanPlugin@hotmail.com")
# SET(PLUGIN_VERSION_MAJOR "0")
# SET(PLUGIN_VERSION_MINOR "2")
# SET(PLUGIN_VERSION_PATCH "0")
# SET(NAME_SUFFIX "ov52")
# SET(PLUGIN_VERSION_DATE "2020-09-07")
SET(CMAKE_BUILD_TYPE Release)
SET(CMAKE_CXX_STANDARD 11)

INCLUDE("cmake/PluginConfigure.cmake")

## Need api-16 for ocpn_plugin.h
INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/api-16)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/nmea0183 ${CMAKE_SOURCE_DIR}/src)

SET(SRC_DASHBOARD
    src/odometer_pi.h
    src/odometer_pi.cpp
    src/iirfilter.cpp
    src/iirfilter.h
    src/instrument.cpp
    src/instrument.h
    src/button.cpp
    src/button.h
    src/dial.cpp
    src/dial.h
    src/speedometer.cpp
    src/speedometer.h
    src/icons.cpp
    src/icons.h
)

SET(SRC_NMEA0183
    src/nmea0183/nmea0183.cpp
    src/nmea0183/nmea0183.hpp
    src/nmea0183/response.hpp
    src/nmea0183/response.cpp
    src/nmea0183/sentence.hpp
    src/nmea0183/sentence.cpp
    src/nmea0183/talkerid.cpp
    src/nmea0183/hexvalue.cpp
    src/nmea0183/expid.cpp
    src/nmea0183/lat.cpp
    src/nmea0183/latlong.cpp
    src/nmea0183/LatLong.hpp
    src/nmea0183/long.cpp
    src/nmea0183/gga.cpp
    src/nmea0183/gga.hpp
    src/nmea0183/gsv.cpp
    src/nmea0183/gsv.hpp
    src/nmea0183/rmc.cpp
    src/nmea0183/rmc.hpp
)

ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_DASHBOARD} ${SRC_NMEA0183} )


IF(WIN32)
    OPTION(OCPN_USE_EXTERN_CURL "Use external libcurl" OFF)
ELSE(WIN32)
    SET(OCPN_USE_EXTERN_CURL ON)
ENDIF(WIN32)

IF(OCPN_USE_CURL)

    ADD_DEFINITIONS(-D__OCPN_USE_CURL__)

    IF(OCPN_USE_EXTERN_CURL)
        FIND_PACKAGE(CURL REQUIRED)
        IF(CURL_FOUND)
            MESSAGE (STATUS "Using SYSTEM curl library...${CURL_LIBRARIES}...${CURL_INCLUDE_DIR}")
        ENDIF(CURL_FOUND)
        INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR})
    ELSE(OCPN_USE_EXTERN_CURL)
#        INCLUDE("Curl.cmake")
        MESSAGE (STATUS "Using local curl library...")
    ENDIF(OCPN_USE_EXTERN_CURL)

 
    IF(WIN32) 
        SET(SRC_WXCURL
        src/wxcurl/base.cpp
        src/wxcurl/dav.cpp
        src/wxcurl/davtool.cpp
        src/wxcurl/dialog.cpp
        src/wxcurl/ftp.cpp
        src/wxcurl/ftpparse.cpp
        src/wxcurl/ftptool.cpp
        src/wxcurl/http.cpp
        src/wxcurl/panel.cpp
        src/wxcurl/telnet.cpp
        src/wxcurl/thread.cpp
        src/wxcurl/utils.cpp
        )
    ENDIF(WIN32)

#    ADD_LIBRARY(WXCURL ${SRC_WXCURL})
    INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/wxcurl)
    
    MESSAGE (STATUS "wxCurl configured...")

ENDIF(OCPN_USE_CURL)

IF(UNIX AND NOT APPLE)
 INCLUDE("cmake/FindTinyXML.cmake")
 FIND_PACKAGE(TinyXML QUIET)
ENDIF(UNIX AND NOT APPLE)

IF(TINYXML_FOUND)
 message (STATUS "Plugin is building with system tinyxml")
 INCLUDE_DIRECTORIES(${TINYXML_INCLUDE_DIR})
ELSE(TINYXML_FOUND)
 message (STATUS "Plugin is building with embedded tinyxml")
 INCLUDE_DIRECTORIES(src/tinyxml/)
ENDIF(TINYXML_FOUND)
ADD_DEFINITIONS(-DTIXML_USE_STL)


#===============================================================
#  MODIFY and adjust ADD_LIBRARY listing for the plugin
#===============================================================

ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_ODOMETER}  ${SRC_NMEA0183} )
# ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_VDR} ${SRC_JSON} )
# ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_SQUIDDIO} ${SRC_NMEA0183} ${SRC_JSON} ${SRC_LTINYXML})

IF (QT_ANDROID )
    TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${OCPN_Core_LIBRARIES} )
    TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} "-lc++_shared")
    TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} "-lz" )
    IF(_wx_selected_config MATCHES "androideabi-qt-arm64")
      TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/${OCPN_Android_Common}/opencpn/API-117/libarm64/libgorp.so )
    ELSE(_wx_selected_config MATCHES "androideabi-qt-arm64")
      TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/${OCPN_Android_Common}/opencpn/API-117/libarmhf/libgorp.so )
    ENDIF(_wx_selected_config MATCHES "androideabi-qt-arm64")
  
ENDIF(QT_ANDROID)


if (MINGW)
  SET(OPENCPN_IMPORT_LIB "${API_DIR}/mingw/libopencpn.dll.a")
elseif (WIN32)
  SET(OPENCPN_IMPORT_LIB "${API_DIR}/opencpn.lib")
endif ()

IF(WIN32)
  TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${CURL_LIBRARIES})
  TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${OPENCPN_IMPORT_LIB} )

ENDIF(WIN32)

IF(NOT APPLE AND NOT QT_ANDROID )
TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${wxWidgets_LIBRARIES} )
ENDIF(NOT APPLE AND NOT QT_ANDROID)


#====================================================================
# Install any special data files or binary helpers here....
# The PlugIn library itself and any "data" directory (with contents) 
# gets installed in generic PluginInstall.cmake later
#====================================================================

IF(UNIX AND NOT APPLE)
SET(PARENT opencpn)
SET(PREFIX_PLUGINS ${LIB_INSTALL_DIR}/${PARENT})
SET(PREFIX_PARENTBIN bin)
SET(PREFIX_PARENTDATA ${PREFIX_DATA}/${PARENT})

#=================================================
# Oesenc Chart Catalog stuff - Not Needed
#=================================================

# IF(QT_ANDROID)										
# etc, etc.

# IF (ARCH MATCHES "amd64" OR ARCH MATCHES "x86_64") 

#	INSTALL(FILES "buildlinux64/oeserverd/oeserverd"
#   etc, etc.

# ELSE (ARCH MATCHES "amd64" OR ARCH MATCHES "x86_64")
#   etc, etc.

# ENDIF (ARCH MATCHES "amd64" OR ARCH MATCHES "x86_64")
#
# IF(PACKAGE_FORMAT MATCHES "DEB")
#    etc etc

# IF(PACKAGE_FORMAT MATCHES "DEB" AND NOT QT_ANDROID)

# INSTALL(FILES "src/rrc_eula_ChartSetsForOpenCPN-v16-12.txt"
#      PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
#      DESTINATION ${PREFIX_PARENTDATA}/plugins/${PACKAGE_NAME} )
# necessary endif

#ENDIF(QT_ANDROID)

ENDIF(UNIX AND NOT APPLE)
#===========================================
#  Oesenc Chart permissions - Not needed 
#============================================
#IF(WIN32)
#    INSTALL(FILES "buildwin/oeserverd/oeserverd.exe"
#   etc, etc
# ENDIF(WIN32)
#IF(APPLE)
#INSTALL(FILES "buildosx/oeserverd/oeserverd"
# etc, etc
#ENDIF(APPLE)

#==============================================
# From Oesenc - Needed
#==============================================

IF(NOT WIN32)
SET(CPACK_SET_DESTDIR ON)
ENDIF(NOT WIN32)

SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "dsr")

#============================================
# Do not change, needed to build app
#=============================================
INCLUDE("cmake/PluginSetup.cmake")

configure_file(
  ${CMAKE_SOURCE_DIR}/pkg_version.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/pkg_version.sh
)

set(PLUGIN_NAME ${VERBOSE_NAME}-plugin-${PKG_TARGET}-${PKG_TARGET_VERSION})

#=============================================================
# Automatically configures file using ${VERBOSE_NAME}
#
#     <pluginname>-plugin.xml.in
#
# Modify/check these file names accordingly,  root directory 
#=============================================================
configure_file(
  ${CMAKE_SOURCE_DIR}/${VERBOSE_NAME}-plugin.xml.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.xml
)

#============================================
# Do not change, needed to build app
#=============================================
INCLUDE("cmake/PluginLocalization.cmake")

INCLUDE("cmake/PluginInstall.cmake")
INCLUDE("cmake/PluginLocalization.cmake")
INCLUDE("cmake/PluginPackage.cmake")

