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


INCLUDE("cmake/PluginInstall.cmake")
INCLUDE("cmake/PluginLocalization.cmake")
INCLUDE("cmake/PluginPackage.cmake")

