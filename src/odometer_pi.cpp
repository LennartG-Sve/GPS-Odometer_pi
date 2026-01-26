//
// This file is part of GPS Odometer, a plugin for OpenCPN.
// based on the original version of dashboard.
//

/* $Id: odometer_pi.cpp, v1.0 2010/08/05 SethDart Exp $
 *
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin
 * Author:   Jean-Eudes Onfray
 *
 */

 /**************************************************************************
 *   Copyright (C) 2010 by David S. Register                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 ***************************************************************************
 */

// wxWidgets Precompiled Headers
#include "wx/wxprec.h"
#include "stdlib.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#include "wx/radiobox.h"
#endif

#include "../include/odometer_pi.h"
#include "version.h"

#include <typeinfo>
#include "icons.h"
#include "../libs/wxJSON/include/json_defs.h"
#include "../libs/wxJSON/include/jsonreader.h"
#include "../libs/wxJSON/include/jsonval.h"
#include "../libs/wxJSON/include/jsonwriter.h"
#include "../libs/N2KParser/include/N2KParser.h"
#include <string>
#include <wx/textfile.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>
#include <wx/mimetype.h>
#include "wx/sizer.h"

// Global variables for fonts
wxFont *g_pFontTitle;
wxFont *g_pFontData;
wxFont *g_pFontLabel;
wxFont *g_pFontSmall;

wxFont g_FontTitle;
wxFont g_FontData;
wxFont g_FontLabel;
wxFont g_FontSmall;

wxFont *g_pUSFontTitle;
wxFont *g_pUSFontData;
wxFont *g_pUSFontLabel;
wxFont *g_pUSFontSmall;

wxFont g_USFontTitle;
wxFont g_USFontData;
wxFont g_USFontLabel;
wxFont g_USFontSmall;


// Preferences, Units and Values
int         g_iShowSpeed = 1;
int         g_iShowDepArrTimes = 1;
int         g_iShowTripLeg = 1;
int         g_iLogFileAge = 0;
int         g_iIncludeTripStops = 0;
int         g_iTripStopMinutes = 5;
int         g_iSelectLogTrips;
int         g_iSelectLogFormat;
int         g_iSelectLogOutput;
int         g_iOdoProtocol;
int         g_iOdoSpeedMax;
int         g_iOdoOnRoute;
int         g_iOdoUTCOffset;
int         g_iOdoSpeedUnit;
int         g_iOdoDistanceUnit;
int         g_iResetTrip = 0;
int         g_iStartStopLeg = 0;
int         g_iResetLeg = 0;
int         g_iAutoResetTrip = 0;
int         g_iAutoResetTripTime = 6;
wxString    g_sDataDir;

/*
TripMode = 0;    // Trip reset, speed below OnRoute
TripMode = 1;    // Departed, speed above OnRoute
TripMode = 2;    // Speed below OnRoute, arrived after departure
TripMode = 3;    // Resuming trip, speed above OnRoute
TripMode = 4;    // Speed below OnRoute, arrived after resumed trip
TripMode = 5;    // Checking and eventually removing trip stops
TripMode = 6;    // Remove last arrival and skip this resume

Default log settings:
   Include Trip Stops:   No
   Trip Stop Minutes:    120 minutes
   Number of Log Trips;  Last three trips
   Log Format:           HTML
   Log Output:           Open output in viewer
*/


// Watchdog timer, performs two functions, firstly refresh the odometer every second,
// and secondly, if no data is received, set instruments to zero.
// BUG BUG Zeroing instruments not yet implemented
wxDateTime watchDogTime;

#if !defined(NAN)
static const long long lNaN = 0xfff8000000000000;
#define NAN (*(double*)&lNaN)
#endif

// The class factories, used to create and destroy instances of the PlugIn
extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr) {
    return (opencpn_plugin *) new odometer_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p) {
    delete p;
}

//---------------------------------------------------------------------------------------------------------
//
//    Odometer PlugIn Implementation
//
//---------------------------------------------------------------------------------------------------------

// !!! WARNING !!!
// do not change the order, add new instruments at the end, before ID_DBP_LAST_ENTRY!
// otherwise, for users with an existing opencpn configuration file, their instruments are changing !
enum { ID_DBP_D_SOG, ID_DBP_I_SUMLOG, ID_DBP_I_TRIPLOG, ID_DBP_I_DEPART, ID_DBP_I_ARRIV,
       ID_DBP_B_TRIPRES, ID_DBP_C_AUTORESET, ID_DBP_I_LEGDIST, ID_DBP_I_LEGTIME,
       ID_DBP_B_STARTSTOP, ID_DBP_B_LEGRES,
       ID_DBP_LAST_ENTRY /* this has a reference in one of the routines; defining a
       "LAST_ENTRY" and setting the reference to it, is one codeline less to change (and 
       find) when adding new instruments :-)  */
};

// Retrieve a caption for each instrument
wxString GetInstrumentCaption(unsigned int id) {
    switch(id) {
        case ID_DBP_D_SOG:
            return _("Speedometer");
        case ID_DBP_I_SUMLOG:
            return _("Sum Log Distance");
        case ID_DBP_I_TRIPLOG:
            return _("Trip Log Distance");
        case ID_DBP_I_DEPART:
            return _("Departure & Arrival");
        case ID_DBP_I_ARRIV:
            return wxEmptyString;
        case ID_DBP_B_TRIPRES:
            return _("Reset Trip");
        case ID_DBP_C_AUTORESET:
            return _("Autoreset Trip");
        case ID_DBP_I_LEGDIST:
            return _("Leg Distance & Time");
        case ID_DBP_I_LEGTIME:
            return wxEmptyString;
        case ID_DBP_B_STARTSTOP:
            return _("Start/Stop Leg");
        case ID_DBP_B_LEGRES:
            return _("Reset Leg");
		default:
			return wxEmptyString;
    }
}

// Populate an index, caption and image for each instrument for use in a list control
void GetListItemForInstrument(wxListItem &item, unsigned int id) {
    item.SetData(id);
    item.SetText(GetInstrumentCaption(id));

	switch(id) {
        case ID_DBP_D_SOG:
        item.SetImage(1);
        break;
        case ID_DBP_I_SUMLOG:
        case ID_DBP_I_TRIPLOG:
        case ID_DBP_I_DEPART:
        case ID_DBP_I_ARRIV:
        case ID_DBP_B_TRIPRES:
        case ID_DBP_C_AUTORESET:
        case ID_DBP_I_LEGDIST:
        case ID_DBP_I_LEGTIME:
        case ID_DBP_B_LEGRES:
        item.SetImage(0);
        break;
    }
}


// Constructs an id for the odometer instance
wxString MakeName() {
    return _T("ODOMETER");
}

//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

odometer_pi::odometer_pi(void *ppimgr) : opencpn_plugin_118(ppimgr), wxTimer(this) {
    // Create the PlugIn icons
    initialize_images();
}

// Odometer Destructor
odometer_pi::~odometer_pi(void) {
      delete _img_odometer_colour;
}

// Initialize the Odometer
int odometer_pi::Init(void) {
    AddLocaleCatalog(_T("opencpn-gpsodometer_pi"));

    // Used at startup, once started the plugin only uses version 2 configuration style
    m_config_version = -1;
    mPriDateTime = 99;
    mUTC_Watchdog = 2;

    g_pFontTitle = new wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL);
    g_pFontData = new wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    g_pFontLabel = new wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    g_pFontSmall = new wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

    g_pUSFontTitle = &g_USFontTitle;
    g_pUSFontData = &g_USFontData;
    g_pUSFontLabel = &g_USFontLabel;
    g_pUSFontSmall = &g_USFontSmall;


    // Wire up the OnClose AUI event
    m_pauimgr = GetFrameAuiManager();
    m_pauimgr->Connect(wxEVT_AUI_PANE_CLOSE, 
                       wxAuiManagerEventHandler(odometer_pi::OnPaneClose), NULL, this);

    // Get a pointer to the opencpn configuration object
    m_pconfig = GetOCPNConfigObject();

    // And load the configuration items
    LoadConfig();

    // Scaleable Vector Graphics (SVG) icons are stored in the following path.
    wxString iconFolder = GetPluginDataDir("gpsodometer_pi") + 
                          wxFileName::GetPathSeparator() + _T("data") +
                          wxFileName::GetPathSeparator();

    wxString normalIcon = iconFolder + _T("gpsodometer.svg");
    wxString toggledIcon = iconFolder + _T("gpsodometer_toggled.svg");
    wxString rolloverIcon = iconFolder + _T("gpsodometer_rollover.svg");

    g_sDataDir = iconFolder;

    // Add toolbar icon (in SVG format)
    m_toolbar_item_id = InsertPlugInToolSVG(_T(""), normalIcon, rolloverIcon, 
                        toggledIcon, wxITEM_CHECK, ("GPS Odometer"), _T(""), NULL,
                        ODOMETER_TOOL_POSITION, 0, this);

    // Having Loaded the config, then display the odometer
    ApplyConfig();

    // If we loaded a version 1 configuration, convert now to version 2,
    if(m_config_version == 1) {
        SaveConfig();
    }

    // Position, Rapid update   PGN 129026
    wxDEFINE_EVENT(EVT_N2K_129026, ObservedEvt);
    NMEA2000Id id_129026 = NMEA2000Id(129026);
    listener_129026 = std::move(GetListener(id_129026, EVT_N2K_129026, this));
    Bind(EVT_N2K_129026, [&](ObservedEvt ev) {
        HandleN2K_129026(ev);
    });

    // GNSS Position Data   PGN 129029
    wxDEFINE_EVENT(EVT_N2K_129029, ObservedEvt);
    NMEA2000Id id_129029 = NMEA2000Id(129029);
    listener_129029 = std::move(GetListener(id_129029, EVT_N2K_129029, this));
    Bind(EVT_N2K_129029, [&](ObservedEvt ev) {
        HandleN2K_129029(ev);
    });

    // Initialize the watchdog timer
    Start(1000, wxTIMER_CONTINUOUS);

    return ( WANTS_CURSOR_LATLON | WANTS_TOOLBAR_CALLBACK | INSTALLS_TOOLBAR_TOOL
            | WANTS_PREFERENCES | WANTS_CONFIG | WANTS_NMEA_SENTENCES | WANTS_NMEA_EVENTS
            | USES_AUI_MANAGER | WANTS_PLUGIN_MESSAGING );
}

bool odometer_pi::DeInit(void) {

    // Save the current configuration
    SaveConfig();

    // Set arrival trip mode if switched off while OnRoute, save current distances
    if (TripMode == 1) {
        TripMode = 2;
        saveTripMode = 1;
    }
    if ((TripMode == 3) || (TripMode >= 5)) {
        TripMode = 4;
        saveTripMode = 1;
    }
    saveTripDist = 1;
    saveTotDist = 1;
    WriteTripData();

    // Is watchdog timer started?
    if (IsRunning()) {
	Stop();
    }

    // Close the odometer instance
    OdometerWindow *odometer_window = m_ArrayOfOdometerWindow.Item(0)->m_pOdometerWindow;

    if (odometer_window) {
        m_pauimgr->DetachPane(odometer_window);
        odometer_window->Close();
        odometer_window->Destroy();
        m_ArrayOfOdometerWindow.Item(0)->m_pOdometerWindow = NULL;
    }

    // And this appears to close each odometer container
    OdometerWindowContainer *pdwc = m_ArrayOfOdometerWindow.Item(0);
    delete pdwc;
    return true;
}

double GetJsonDouble(wxJSONValue &value) {
  double d_ret = 0;
  if (value.IsDouble()) {
    d_ret = value.AsDouble();
    return d_ret;
  } else if (value.IsLong()) {
    int i_ret = value.AsLong();
    d_ret = i_ret;
    return d_ret;
  }
  return nan("");
}

// Called for each timer tick, refreshes each display
void odometer_pi::Notify() {

    // Force a repaint of the instrument panel
    OdometerWindow *odometer_window = m_ArrayOfOdometerWindow.Item(0)->m_pOdometerWindow;
	if (odometer_window) {
	    odometer_window->Refresh();
	}

    //  Manage the watchdogs, watch messages used
    mUTC_Watchdog--;
    if (mUTC_Watchdog <= 0) {
        mPriDateTime = 99;
        mUTC_Watchdog = gps_watchdog_timeout_ticks;
    }
}

int odometer_pi::GetAPIVersionMajor() {
    return OCPN_API_VERSION_MAJOR;
}

int odometer_pi::GetAPIVersionMinor() {
    return OCPN_API_VERSION_MINOR;
}

int odometer_pi::GetPlugInVersionMajor() {
    return PLUGIN_VERSION_MAJOR;
}

int odometer_pi::GetPlugInVersionMinor() {
    return PLUGIN_VERSION_MINOR;
}

int odometer_pi::GetPlugInVersionPatch() {
    return PLUGIN_VERSION_PATCH;
}

int odometer_pi::GetPlugInVersionPost() {
    return PLUGIN_VERSION_TWEAK;
}


wxString odometer_pi::GetCommonName() {
    return _T(PLUGIN_COMMON_NAME);
}

wxString odometer_pi::GetShortDescription() {
    return _(PLUGIN_SHORT_DESCRIPTION);
}

wxString odometer_pi::GetLongDescription() {
    return _(PLUGIN_LONG_DESCRIPTION);
}

// The plugin bitmap is loaded by the call to InitializeImages in icons.cpp
// Use png2wx.pl perl script to generate the binary data used in icons.cpp
wxBitmap *odometer_pi::GetPlugInBitmap() {
    return _img_odometer_colour;
}

// Sends the data value from the parsed NMEA sentence to each gauge
void odometer_pi::SendSentenceToAllInstruments(int st, double value, wxString unit) {

    OdometerWindow *odometer_window = m_ArrayOfOdometerWindow.Item(0)->m_pOdometerWindow;
	if (odometer_window) {
	    odometer_window->SendSentenceToAllInstruments(st, value, unit);
	}
}

// Using pluginmanager for NMEA Course and Speed data filter
void odometer_pi::SetPositionFix(PlugIn_Position_Fix &pfix)
{
    FilteredSpeed = pfix.Sog;
}

// This method is invoked by OpenCPN when reading Signal K sentences
void odometer_pi::ParseSignalK( wxString &msg) {

    wxJSONValue root;
    wxJSONReader jsonReader;
    int errors = jsonReader.Parse(msg, &root);

    if(root.HasMember("self")) {
        if(root["self"].AsString().StartsWith(_T("vessels.")))
            m_self = (root["self"].AsString());         // for java server, and OpenPlotter
                                                        // node.js server 1.20
        else
            m_self = _T("vessels.") + (root["self"].AsString());   // for Node.js server
    }

    if(root.HasMember("updates") && root["updates"].IsArray()) {
        wxJSONValue &updates = root["updates"];
        for (int i = 0; i < updates.Size(); ++i) {
            handleSKUpdate(updates[i]);
        }
    }
}

void odometer_pi::handleSKUpdate(wxJSONValue &update) {
    wxString sfixtime = "";

    if(update.HasMember("timestamp")) {
        sfixtime = update["timestamp"].AsString();
    }

    if(update.HasMember("values") && update["values"].IsArray()) {
        wxString talker = wxEmptyString;
        if (update.HasMember("source")) {
            if (update["source"].HasMember("talker")) {
                if (update["source"]["talker"].IsString()) {
                    talker = update["source"]["talker"].AsString();
                }
            }
        }

        for (int j = 0; j < update["values"].Size(); ++j) {
            wxJSONValue &item = update["values"][j];
              updateSKItem(item, talker, sfixtime);
        }
    }
}

void odometer_pi::updateSKItem(wxJSONValue &item, wxString &talker, wxString &sfixtime) {

    if (g_iOdoProtocol == 1) {    // Signal K selected

        if(item.HasMember("path") && item.HasMember("value")) {
            const wxString &update_path = item["path"].AsString();
            wxJSONValue &value = item["value"];

    // Container for last received sat-system info from SK-N2k
    // static wxString talkerID = wxEmptyString;  // Type of satellite (GPS, GLONASS...)

            SatsRequired = atoi(m_SatsRequired);
            HDOPdefine = atoi(m_HDOPdefine);
            SKSpeed = 0;

            if (update_path == _T("navigation.speedOverGround")){
                SKSpeed = 1.9438444924406 * GetJsonDouble(value);
                if (std::isnan(SKSpeed)) SKSpeed = 0;
            }

            else if (update_path == _T("navigation.datetime")) {
                if (mPriDateTime >= 1) {
                    mPriDateTime = 1;
                    wxString s_dt = (value.AsString()); //"2019-12-28T09:26:58.000Z"
                    s_dt.Replace('-', wxEmptyString);
                    s_dt.Replace(':', wxEmptyString);
                    wxString utc_dt = s_dt.BeforeFirst('T'); //Date
                    utc_dt.Append(s_dt.AfterFirst('T').Left( 6 )); //time
                    mUTCDateTime.ParseFormat(utc_dt.c_str(), _T("%Y%m%d%H%M%S"));
                    mUTC_Watchdog = gps_watchdog_timeout_ticks;
                }
            }

            else if (update_path == _T("navigation.gnss.satellites")) {
                if (value.IsInt()) {
                    SKSatsUsed = (value.AsInt());
                      // at least 4 satellites required
                    if (SatsRequired <= 4) SatsRequired = 4;
                }
            }

            else if (update_path == _T("navigation.gnss.methodQuality")) {
                 wxString SKGNSS_quality = (value.AsString());
                 SKQuality = 0;
                 // Must be one of 'GNSS Fix', 'DGNSS fix' or 'Precise GNSS'
                 if ((SKGNSS_quality == "GNSS Fix") || (SKGNSS_quality == "DGNSS fix") ||
                     (SKGNSS_quality == "Precise GNSS")) {
                     SKQuality = 1;
                 }
            }

            else if (update_path == _T("navigation.gnss.horizontalDilution")){
                SKHDOPlevel = GetJsonDouble(value);
                int HDOPdefine = atoi(m_HDOPdefine);
                if (HDOPdefine <= 1) HDOPdefine = 1;  // HDOP limit between 1 and 10
                if (HDOPdefine >= 10) HDOPdefine = 10;
              }

            GNSSok = 0;
            if ((SKQuality == 1) && (SKSatsUsed >= SatsRequired) && 
               (SKHDOPlevel <= HDOPdefine))  GNSSok = 1;
            CurrSpeed = SKSpeed;   // Should be used if filter is off
            Odometer();
        }
    }
}


// This method is invoked by OpenCPN when we specify WANTS_NMEA_SENTENCES
void odometer_pi::SetNMEASentence(wxString &sentence) {

    if (g_iOdoProtocol == 0) {            // NMEA 0183 selected

        m_NMEA0183 << sentence;
        if (m_NMEA0183.PreParse()) {
            if( m_NMEA0183.LastSentenceIDReceived == _T("GGA") ) {
                if( m_NMEA0183.Parse() ) {

                    GPSQuality = m_NMEA0183.Gga.GPSQuality;
                    if ((GPSQuality >= 0 ) && (GPSQuality <=5)) {
                        SignalQuality = 1;
                    } else {
                        SignalQuality = 0;
                    }

                    SatsUsed = m_NMEA0183.Gga.NumberOfSatellitesInUse;
                    SatsRequired = atoi(m_SatsRequired);
                    // at least 4 satellites required
                    if (SatsRequired <= 4) SatsRequired = 4;

                    HDOPlevel = m_NMEA0183.Gga.HorizontalDilutionOfPrecision;
                    HDOPdefine = atoi(m_HDOPdefine);
                    if (HDOPdefine <= 1) HDOPdefine = 1;  // HDOP limit between 1 and 10
                    if (HDOPdefine >= 10) HDOPdefine = 10;

                    // Ensure HDOP level is valid, will be 999 if field is empty
                    if ((HDOPlevel == 0.0) || (HDOPlevel >= 100)) HDOPlevel = 100;

                }
            }

            else if (m_NMEA0183.LastSentenceIDReceived == _T("RMC")) {
                if (m_NMEA0183.Parse() ) {
                    if (m_NMEA0183.Rmc.IsDataValid == NTrue) {

                        if( mPriDateTime >= 3 ) {
                            mPriDateTime = 3;
                            wxString dt = m_NMEA0183.Rmc.Date + m_NMEA0183.Rmc.UTCTime;
                            mUTCDateTime.ParseFormat( dt.c_str(), _T("%d%m%y%H%M%S") );
                            mUTC_Watchdog = gps_watchdog_timeout_ticks;
                        }

                        NMEASpeed = m_NMEA0183.Rmc.SpeedOverGroundKnots;
                        if (std::isnan(NMEASpeed)) NMEASpeed = 0;
                    }
                }
            }
            GNSSok = 0;
           if ((SignalQuality == 1) && (SatsUsed >= SatsRequired) && 
              (HDOPlevel <= HDOPdefine)) GNSSok = 1;
            CurrSpeed = NMEASpeed;   // Should be used if filter is off
            Odometer();
        }
    }
}

// NMEA2000, N2K
//...............


wxString talker_N2k = wxEmptyString;
void odometer_pi::HandleN2K_129026(ObservedEvt ev) {
  NMEA2000Id id_129026(129026);
  std::vector<uint8_t>v = GetN2000Payload(id_129026, ev);

    unsigned char SID;
    tN2kHeadingReference ref;
    double COG;
    double SOG;

    // Get current Speed Over Ground
    if (ParseN2kPGN129026(v, SID, ref, COG, SOG)) {
        if (g_iOdoProtocol == 2) {    // socketCAN (NMEA 2000) selected
            CurrSpeed = SOG;
        }
    }
}


void odometer_pi::HandleN2K_129029(ObservedEvt ev) {
  NMEA2000Id id_129029(129029);
  std::vector<uint8_t>v = GetN2000Payload(id_129029, ev);

  unsigned char SID;
  uint16_t DaysSince1970;
  double SecondsSinceMidnight;
  double Latitude, Longitude, Altitude;
  tN2kGNSStype GNSStype;
  tN2kGNSSmethod GNSSmethod;
  unsigned char nSatellites;
  double HDOP, PDOP, GeoidalSeparation;
  unsigned char nReferenceStations;
  tN2kGNSStype ReferenceStationType;
  uint16_t ReferenceSationID;
  double AgeOfCorrection;

  // Extract HDOP, nSatellites and GNSSmethod
  if (ParseN2kPGN129029(v, SID, DaysSince1970, SecondsSinceMidnight,
                        Latitude, Longitude, Altitude,
                        GNSStype, GNSSmethod,
                        nSatellites, HDOP, PDOP, GeoidalSeparation,
                        nReferenceStations, ReferenceStationType, ReferenceSationID,
                        AgeOfCorrection)) {

      if (g_iOdoProtocol == 2) {    // socketCAN (NMEA 2000) selected

          int HDOPdefine = atoi(m_HDOPdefine);
          if (HDOPdefine <= 1) HDOPdefine = 1;  // HDOP limit between 1 and 10
          if (HDOPdefine >= 10) HDOPdefine = 10;

          SatsRequired = atoi(m_SatsRequired);
          if (SatsRequired <= 4) SatsRequired = 4;  // at least 4 satellites required

          switch (GNSSmethod) {
              case 0: talker_N2k = "noGNSS"; break; 
              case 1: talker_N2k = "GNSSfix"; break; 
              case 2: talker_N2k = "DGNSS"; break;
              case 3: talker_N2k = "PreciseGNSS"; break;
              default: talker_N2k = wxEmptyString;
          }
          int Quality = 1;
          if ((talker_N2k == "noGNSS" ) || (talker_N2k.empty())) Quality = 0;

          GNSSok = 0;
          if ((Quality == 1) && (nSatellites >= SatsRequired) && 
               (HDOP <= HDOPdefine))  GNSSok = 1;
          Odometer();
      }

      wxString dmsg( _T("HandleN2KSpeed: ") );
      dmsg.append(talker_N2k);
      wxLogMessage(dmsg);
      printf("%s\n", dmsg.ToUTF8().data());
  }
}


// Generating Odometer info
//..........................

void odometer_pi::Odometer() {
     //  Adjust time to local time zone used by departure and arrival times
    UTCTime = wxDateTime::Now();
    double offset = ((g_iOdoUTCOffset-24)*30);
    wxTimeSpan TimeOffset(0, offset,0);
    LocalTime = UTCTime.Add(TimeOffset);
    m_LocalTime = LocalTime.FormatISOCombined(' ');

    // Speedometer
    FilterSOG = atoi(m_FilterSOG);
    // Filtered speed from OpenCPN (always?)
    if (FilterSOG != 0 ) CurrSpeed = FilteredSpeed;

    // Cover a speed bug in Signal K when no sats are found
    // Error when GNSSok goes from 0 to 1 as speeds remains over the onRoute limit
    // triggering false dep/arr times due to 'Filter NMEA Course and speed data'.
    // PwrOnDelaySecs is used to delay the GNSSok valid detection.

    wxDateTime GPSTime;    // Dummy for proper CurrTime function

    if (GNSSok == 0) {     // Give GPS time to stabilize once saying it's ok
       CurrSpeed = 0.0;
       CurrTime = LocalTime;
       int PwrOnDelaySecs = atoi(m_PwrOnDelSecs);
       if (PwrOnDelaySecs <= 4) PwrOnDelaySecs = 5;
       wxTimeSpan PwrOnDelay(0,0,PwrOnDelaySecs);
       wxDateTime GPSTime = CurrTime.Add(PwrOnDelay);
    }

    if ((CurrTime > LocalTime) || (PowerUpActive == 1)) {
        SendSentenceToAllInstruments(OCPN_DBP_STC_SOG, toUsrSpeed_Plugin (0.0,
            g_iOdoSpeedUnit), getUsrSpeedUnit_Plugin(g_iOdoSpeedUnit));
        CurrSpeed=0.0;
    } else {
        // Filter out not wanted decimals from iirfilter, need one decimal only
        SendSentenceToAllInstruments(OCPN_DBP_STC_SOG, toUsrSpeed_Plugin
            (round(mSOGFilter.filter(CurrSpeed) * 10) / 10,g_iOdoSpeedUnit),
            getUsrSpeedUnit_Plugin(g_iOdoSpeedUnit));
    }

    /* TODO: There must be a better way to receive the reset event from
             'OdometerInstrument_Button' but using a global variable for transfer.  */
    if (g_iResetTrip == 1) {
        UseSavedDepTime = 0;
        UseSavedArrTime = 0;
        UseSavedTrip = 0;
        DepTimeShow = 0;
        ArrTimeShow = 0;
        strDep = "---";
        strArr = "---";
        TripDist = 0.0;
        m_TripDist << TripDist;
        // SaveConfig();              // TODO BUG: Does not save config file
        TripMode = 0;
        saveTripMode = 1;
        saveTripDist = 1;
        WriteTripData();
        readDataFile = 1;
        g_iLogFileAge = 1;
        g_iResetTrip = 0;
    }

    if (g_iResetLeg == 1) {
        LegDist = 0.0;
        LegTime = 0;
        m_LegDist << LegDist;
        m_LegTime = "---";
        LegStart = LocalTime;
        // SaveConfig();              // TODO BUG: Does not save config file
        g_iResetLeg = 0;
    }

    // Set departure time to local time if CurrSpeed is greater than or equal to
    OnRouteSpeed = g_iOdoOnRoute;
    // Reset after arrival
    if ((CurrSpeed >= OnRouteSpeed) && strDep == "---" )  {
        DepTime = LocalTime;
        m_DepTime = LocalTime.FormatISOCombined(' ');
    }

    // Reset after power up, before trip start
    if ((CurrSpeed >= OnRouteSpeed) && SetDepTime == 1 )  {
        DepTime = LocalTime;
        m_DepTime = LocalTime.FormatISOCombined(' ');
        SetDepTime = 0;
    }

    // Departure time
    // Reset trip button pressed
    if (UseSavedDepTime == 0) {
        DepTime = LocalTime;
        UseSavedDepTime = 1;
        ReadDepFromData = 0;
        DepTimeShow = 0;
        if (CurrSpeed >= OnRouteSpeed)  DepTimeShow = 1;
    }

    if ((CurrSpeed > OnRouteSpeed) && (PowerUpActive == 0)) {
        DepTimeShow = 1;
        if (TripMode == 0)  {

            // Save logDepTime to trip data log file
            TripMode = 1;
            saveDepTime = 1;
            saveTripMode = 1;
            WriteTripData();
        }

        // Restarted trip after departure or resumed trip
        if ((PowerUpActive == 0) && ((TripMode == 2) || (TripMode == 4)))  {

            // Fetch trip stop times, setup in setup dialogue
            switch (g_iTripStopMinutes) {
                case 0:
                    StopMinutes = 0;
                    break;
                case 1:
                    StopMinutes = 5;
                    break;
                case 2:
                    StopMinutes = 15;
                    break;
                case 3:
                    StopMinutes = 30;
                    break;
                case 4:
                    StopMinutes = 60;
                    break;
                case 5:
                    StopMinutes = 120;
                    break;
                case 6:
                    StopMinutes = 240;
                    break;
                case 7:
                    StopMinutes = 480;
                    break;
                case 8:
                    StopMinutes = 9999;
                    break;
            }

            // Define required time between arrival and resumed trip
            g_iIncludeTripStops = 1;
            if (StopMinutes == 9999) {
                g_iIncludeTripStops = 0;
                StopMinutes = 0;
            }
            ArrTime.ParseDateTime(m_ArrTime);
            wxDateTime TripStopTime;
            TripStopTime = ArrTime;
            wxTimeSpan TripStopDelay(0,StopMinutes,0);
            TripStopTime = TripStopTime.Add(TripStopDelay);
            m_ResumeAfter = TripStopTime.FormatISOCombined(' ');

            // Trip resumed, set new TripMode and write data to disk
            TripMode = 5;
            saveResTime = 1;
            saveTripMode = 1;
            WriteTripData();
        }
   }

    strDep = m_DepTime;
    strDep = strDep.Truncate(16);  // Cut seconds for instrument

    if (DepTimeShow == 0) {
        strDep = "---";    // Trip reset selected
    }

    // Arrival and Resume times
    // Reset trip button pressed
    if (UseSavedArrTime == 0) {
        ArrTime = LocalTime;
        ResTime = LocalTime;
        UseSavedArrTime = 1;
        ReadArrFromData = 0;
        ArrTimeShow = 0;
    }

    // Arrival detected and logged
    if ((CurrSpeed < OnRouteSpeed) && (PowerUpActive == 0))  {

        // Write trip mode and arrival info to log file
        if ((TripMode == 1) || (TripMode == 3))  {

            if (TripMode == 1)  TripMode = 2;   // Arrived after departure
            if (TripMode == 3)  TripMode = 4;   // Arrived after resumed trip

            // Save to data log file
            saveArrTime = 1;
            saveTripMode = 1;
            WriteTripData();
        }
    }

    strArr = m_ArrTime;
    strArr = strArr.Truncate(16);  // Cut seconds
    if (CurrSpeed > OnRouteSpeed) {
        ArrTimeShow = 1;
        strArr = _("On Route");
    }
    if (ArrTimeShow == 0) strArr = "---";    // Trip reset selected

    // Read updated distances, times and TripMode from data direcrory
    if ((ReadSavedData == 1) && (m_DataFileTextReader.Open(m_DataFile))) {

        if (ReadTripFromData == 1)  {
            m_dataTripDist = m_DataFileTextReader.GetLine(0);
            m_dataTripDist.ToDouble(&dataTripDist);   // From data save
            m_confTripDist.ToDouble(&TripDist);       // From conf file

            // Use TripDist from conf file if higher (user updated value)
            if (dataTripDist >= TripDist) {
                TripDist = dataTripDist;
                saveTripDist = 1;
            }
            ReadTripFromData = 0;
        }

        if (ReadSumlogFromData == 1)  {
            m_dataTotDist = m_DataFileTextReader.GetLine(1);
            m_dataTotDist.ToDouble(&dataTotDist);  // From data save
            m_confTotDist.ToDouble(&TotDist);      // From conf file

            // Use TotDist from conf file if higher (user updated value)
            if (dataTotDist >= TotDist) {
                TotDist = dataTotDist;
                saveTotDist = 1;
            }
            ReadSumlogFromData = 0;
        }

        if (ReadDepFromData == 1)  {
            m_DepTime = m_DataFileTextReader.GetLine(2);
            ReadDepFromData = 0;
        }

        m_ArrTime = m_DataFileTextReader.GetLine(3);
        ReadArrFromData = 0;

        if (ReadResFromData == 1)  {
            m_ResTime = m_DataFileTextReader.GetLine(4);
            if (m_ResTime < m_DepTime) m_ResTime = m_DepTime;
            ReadResFromData = 0;
        }

        if (ReadTripModeFromData == 1)  {
            strTripMode = m_DataFileTextReader.GetLine(5);
            TripMode = wxAtoi(strTripMode);

            // Ensure TripMode has a correct value (0, 2 or 4)
            if (TripMode == 1) TripMode = 2;
            if ((TripMode == 3) || (TripMode >= 5)) TripMode = 4;
            ReadTripModeFromData = 0;
        }

        if (ReadTripAutoResetFromData == 1)  {
            strTripAutoReset = m_DataFileTextReader.GetLine(6);
            AutoReset = wxAtoi(strTripAutoReset);
            ReadTripAutoResetFromData = 0;
        }

        m_DataFileTextReader.Close();
        ReadSavedData = 0;
    }


    // Trip Distance, reset trip button pressed
    if (UseSavedTrip == 0) {
        TripDist = 0.0;
        UseSavedTrip = 1;
        ReadTripFromData = 0;
    }

    GetDistance();

    //  Are at start randomly getting extreme values for distance even if validGPS
    //  is ok.
    //  Delay a minimum of 5 seconds at power up to allow everything to be properly
    //  set before measuring distances

    wxDateTime PwrTime;    // Dummy for proper EnabledTime function

    if (Initializing == 1) {   // Set in odometer_pi.h
        EnabledTime = LocalTime;
        int PwrOnDelaySecs = atoi(m_PwrOnDelSecs);
        if (PwrOnDelaySecs <= 4) PwrOnDelaySecs = 5;
        wxTimeSpan PwrOnDelay(0,0,PwrOnDelaySecs);
        wxDateTime PwrTime = EnabledTime.Add(PwrOnDelay);
        Initializing = 0;
        ArrTimeShow = 1;
        ReadSavedData = 1;
        ReadSumlogFromData = 1;
        ReadTripFromData = 1;
        ReadDepFromData = 1;
        ReadArrFromData = 1;
        ReadResFromData = 1;
        ReadTripModeFromData = 1;
        AutoReset = 1;
        ReadTripAutoResetFromData = 1;
    }

    PowerUpActive = 1;
    if (EnabledTime > LocalTime) {  // Power up time delay elapsed
        StepDist = 0.0;
        PowerUpActive = 1;
    } else {
        PowerUpActive = 0;  // Power up time not yet elapsed 
    }

    // Total distance (sumlog)
    oldTotDist = TotDist;
    TotDist = (TotDist + StepDist);

    // First decimal changed? Then save the current value
    if (trunc(TotDist * 10) > trunc(oldTotDist * 10)) {
        saveTotDist = 1;
        WriteTripData();
    }

    // Generate string info, only one decimal,
    // and remove eventual whitespace before and after
    // Need to go this way to avoid rounding error and trailing zeroes
    m_TotDist = " ";
    m_TotDist.Printf("%f",trunc(TotDist*10)/10); // Cut to one decimal
    m_TotDist.ToDouble(&dispTotDist);
    m_TotDist.Printf("%.1f", dispTotDist); // Cut trailing zeroes
    m_TotDist.Trim(0);
    m_TotDist.Trim(1);

    // Trip distance
    oldTripDist = TripDist;
    TripDist = (TripDist + StepDist);

    // First decimal changed? Then save the current value
    if (trunc(TripDist * 10) > trunc(oldTripDist * 10)) {
        saveTripDist = 1;
        WriteTripData();
    }

    // Generate string info, only one decimal,
    // and remove eventual whitespace before and after
    // Need to go this way to avoid rounding error and trailing zeroes
    m_TripDist = " ";
    m_TripDist.Printf("%f",trunc(TripDist*10)/10); // Zero all but first decimal
    m_TripDist.ToDouble(&dispTripDist);
    m_TripDist.Printf("%.1f", dispTripDist);  // Cut trailing zeroes
    m_TripDist.Trim(0);
    m_TripDist.Trim(1);

    // Leg Trip
    // Toggle leg counter and button text
    if (g_iStartStopLeg == 1) {
        if (CountLeg == 1) {
            CountLeg = 0;  // Counter paused
        } else {
            CountLeg = 1;
        }
        g_iStartStopLeg = 0;
    }

    if (g_iShowTripLeg != 1) {   // stop to avoid overcount
        LegDist = 0.0;
        LegStart = LocalTime;
    }

    // Count leg distance and time
    if (CountLeg == 1) {
        LegDist = (LegDist + StepDist);
        LegTime = LocalTime.Subtract(LegStart);
    } else {
        LegStart = LocalTime.Subtract(LegTime);
    }

    // Generate string info, use two decimals,
    // and remove eventual whitespace before and after
    // Need to go this way to avoid rounding error and trailing zeroes
    m_LegDist = " ";
    m_LegDist.Printf("%f",trunc(LegDist*100)/100); // Zero all but two first decimals
    m_LegDist.ToDouble(&dispLegDist);
    m_LegDist.Printf("%.2f", dispLegDist);  // Cut trailing zeroes
    m_LegDist.Trim(0);
    m_LegDist.Trim(1);

    m_LegTime = LegTime.Format("%H:%M:%S");
    wxString strLegTime;
    strLegTime = LegTime.Format("%H:%M:%S");


    SendSentenceToAllInstruments(OCPN_DBP_STC_DEPART, ' ' , strDep );
    SendSentenceToAllInstruments(OCPN_DBP_STC_ARRIV, ' ' , strArr );
    SendSentenceToAllInstruments(OCPN_DBP_STC_SUMLOG, TotDist , DistUnit );
    SendSentenceToAllInstruments(OCPN_DBP_STC_TRIPLOG, TripDist , DistUnit );
    SendSentenceToAllInstruments(OCPN_DBP_STC_LEGDIST, LegDist , DistUnit );
    SendSentenceToAllInstruments(OCPN_DBP_STC_LEGTIME, ' ' , strLegTime );

    TripAutoReset(strArr, LocalTime);
}

void odometer_pi::DefineTripData() {

    // Define data file location, ensure it exists or add default data
    m_DataFile.Clear();
    m_DataFile = g_sDataDir + _T("datalog.log");
    size_t logfilelines;
    logfilelines = 0;
    int datalogOK = 0;

    if (m_DataFileTextReader.Open(m_DataFile))  { 
        logfilelines = (m_DataFileTextReader.GetLineCount());
        m_DataFileTextReader.Close();   // File exists, close it again  
        if (logfilelines >= 8)  {
            datalogOK = 1;
        } else {
            remove(m_DataFile);  // File likely corrupt, remove it
        }
    }

    if (datalogOK == 0) {    // Create datalog file with default data
        m_DataFileTextReader.Create(m_DataFile);
        m_DataFileTextReader.Open(m_DataFile);
        m_DataFileTextReader.AddLine("0.0");
        m_DataFileTextReader.AddLine("0.0");
        m_DataFileTextReader.AddLine("2000-01-01 00:00:00");
        m_DataFileTextReader.AddLine("2000-01-01 00:00:00");
        m_DataFileTextReader.AddLine("2000-01-01 00:00:00");
        m_DataFileTextReader.AddLine("0");
        m_DataFileTextReader.AddLine("0");
        m_DataFileTextReader.AddLine("##### Trip log below this line #####");
        m_DataFileTextReader.Write();
        m_DataFileTextReader.Close();

        // If this is an update then remove no longer relevant files, one by one
        // Total distance is fetched from opencpn configuration upon start
        oldLogFile.Clear();
        oldLogFile = g_sDataDir + _T("arrival.log");
        rmOldLogFiles();
        oldLogFile = g_sDataDir + _T("arrtime.log");
        rmOldLogFiles();
        oldLogFile = g_sDataDir + _T("departure.log");
        rmOldLogFiles();
        oldLogFile = g_sDataDir + _T("deptime.log");
        rmOldLogFiles();
        oldLogFile = g_sDataDir + _T("resume.log");
        rmOldLogFiles();
        oldLogFile = g_sDataDir + _T("sumlog.log");
        rmOldLogFiles();
        oldLogFile = g_sDataDir + _T("totaldist.log");
        rmOldLogFiles();
        oldLogFile = g_sDataDir + _T("tripdist.log");
        rmOldLogFiles();
        oldLogFile = g_sDataDir + _T("triplog.log");
        rmOldLogFiles();
        oldLogFile = g_sDataDir + _T("tripmode.log");
        rmOldLogFiles();
    }
}


void odometer_pi::rmOldLogFiles() {

    // Need delays for slow CPU's using SSD disks (Win crashes)
    wxFileName logfile(oldLogFile);
    if (m_oldLogFileName.Open(oldLogFile)) {
        m_oldLogFileName.Close();
        wxMilliSleep(50);
        wxRemoveFile(oldLogFile);
        wxMilliSleep(100);
    }
    wxMilliSleep(50);
}


void odometer_pi::WriteTripData() {

    // Write updated trip data to data direcrory,
    if (m_DataFileTextReader.Open(m_DataFile)) {

        if (saveTripDist == 1)  {
            wxString writeTripDist;
            writeTripDist.Printf("%f", TripDist);
            m_DataFileTextReader.GetLine(0) = writeTripDist;
            saveTripDist = 0;
        }

        if (saveTotDist == 1)  {
            wxString writeTotDist;
            writeTotDist.Printf("%f", TotDist);
            m_DataFileTextReader.GetLine(1) = writeTotDist;
            saveTotDist = 0;
        }

        if (saveDepTime == 1)  {  // Called at trip reset
            m_DataFileTextReader.GetLine(2) = m_DepTime;
            // Check if any departures in log file are older than one month,
            // then remove those lines
            if (g_iLogFileAge == 1) {

                // Set reference time 1 month back
                wxDateTime AgeTime = LocalTime;
                wxDateSpan AgeTimeLimit(0,1,0);
                AgeTime = AgeTime.Subtract(AgeTimeLimit);
                wxString m_AgeTime = AgeTime.FormatISOCombined(' ');
                m_AgeTime = m_AgeTime.Truncate(16);  // 16 characters in log file

                wxString DepLogAge;
                int OldDepartRemove = 1;

                // Get the departure time only and remove the first line if to old
                while (OldDepartRemove == 1)  {
                    DepLogAge = m_DataFileTextReader.GetLine(8);
                    DepLogAge = DepLogAge.Mid(3,16);

                    if (DepLogAge < m_AgeTime) {
                        m_DataFileTextReader.RemoveLine(8);
                    } else {
                        OldDepartRemove = 0;
                    }
                }
                g_iLogFileAge = 0;
            }
            // Add a new line for departure
            DepTime = LocalTime;
            m_DepTime = DepTime.FormatISOCombined(' ');
            logDepTime = m_DepTime;
            logDepTime = logDepTime.Truncate(16);
            m_DataFileTextReader.AddLine("D: " + logDepTime);
            saveDepTime = 0;
        }

        if (saveArrTime == 1)  {
            ArrTime = LocalTime;
            m_ArrTime = ArrTime.FormatISOCombined(' ');
            ArrTimeShow = 1;
            m_DataFileTextReader.GetLine(3) = m_ArrTime;

            // Add arrival and trip data
            logArrTime = m_ArrTime;
            logArrTime = logArrTime.Truncate(16);
            wxString arrLog(", A: " + logArrTime);
            wxString tripLog(", T: " + m_TripDist);
            currLogLine.Clear();
            currLogLine = m_DataFileTextReader.GetLastLine();
            m_DataFileTextReader.GetLastLine() = (currLogLine + arrLog + tripLog);
            saveArrTime = 0;
        }

        if (saveResTime == 1)  {
            if (TripMode == 5)   {

                // Include trip stops in logfile
                if (((g_iIncludeTripStops == 1) && (m_LocalTime <= m_ResumeAfter))
                    || (g_iIncludeTripStops == 0))  {
                    TripMode = 6;

                    // Arrival is 21 characters while trip distance is 3 to 7 characters
                    // plus 5 charaters as trip tag, all at he end of the current log info.
                    // Read line and find current length
                    int lengthTripString = 0;
                    lengthTripString = m_TripDist.Len();
                    int CharsToRemove = (21 + lengthTripString + 5);
                    currLogLine.Clear();
                    currLogLine = m_DataFileTextReader.GetLastLine();

                    // Starting or stopping while on route could cause time errors
                    // Also ensure there are enough characters in the file
                    // Strip required characters from end  and write back
                    if ((m_ArrTime < m_LocalTime) && (currLogLine.Len() > 40))  {
                        m_DataFileTextReader.GetLastLine() = (currLogLine.Mid(0,
                        (currLogLine.Len() - CharsToRemove)));
                    }
                } else {

                    // Trip arrival shall not be removed, add resume time
                    ResTime = LocalTime;
                    m_ResTime = ResTime.FormatISOCombined(' ');
                    logResTime = m_ResTime;
                    logResTime = logResTime.Truncate(16);

                    wxString resLog(", R: " + logResTime);
                    currLogLine.Clear();
                    currLogLine = m_DataFileTextReader.GetLastLine();
                    m_DataFileTextReader.GetLastLine() = (currLogLine + resLog);
                }
                m_DataFileTextReader.GetLine(4) = m_ResTime;
    }
            TripMode = 3;
            saveTripMode = 1;
            saveResTime = 0;
        }

        if (saveTripMode == 1)  {
            strTripMode = std::to_string(TripMode);
            m_DataFileTextReader.GetLine(5) = strTripMode;
            saveTripMode = 0;
        }

        if (saveTripAutoReset == 1)  {
            strTripAutoReset = std::to_string(AutoReset);
            m_DataFileTextReader.GetLine(6) = strTripAutoReset;
            saveTripAutoReset = 0;
        }

        m_DataFileTextReader.Write();
        wxMilliSleep(50);     // Required by slow systems with SSD disks (?)
        m_DataFileTextReader.Close();
    }
}

void odometer_pi::GetDistance() {

    wxDateTime DistTime = LocalTime;

    switch (g_iOdoDistanceUnit) {
        case 0:
            DistDiv = 3600;
            DistUnit = "M";
            break;
        case 1:
            DistDiv = 3128;
            DistUnit = "miles";
            break;
        case 2:
            DistDiv = 1944;
            DistUnit = "km";
            break;
    }

    CurrSec = wxAtoi(DistTime.Format(wxT("%S")));

    // Calculate distance travelled during the elapsed time
    StepDist = 0.0;

    if (CurrSec != PrevSec) {
        if (CurrSec > PrevSec) {
            SecDiff = (CurrSec - PrevSec);
        } else {
            PrevSec = (PrevSec - 58);  // Is this always ok no matter GPS update rates?
        }
        StepDist = (SecDiff * (CurrSpeed/DistDiv));

        if (std::isnan(StepDist)) StepDist = 0.0;
    }
    PrevSec = CurrSec;
}


void odometer_pi::TripAutoReset(wxString strArr, wxDateTime LocalTime) {
    // Automatic Trip Reset
    if ((g_iAutoResetTrip == 1) && (AutoReset == 1)) {

        switch (g_iAutoResetTripTime) {  // Calculate time selected
            case 0: 
                ResetDelay = 15;
                break;
            case 1:
                ResetDelay = 30;
                break;
            case 2:
                ResetDelay = 60;
                break;
            case 3:
                ResetDelay = 120;
                break;
            case 4:
                ResetDelay = 180;
                break;
            case 5:
                ResetDelay = 240;
                break;
            case 6:
                ResetDelay = 300;
                break;
            case 7:
                ResetDelay = 360;
                break;
            case 8:
                ResetDelay = 540;
                break;
            case 9:
                ResetDelay = 720;
                break;
        }
        wxTimeSpan m_ResetDelay(0,ResetDelay,0);

        wxDateTime ArrRefTime;
        ArrRefTime.ParseDateTime(m_ArrTime.Truncate(16));
        wxDateTime ArrResetTime = ArrRefTime.Add(m_ResetDelay);
        strReset = ArrResetTime.FormatISOCombined(' ');
        strReset = strReset.Truncate(16);  // Cut seconds

        wxDateTime ResetTime = LocalTime;
        strLocal = ResetTime.Format(wxT("%F %R"));
        if (strLocal >= strReset) g_iResetTrip = 1;

        AutoReset = 0;
        saveTripAutoReset = 1;
        WriteTripData();
    }
}


void odometer_pi::SetPluginMessage(wxString &message_id, wxString &message_body)  {

    if(message_id == _T("WMM_VARIATION_BOAT")) {
        // construct the JSON root object
        wxJSONValue  root;
        // construct a JSON parser
        wxJSONReader reader;

        // now read the JSON text and store it in the 'root' structure
        // check for errors before retreiving values...
        int numErrors = reader.Parse( message_body, &root );
        if ( numErrors > 0 )  {
            //              const wxArrayString& errors = reader.GetErrors();
            return;
        }

        // get the DECL value from the JSON message
        wxString decl = root[_T("Decl")].AsString();
        double decl_val;
        decl.ToDouble(&decl_val);
    } else if(message_id == _T("OCPN_CORE_SIGNALK")) {
        ParseSignalK( message_body);
    }
}

// Not sure what this does, I guess we only install one toolbar item?? It is however required.
int odometer_pi::GetToolbarToolCount(void) {
    return 1;
}

//---------------------------------------------------------------------------------------------------------
//
// Odometer Settings Dialog
//
//---------------------------------------------------------------------------------------------------------

void odometer_pi::ShowPreferencesDialog(wxWindow* parent) {
	OdometerPreferencesDialog *dialog = new OdometerPreferencesDialog(
        parent, wxID_ANY, m_ArrayOfOdometerWindow);

	if (dialog->ShowModal() == wxID_OK) {

        double scaler = 1.0;
        if (OCPN_GetWinDIPScaleFactor() < 1.0)
        scaler = 1.0 + OCPN_GetWinDIPScaleFactor() / 4;
        scaler = wxMax(1.0, scaler);

        // Used for title texts on grayed areas, on buttons and trip autoreset
        g_pUSFontTitle = new wxFont(dialog->m_pFontPickerTitle->GetSelectedFont());
        g_FontTitle = g_pUSFontTitle->Scaled(scaler);
        g_USFontTitle = *g_pUSFontTitle;

        // Data texts are used for displayed data (times and distances)
        g_pUSFontData = new wxFont(dialog->m_pFontPickerData->GetSelectedFont());
        g_FontData = g_pUSFontData->Scaled(scaler);
        g_USFontData = *g_pUSFontData;

        // Label tests are used for digital speed in speedometer
        g_pUSFontLabel = new wxFont(dialog->m_pFontPickerLabel->GetSelectedFont());
        g_FontLabel = g_pUSFontLabel->Scaled(scaler);
        g_USFontLabel = *g_pUSFontLabel;

        // Small tests are used for speedometer values
        g_pUSFontSmall = new wxFont(dialog->m_pFontPickerSmall->GetSelectedFont());
        g_FontSmall = g_pUSFontSmall->Scaled(scaler);
        g_USFontSmall = *g_pUSFontSmall;


        /* Instrument visibility is not detected by ApplyConfig as no instuments are 
           added, reordered or deleted. 
           Globals are not checked at all by ApplyConfig.  */

        bool showSpeedDial = dialog->m_pCheckBoxShowSpeed->GetValue();
        g_iShowSpeed = 0;
        if (showSpeedDial == true) g_iShowSpeed = 1;

        bool showDepArrTimes = dialog->m_pCheckBoxShowDepArrTimes->GetValue();
        g_iShowDepArrTimes = 0;
        if (showDepArrTimes == true) g_iShowDepArrTimes = 1;

        bool showTripLeg = dialog->m_pCheckBoxShowTripLeg->GetValue();
        g_iShowTripLeg = 0;
        if (showTripLeg == true) g_iShowTripLeg = 1;

		// OnClose should handle that for us normally but it doesn't seems to do so
		// We must save changes first
        dialog->SaveOdometerConfig();
        m_ArrayOfOdometerWindow.Clear();
        m_ArrayOfOdometerWindow = dialog->m_Config;

		ApplyConfig();
		SaveConfig();   // TODO BUG: Does not save configuration file
		SetToolbarItemState(m_toolbar_item_id, GetOdometerWindowShownCount() != 0);
	}
    // Invoke the dialog destructor
	dialog->Destroy();
}


void odometer_pi::SetColorScheme(PI_ColorScheme cs) {

    OdometerWindow *odometer_window = m_ArrayOfOdometerWindow.Item(0)->m_pOdometerWindow;
    if (odometer_window) {
		odometer_window->SetColorScheme(cs);
	}
}

int odometer_pi::GetToolbarItemId() {
	return m_toolbar_item_id;
}

int odometer_pi::GetOdometerWindowShownCount() {

    int cnt = 0;

    OdometerWindow *odometer_window = m_ArrayOfOdometerWindow.Item(0)->m_pOdometerWindow;
    if (odometer_window) {
        wxAuiPaneInfo &pane = m_pauimgr->GetPane(odometer_window);
        if (pane.IsOk() && pane.IsShown()) cnt++;
    }
    return cnt;
}

void odometer_pi::OnPaneClose(wxAuiManagerEvent& event) {

    // if name is unique, we should use it
    OdometerWindow *odometer_window = (OdometerWindow *) event.pane->window;

    int cnt = 0;
    OdometerWindowContainer *cont = m_ArrayOfOdometerWindow.Item(0);
    OdometerWindow *d_w = cont->m_pOdometerWindow;
    if (d_w) {
        // we must not count this one because it is being closed
        if (odometer_window != d_w) {
            wxAuiPaneInfo &pane = m_pauimgr->GetPane(d_w);
            if (pane.IsOk() && pane.IsShown()) cnt++;
        } else {
            cont->m_bIsVisible = false;
        }
    }
    SetToolbarItemState(m_toolbar_item_id, cnt != 0);

    event.Skip();
}

void odometer_pi::OnToolbarToolCallback(int id) {

    int cnt = GetOdometerWindowShownCount();
    bool b_anyviz = false;   // ???
    for (size_t i = 0; i < m_ArrayOfOdometerWindow.GetCount(); i++) {
        OdometerWindowContainer *cont = m_ArrayOfOdometerWindow.Item(i);
        if (cont->m_bIsVisible) {
            b_anyviz = true;
            break;   // This must be handled before removing the for statement
        }
    }

    OdometerWindowContainer *cont = m_ArrayOfOdometerWindow.Item(0);
    OdometerWindow *odometer_window = cont->m_pOdometerWindow;

    if (odometer_window) {
        wxAuiPaneInfo &pane = m_pauimgr->GetPane(odometer_window);
        if (pane.IsOk()) {
            bool b_reset_pos = false;

        #ifdef __WXMSW__
            //  Support MultiMonitor setups which an allow negative window positions.
            //  If the requested window title bar does not intersect any installed monitor,
            //  then default to simple primary monitor positioning.
            RECT frame_title_rect;
            frame_title_rect.left = pane.floating_pos.x;
            frame_title_rect.top = pane.floating_pos.y;
            frame_title_rect.right = pane.floating_pos.x + pane.floating_size.x;
            frame_title_rect.bottom = pane.floating_pos.y + 30;

			if (NULL == MonitorFromRect(&frame_title_rect, MONITOR_DEFAULTTONULL)) {
				b_reset_pos = true;
			}
        #else

            //    Make sure drag bar (title bar) of window intersects wxClient Area of
            //    screen, with a little slop...
            wxRect window_title_rect;// conservative estimate
            window_title_rect.x = pane.floating_pos.x;
            window_title_rect.y = pane.floating_pos.y;
            window_title_rect.width = pane.floating_size.x;
            window_title_rect.height = 30;

            wxRect ClientRect = wxGetClientDisplayRect();
            // Prevent the new window from being too close to the edge
            // ClientRect.Deflate(60, 60);
 			if (!ClientRect.Intersects(window_title_rect)) {
				b_reset_pos = true;
			}
        #endif

			if (b_reset_pos) {
				pane.FloatingPosition(50, 50);
			}

            if (cnt == 0)
                if (b_anyviz)
                    pane.Show(cont->m_bIsVisible);
                else {
                   cont->m_bIsVisible = cont->m_bPersVisible;
                   pane.Show(cont->m_bIsVisible);
                }
            else
                pane.Show(false);
        }

        //  This patch fixes a bug in wxAUIManager
        //  FS#548
        // Dropping a Odometer Window right on top on the (supposedly fixed) chart bar
        //  window causes a resize of the chart bar, and the Odometer window assumes some
        //  of its properties
        // The Odometer window is no longer grabbable...
        // Workaround:  detect this case, and force the pane to be on a different Row.
        // so that the display is corrected by toggling the odometer off and back on.
        if ((pane.dock_direction == wxAUI_DOCK_BOTTOM) && pane.IsDocked()) pane.Row(2);
    }
    // Toggle is handled by the toolbar but we must keep plugin manager b_toggle updated
    // to actual status to ensure right status upon toolbar rebuild
    SetToolbarItemState(m_toolbar_item_id, GetOdometerWindowShownCount() != 0);
    m_pauimgr->Update();
}

void odometer_pi::UpdateAuiStatus(void) {

    // This method is called after the PlugIn is initialized
    // and the frame has done its initial layout, possibly from a saved wxAuiManager
    // "Perspective"
    // It is a chance for the PlugIn to syncronize itself internally with the state of
    // any Panes that were added to the frame in the PlugIn ctor.

    OdometerWindowContainer *cont = m_ArrayOfOdometerWindow.Item(0);
    wxAuiPaneInfo &pane = m_pauimgr->GetPane(cont->m_pOdometerWindow);
    // Initialize visible state as perspective is loaded now
    cont->m_bIsVisible = (pane.IsOk() && pane.IsShown());

#ifdef __WXQT__
    if (pane.IsShown()) {
        pane.Show(false);
        m_pauimgr->Update();
        pane.Show(true);
        m_pauimgr->Update();
    }
#endif

    m_pauimgr->Update();

    // We use this callback here to keep the context menu selection in sync with the
    // window state
    SetToolbarItemState(m_toolbar_item_id, GetOdometerWindowShownCount() != 0);
}

// Load a saved configuration
bool odometer_pi::LoadConfig(void) {
    wxFileConfig *pConf = (wxFileConfig *) m_pconfig;

    if (pConf) {
        pConf->SetPath(_T("/PlugIns/GPS-Odometer"));

        wxString version;
        pConf->Read(_T("Version"), &version, wxEmptyString);
		wxString config;

        // Set some sensible defaults
        wxString TitleFont;
        wxString DataFont;
        wxString LabelFont;
        wxString SmallFont;

        TitleFont = g_pFontTitle->GetNativeFontInfoDesc();
        DataFont = g_pFontData->GetNativeFontInfoDesc();
        LabelFont = g_pFontLabel->GetNativeFontInfoDesc();
        SmallFont = g_pFontSmall->GetNativeFontInfoDesc();

        double scaler = 1.0;
        if (OCPN_GetWinDIPScaleFactor() < 1.0)
            scaler = 1.0 + OCPN_GetWinDIPScaleFactor()/4;
        scaler = wxMax(1.0, scaler);

        pConf->Read(_T("FontTitle"), &config, TitleFont);
        LoadFont(&g_pUSFontTitle, config);
        g_FontTitle = g_pUSFontTitle->Scaled(scaler);
        g_pFontTitle = &g_FontTitle;

        g_pFontData = &g_FontData;
        pConf->Read(_T("FontData"), &config, DataFont);
        LoadFont(&g_pUSFontData, config);
        g_FontData = g_pUSFontData->Scaled(scaler);

        g_pFontLabel = &g_FontLabel;
        pConf->Read(_T("FontLabel"), &config, LabelFont);
        LoadFont(&g_pUSFontLabel, config);
        g_FontLabel = g_pUSFontLabel->Scaled(scaler);

        g_pFontSmall = &g_FontSmall;
        pConf->Read(_T("FontSmall"), &config, SmallFont);
        LoadFont(&g_pUSFontSmall, config);
        g_FontSmall = g_pUSFontSmall->Scaled(scaler);

		// Load odometer settings plus set default values
        pConf->Read( _T("TotalDistance"), &m_confTotDist, "0.0");
        pConf->Read( _T("TripDistance"), &m_confTripDist, "0.0");
        pConf->Read( _T("AutoResetTrip"), &g_iAutoResetTrip, 0);
        pConf->Read( _T("AutoResetTripTime"), &g_iAutoResetTripTime, 7);
        pConf->Read( _T("PowerOnDelaySecs"), &m_PwrOnDelSecs, "15");
        pConf->Read( _T("SatsRequired"), &m_SatsRequired, "4");
        pConf->Read( _T("HDOP"), &m_HDOPdefine, "4");
        pConf->Read( _T("DepartureTime"), &m_DepTime, "2020-01-01 00:00:00");
        pConf->Read( _T("ArrivalTime"), &m_ArrTime, "2020-01-01 00:00:00");
        pConf->Read( _T("FilterSpeed"), &m_FilterSOG, "1");

        pConf->Read(_T("Protocol"), &g_iOdoProtocol, NMEA_0183);
        pConf->Read(_T("SpeedometerMax"), &g_iOdoSpeedMax, 12);
        pConf->Read(_T("OnRouteSpeedLimit"), &g_iOdoOnRoute, 2);
        pConf->Read(_T("UTCOffset"), &g_iOdoUTCOffset, 24 );
        pConf->Read(_T("SpeedUnit"), &g_iOdoSpeedUnit, SPEED_KNOTS);
        pConf->Read(_T("DistanceUnit"), &g_iOdoDistanceUnit, DISTANCE_NAUTICAL_MILES);
        pConf->Read(_T("TripStopMinutes"), &g_iTripStopMinutes, 5);
        pConf->Read(_T("NumLogTrips"), &g_iSelectLogTrips, 1);
        pConf->Read(_T("LogFormat"), &g_iSelectLogFormat, 2);
        pConf->Read(_T("LogOutput"), &g_iSelectLogOutput, 1);

        int d_cnt = 0;

        // Memory leak? We should destroy everything first
        m_ArrayOfOdometerWindow.Clear();
        if (version.IsEmpty() && d_cnt == -1) {
           m_config_version = 1;
            // Let's load version 1 or default settings.
            int i_cnt;
            pConf->Read(_T("InstrumentCount"), &i_cnt, -1);
            wxArrayInt ar;
            if (i_cnt != -1) {
                for (int i = 0; i < i_cnt; i++) {
                    int id;
                    pConf->Read(wxString::Format(_T("Instrument%d"), i + 1), &id, -1);
                    if (id != -1) ar.Add(id);
                }
            } else {
                // Load the default instrument list, do not change this order!
                // Non-default instruments are left out
                ar.Add( ID_DBP_I_SUMLOG );
                ar.Add( ID_DBP_I_TRIPLOG );
                ar.Add( ID_DBP_I_DEPART );
                ar.Add( ID_DBP_I_ARRIV );
                ar.Add( ID_DBP_B_TRIPRES );
                ar.Add( ID_DBP_C_AUTORESET );
            }
      
	        // Generate a named GUID for the odometer container
            OdometerWindowContainer *cont = new OdometerWindowContainer(
                NULL, MakeName(), _("GPS Odometer"), _T("V"), ar);
            m_ArrayOfOdometerWindow.Add(cont);
            cont->m_bPersVisible = true;

        } else {
           // Configuration Version 2
            m_config_version = 2;
            bool b_onePersisted = false;
         
            wxString name;
            pConf->Read(_T("Name"), &name, MakeName());
            wxString caption;
            pConf->Read(_T("Caption"), &caption, _("Odometer"));
            wxString orient;
            pConf->Read(_T("Orientation"), &orient, _T("V"));
            int i_cnt;
            pConf->Read(_T("InstrumentCount"), &i_cnt, -1);
            bool b_persist;
            pConf->Read(_T("Persistence"), &b_persist, 0);
            bool b_speedo;
            pConf->Read( _T("ShowSpeedometer"), &b_speedo, 1) ;
            bool b_deparr;
            pConf->Read( _T("ShowDepArrTimes"), &b_deparr, 1);
            bool b_tripleg;
            pConf->Read( _T("ShowTripLeg"), &b_tripleg, 0);
            bool b_autoresettrip;
            pConf->Read( _T("AutoResetTrip"), &b_autoresettrip, 1);
            bool b_incltripstops;
            pConf->Read( _T("InclTripStops"), &b_incltripstops, 0);
            int i_tripstopminutes;
            pConf->Read( _T("TripStopMinutes"), &i_tripstopminutes, 5);
            int i_selectlogtrips;
            pConf->Read(_T("NumLogTrips"), &i_selectlogtrips, 0);
            int i_logtripformat;
            pConf->Read(_T("LogFormat"), &i_logtripformat, 1);
            int i_logoutput;
            pConf->Read(_T("LogOutput"), &i_logoutput, 1);

            wxArrayInt ar;
            ar.Clear();
            if (b_speedo == true) ar.Add( ID_DBP_D_SOG );
            ar.Add( ID_DBP_I_SUMLOG );
            ar.Add( ID_DBP_I_TRIPLOG );
            if (b_deparr == true) {
                ar.Add( ID_DBP_I_DEPART );
                ar.Add( ID_DBP_I_ARRIV );
            }
            ar.Add( ID_DBP_B_TRIPRES );
            ar.Add( ID_DBP_C_AUTORESET );
            if (b_tripleg == true) {
                ar.Add( ID_DBP_I_LEGDIST );
                ar.Add( ID_DBP_I_LEGTIME );
                ar.Add( ID_DBP_B_STARTSTOP );
                ar.Add( ID_DBP_B_LEGRES );
            }

            OdometerWindowContainer *cont = 
                new OdometerWindowContainer(NULL, name, caption, orient, ar);

            cont->m_bPersVisible = b_persist;
            cont->m_bShowSpeed = b_speedo;
            cont->m_bShowDepArrTimes = b_deparr;
            cont->m_bShowTripLeg = b_tripleg;

            g_iShowSpeed = b_speedo;
            g_iShowDepArrTimes = b_deparr;
            g_iShowTripLeg = b_tripleg;

    		if (b_persist) b_onePersisted = true;
            m_ArrayOfOdometerWindow.Add(cont);

            // Make sure at least one odometer is scheduled to be visible
            if (m_ArrayOfOdometerWindow.Count() && !b_onePersisted){
                OdometerWindowContainer *cont = m_ArrayOfOdometerWindow.Item(0);
                if (cont) cont->m_bPersVisible = true;
            }
        }
        return true;
    } else {
        return false;
    }
}


void odometer_pi::LoadFont(wxFont **target, wxString native_info)
{
    if( !native_info.IsEmpty() ){
        (*target)->SetNativeFontInfo( native_info );
    }
}

bool odometer_pi::SaveConfig(void) {
    /* Does not save when called from 'odometer_pi::ShowPreferencesDialog' (or several
       other routines) but works correct when starting/stopping OpenCPN.  */

    wxFileConfig *pConf = (wxFileConfig *) m_pconfig;

    if (pConf) {
        pConf->SetPath(_T("/PlugIns/GPS-Odometer"));
        pConf->Write(_T("Version"), _T("2"));
        pConf->Write(_T("FontTitle"), g_pUSFontTitle->GetNativeFontInfoDesc());
        pConf->Write(_T("FontData"), g_pUSFontData->GetNativeFontInfoDesc());
        pConf->Write(_T("FontLabel"), g_pUSFontLabel->GetNativeFontInfoDesc());
        pConf->Write(_T("FontSmall"), g_pUSFontSmall->GetNativeFontInfoDesc());

        pConf->Write( _T("TotalDistance"), m_TotDist);
        pConf->Write( _T("TripDistance"), m_TripDist);
        pConf->Write( _T("AutoResetTrip"), g_iAutoResetTrip);
        pConf->Write( _T("AutoResetTripTime"), g_iAutoResetTripTime);
        pConf->Write( _T("PowerOnDelaySecs"), m_PwrOnDelSecs);
        pConf->Write( _T("SatsRequired"), m_SatsRequired);
        pConf->Write( _T("HDOP"), m_HDOPdefine);
        pConf->Write( _T("DepartureTime"), m_DepTime);
        pConf->Write( _T("ArrivalTime"), m_ArrTime);
        pConf->Write( _T("FilterSpeed"), m_FilterSOG);

        pConf->Write(_T("Protocol"), g_iOdoProtocol);
        pConf->Write(_T("SpeedometerMax"), g_iOdoSpeedMax);
        pConf->Write(_T("OnRouteSpeedLimit"), g_iOdoOnRoute);
        pConf->Write(_T("UTCOffset"), g_iOdoUTCOffset);
        pConf->Write(_T("SpeedUnit"), g_iOdoSpeedUnit);
        pConf->Write(_T("DistanceUnit"), g_iOdoDistanceUnit);

        // Only one Odometer Window
        pConf->Write(_T("OdometerCount"), 
            (int) m_ArrayOfOdometerWindow.GetCount());
            OdometerWindowContainer *cont = m_ArrayOfOdometerWindow.Item(0);
        pConf->Write(_T("Name"), cont->m_sName);
        pConf->Write(_T("Caption"), cont->m_sCaption);
        pConf->Write(_T("Orientation"), cont->m_sOrientation);
        pConf->Write(_T("Persistence"), cont->m_bPersVisible);
        pConf->Write(_T("InstrumentCount"),
                   (int)cont->m_aInstrumentList.GetCount());
        for (unsigned int j = 0; j < cont->m_aInstrumentList.GetCount(); j++)
            pConf->Write(wxString::Format(_T("Instrument%d"), j + 1),
                     cont->m_aInstrumentList.Item(j));
        pConf->Write(_T("ShowSpeedometer"), cont->m_bShowSpeed);
        pConf->Write(_T("ShowDepArrTimes"), cont->m_bShowDepArrTimes);
        pConf->Write(_T("ShowTripLeg"), cont->m_bShowTripLeg);
        pConf->Write(_T("TripStopMinutes"), g_iTripStopMinutes);
        pConf->Write(_T("NumLogTrips"), g_iSelectLogTrips);
        pConf->Write(_T("LogFormat"), g_iSelectLogFormat);
        pConf->Write(_T("LogOutput"), g_iSelectLogOutput);

        return true;
	} else {
		return false;
	}
}

// Load current odometer containers and their instruments
// Called at start and when preferences dialogue closes
void odometer_pi::ApplyConfig(void) {

    // Reverse order to handle deletes
    for (size_t i = m_ArrayOfOdometerWindow.GetCount(); i > 0; i--) {
        OdometerWindowContainer *cont = m_ArrayOfOdometerWindow.Item(i - 1);
        int orient = 0;  // Always vertical

        // A new odometer is created
        if(!cont->m_pOdometerWindow) {
            cont->m_pOdometerWindow = new OdometerWindow(
                GetOCPNCanvasWindow(), wxID_ANY, m_pauimgr, this, orient, cont);
            cont->m_pOdometerWindow->SetInstrumentList(cont->m_aInstrumentList);

            bool vertical = orient == wxVERTICAL;

            // Generate datalog.log and set decent valuese for default 
            // odometer pane for first time use, then fetch size
            DefineTripData(); 
            wxSize sz = cont->m_pOdometerWindow->GetMinSize();

            // Mac has a little trouble with initial Layout() sizing...
            #ifdef __WXOSX__
                if (sz.x == 0) sz.IncTo(wxSize(160, 388));
            #endif

            wxAuiPaneInfo p = wxAuiPaneInfo()
                                  .Name(cont->m_sName)
                                  .Caption(cont->m_sCaption)
                                  .CaptionVisible(false)
                                  .TopDockable(!vertical)
                                  .BottomDockable(!vertical)
                                  .LeftDockable(vertical)
                                  .RightDockable(vertical)
                                  .MinSize(sz)
                                  .BestSize(sz)
                                  .FloatingSize(sz)
                                  .FloatingPosition(100, 100)
                                  .Float()
                                  .Show(cont->m_bIsVisible)
                                  .Gripper(false);

            m_pauimgr->AddPane(cont->m_pOdometerWindow, p);
            //wxAuiPaneInfo().Name(cont->m_sName).Caption(cont->m_sCaption
            // ).CaptionVisible(false).TopDockable(
            // !vertical).BottomDockable(vertical).LeftDockable(vertical
            // ).RightDockable(vertical).MinSize( sz).BestSize(sz).FloatingSize(
            // sz).FloatingPosition(100, 100).Float().Show(cont->m_bIsVisible));

// #ifdef __OCPN__ANDROID__
            wxAuiPaneInfo& pane = m_pauimgr->GetPane( cont->m_pOdometerWindow );
//            pane = m_pauimgr->GetPane( cont->m_pOdometerWindow );
            pane.Dockable( false );
// #endif

        } else {
            // Odometer settings dialog -> OK is clicked.
            // The sizer used is just a number, not a correct wxSizer

            wxAuiPaneInfo& pane = m_pauimgr->GetPane(cont->m_pOdometerWindow);
            pane.Caption(cont->m_sCaption).Show(cont->m_bIsVisible);
            if (!cont->m_pOdometerWindow->isInstrumentListEqual(cont->m_aInstrumentList)) {
                // List of instruments is changed, update the current odometer 
                // and rebuild list of instruments from scratch

                // wxAuiManager requires properly defined sizers to update content
                // Panel need to be rebuilt
                m_pauimgr->DetachPane(cont->m_pOdometerWindow);

                cont->m_aInstrumentList.Clear();
                if (g_iShowSpeed == 1) {
                    cont->m_aInstrumentList.Add(0);
                }
                cont->m_aInstrumentList.Add(1);
                cont->m_aInstrumentList.Add(2);
                if (g_iShowDepArrTimes == 1) {
                    cont->m_aInstrumentList.Add(3);
                    cont->m_aInstrumentList.Add(4);
                }
                cont->m_aInstrumentList.Add(5);
                cont->m_aInstrumentList.Add(6);
                if (g_iShowTripLeg == 1) {
                    cont->m_aInstrumentList.Add(7);
                    cont->m_aInstrumentList.Add(8);
                    cont->m_aInstrumentList.Add(9);
                    cont->m_aInstrumentList.Add(10);
                }
                cont->m_pOdometerWindow->SetInstrumentList(cont->m_aInstrumentList);
                bool vertical = orient == wxVERTICAL;
                GetOCPNCanvasWindow(), wxID_ANY, m_pauimgr, this, orient, cont;
                wxSize sz = cont->m_pOdometerWindow->GetMinSize();

                // Mac has a little trouble with initial Layout() sizing...
                #ifdef __WXOSX__
                    if (sz.x == 0) sz.IncTo(wxSize(160, 388));
                #endif

                wxAuiPaneInfo p = wxAuiPaneInfo()
                                      .Name(cont->m_sName)
                                      .Caption(cont->m_sCaption)
                                      .CaptionVisible(false)
                                      .TopDockable(!vertical)
                                      .BottomDockable(!vertical)
                                      .LeftDockable(vertical)
                                      .RightDockable(vertical)
                                      .MinSize(sz)
                                      .BestSize(sz)
                                      .FloatingSize(sz)
                                      .FloatingPosition(100, 100)
                                      .Float()
                                      .Show(cont->m_bIsVisible)
                                      .Gripper(false);
                m_pauimgr->AddPane(cont->m_pOdometerWindow, p);
                m_pauimgr->Update();
            }
        }
        wxAuiPaneInfo& pane = m_pauimgr->GetPane(cont->m_pOdometerWindow);
        pane.Dockable( false );

        //  Enabling these lines causes resizing when OK is clicked, no matter
        //  if instrument list is updated or not
        // wxSize sz = cont->m_pOdometerWindow->GetMinSize();
        // pane.MinSize(sz).BestSize(sz).FloatingSize(sz);
    }
    m_pauimgr->Update();
}


void odometer_pi::ShowOdometer(size_t id, bool visible) {

    if (id < m_ArrayOfOdometerWindow.GetCount()) {
        OdometerWindowContainer *cont = m_ArrayOfOdometerWindow.Item(id);
        m_pauimgr->GetPane(cont->m_pOdometerWindow).Show(visible);
        cont->m_bIsVisible = visible;
        cont->m_bPersVisible = visible;
        m_pauimgr->Update();
    }
}


//---------------------------------------------------------------------------------------------------------
//
// Odometer Preferences Dialog
//
//---------------------------------------------------------------------------------------------------------

OdometerPreferencesDialog::OdometerPreferencesDialog(
    wxWindow *parent, wxWindowID id, wxArrayOfOdometer config) 
    :  wxDialog(parent, id, _("Odometer Settings"), wxDefaultPosition, 
           wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {

    int display_width, display_height;
    wxDisplaySize(&display_width, &display_height);

    Connect(wxEVT_CLOSE_WINDOW,
             wxCloseEventHandler(OdometerPreferencesDialog::OnCloseDialog), NULL, this);

    // Copy original config
    m_Config = wxArrayOfOdometer(config);
    // Build Odometer Page for Toolbox
    int border_size = 2;

    wxBoxSizer* itemBoxSizerMainPanel = new wxBoxSizer(wxVERTICAL);
    SetSizer(itemBoxSizerMainPanel);

    wxFlexGridSizer *itemFlexGridSizer = new wxFlexGridSizer(2);
    itemFlexGridSizer->AddGrowableCol(1);
    m_pPanelPreferences = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxBORDER_SUNKEN);
    itemBoxSizerMainPanel->Add(m_pPanelPreferences, 1, wxEXPAND | wxTOP | wxRIGHT,
        border_size);

    wxBoxSizer* itemBoxSizerMainFrame = new wxBoxSizer(wxVERTICAL);
    m_pPanelPreferences->SetSizer(itemBoxSizerMainFrame);

    wxStaticBox* itemStaticBoxDispOpts = new wxStaticBox(m_pPanelPreferences, wxID_ANY, _("Display options"));
    wxStaticBoxSizer* itemStaticBoxSizer03 = new wxStaticBoxSizer(itemStaticBoxDispOpts, wxHORIZONTAL);
    itemBoxSizerMainFrame->Add(itemStaticBoxSizer03, 1, wxEXPAND | wxALL, border_size);
    wxFlexGridSizer *itemFlexGridSizer01 = new wxFlexGridSizer(2);
    itemFlexGridSizer01->AddGrowableCol(0);
    itemStaticBoxSizer03->Add(itemFlexGridSizer01, 1, wxEXPAND | wxALL, 0);

    m_pCheckBoxIsVisible = new wxCheckBox(m_pPanelPreferences, wxID_ANY, _("Show this odometer"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer01->Add(m_pCheckBoxIsVisible, 0, wxEXPAND | wxALL, border_size);

    m_pCheckBoxShowSpeed = new wxCheckBox(m_pPanelPreferences, wxID_ANY, _("Show Speedometer instrument"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer01->Add(m_pCheckBoxShowSpeed, 0, wxEXPAND | wxALL, border_size);

    m_pCheckBoxShowDepArrTimes = new wxCheckBox(m_pPanelPreferences, wxID_ANY, _("Show Dep. and Arr. times"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer01->Add(m_pCheckBoxShowDepArrTimes, 0, wxEXPAND | wxALL, border_size);

    m_pCheckBoxShowTripLeg = new wxCheckBox(m_pPanelPreferences, wxID_ANY, _("Show/Reset Leg Distance and time"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer01->Add(m_pCheckBoxShowTripLeg, 0, wxEXPAND | wxALL, border_size);

    /* There must be an even number of checkboxes/objects preceeding caption or alignment
       gets messed up, enable the next section as required  */
//    wxStaticText *itemDummy01 = new wxStaticText(m_pPanelPreferences, wxID_ANY, _T(""));
//       itemFlexGridSizer01->Add(itemDummy01, 0, wxEXPAND | wxALL, border_size);

    wxStaticText* itemStaticText01 = new wxStaticText(m_pPanelPreferences, wxID_ANY, _("Caption:"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer01->Add(itemStaticText01, 0, wxEXPAND | wxALL, border_size);
    m_pTextCtrlCaption = new wxTextCtrl(m_pPanelPreferences, wxID_ANY, _T(""), wxDefaultPosition,
            wxDefaultSize);
    itemFlexGridSizer01->Add(m_pTextCtrlCaption, 0, wxEXPAND | wxALL, border_size);

    wxStaticBox* itemStaticBoxFonts = new wxStaticBox( m_pPanelPreferences, wxID_ANY, _("Fonts") );
    wxStaticBoxSizer* itemStaticBoxSizer04 = new wxStaticBoxSizer( itemStaticBoxFonts, wxHORIZONTAL );
    itemBoxSizerMainFrame->Add( itemStaticBoxSizer04, 0, wxEXPAND | wxALL, border_size );
    wxFlexGridSizer *itemFlexGridSizer02 = new wxFlexGridSizer( 2 );
    itemFlexGridSizer02->AddGrowableCol( 1 );
    itemStaticBoxSizer04->Add( itemFlexGridSizer02, 1, wxEXPAND | wxALL, 0 );
    itemBoxSizerMainFrame->AddSpacer( 5 );

    wxStaticText* itemStaticText02 = new wxStaticText(m_pPanelPreferences, wxID_ANY, _("Title:"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer02->Add(itemStaticText02, 0, wxEXPAND | wxALL, border_size);
    m_pFontPickerTitle = new wxFontPickerCtrl(m_pPanelPreferences, wxID_ANY, g_USFontTitle,
            wxDefaultPosition, wxDefaultSize);
    itemFlexGridSizer02->Add(m_pFontPickerTitle, 0, wxALIGN_RIGHT | wxALL, 0);

    wxStaticText* itemStaticText03 = new wxStaticText(m_pPanelPreferences, wxID_ANY, _("Data:"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer02->Add(itemStaticText03, 0, wxEXPAND | wxALL, border_size);
    m_pFontPickerData = new wxFontPickerCtrl(m_pPanelPreferences, wxID_ANY, g_USFontData,
            wxDefaultPosition, wxDefaultSize);
    itemFlexGridSizer02->Add(m_pFontPickerData, 0, wxALIGN_RIGHT | wxALL, 0);

    wxStaticText* itemStaticText04 = new wxStaticText(m_pPanelPreferences, wxID_ANY, _("Label:"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer02->Add(itemStaticText04, 0, wxEXPAND | wxALL, border_size);
    m_pFontPickerLabel = new wxFontPickerCtrl(m_pPanelPreferences, wxID_ANY, g_USFontLabel,
            wxDefaultPosition, wxDefaultSize);
    itemFlexGridSizer02->Add(m_pFontPickerLabel, 0, wxALIGN_RIGHT | wxALL, 0);

    wxStaticText* itemStaticText05 = new wxStaticText(m_pPanelPreferences, wxID_ANY, _("Small:"),
            wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer02->Add(itemStaticText05, 0, wxEXPAND | wxALL, border_size);
    m_pFontPickerSmall = new wxFontPickerCtrl(m_pPanelPreferences, wxID_ANY, g_USFontSmall,
            wxDefaultPosition, wxDefaultSize);
    itemFlexGridSizer02->Add(m_pFontPickerSmall, 0, wxALIGN_RIGHT | wxALL, 0);

    wxStaticBox* itemStaticBoxURF = new wxStaticBox( m_pPanelPreferences, wxID_ANY,
    _("Units, Ranges, Formats") );
    wxStaticBoxSizer* itemStaticBoxSizer05 = new wxStaticBoxSizer( itemStaticBoxURF, wxHORIZONTAL );
    itemBoxSizerMainFrame->Add( itemStaticBoxSizer05, 0, wxEXPAND | wxALL, border_size );
    wxFlexGridSizer *itemFlexGridSizer03 = new wxFlexGridSizer( 2 );
    itemFlexGridSizer03->AddGrowableCol( 1 );
    itemStaticBoxSizer05->Add( itemFlexGridSizer03, 1, wxEXPAND | wxALL, 0 );
    itemBoxSizerMainFrame->AddSpacer( 5 );

    wxStaticText* itemStaticText06 = new wxStaticText( m_pPanelPreferences, wxID_ANY, _("Protocol:"),
        wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer03->Add( itemStaticText06, 0, wxEXPAND | wxALL, border_size );
    wxString m_ProtocolChoices[] = { _("NMEA 0183"), _("Signal K"), _("NMEA 2000 (socketCAN)") };
    int m_ProtocolNChoices = sizeof( m_ProtocolChoices ) / sizeof( wxString );
    m_pChoiceProtocol = new wxChoice( m_pPanelPreferences, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        m_ProtocolNChoices, m_ProtocolChoices, 0 );
    m_pChoiceProtocol->SetSelection( g_iOdoProtocol );
    itemFlexGridSizer03->Add( m_pChoiceProtocol, 0, wxALIGN_RIGHT | wxALL, 0 );

    wxStaticText* itemStaticText07 = new wxStaticText( m_pPanelPreferences, wxID_ANY, _("Speedometer max value:"),
        wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer03->Add( itemStaticText07, 0, wxEXPAND | wxALL, border_size );
    m_pSpinSpeedMax = new wxSpinCtrl( m_pPanelPreferences, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxSP_ARROW_KEYS, 10, 80, g_iOdoSpeedMax );
    itemFlexGridSizer03->Add(m_pSpinSpeedMax, 0, wxALIGN_RIGHT | wxALL, 0);

    wxStaticText* itemStaticText08 = new wxStaticText( m_pPanelPreferences, wxID_ANY, _("Minimum On-Route speed:"),
        wxDefaultPosition, wxDefaultSize, 0);
    itemFlexGridSizer03->Add(itemStaticText08, 0, wxEXPAND | wxALL, border_size);
    m_pSpinOnRoute = new wxSpinCtrl(m_pPanelPreferences, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxSP_ARROW_KEYS, 0, 5, g_iOdoOnRoute);
    itemFlexGridSizer03->Add(m_pSpinOnRoute, 0, wxALIGN_RIGHT | wxALL, 0);

    wxStaticText* itemStaticText09 = new wxStaticText( m_pPanelPreferences, wxID_ANY, _( "Local Time Offset From UTC:" ),
        wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer03->Add( itemStaticText09, 0, wxEXPAND | wxALL, border_size );
    wxString m_UTCOffsetChoices[] = {
        _T( "-12:00" ), _T( "-11:30" ), _T( "-11:00" ), _T( "-10:30" ), _T( "-10:00" ),
        _T( "-09:30" ), _T( "-09:00" ), _T( "-08:30" ), _T( "-08:00" ), _T( "-07:30" ),
        _T( "-07:00" ), _T( "-06:30" ), _T( "-06:00" ), _T( "-05:30" ), _T( "-05:00" ),
        _T( "-04:30" ), _T( "-04:00" ), _T( "-03:30" ), _T( "-03:00" ), _T( "-02:30" ),
        _T( "-02:00" ), _T( "-01:30" ), _T( "-01:00" ), _T( "-00:30" ), _T( " 00:00" ),
        _T( " 00:30" ), _T( " 01:00" ), _T( " 01:30" ), _T( " 02:00" ), _T( " 02:30" ),
        _T( " 03:00" ), _T( " 03:30" ), _T( " 04:00" ), _T( " 04:30" ), _T( " 05:00" ),
        _T( " 05:30" ), _T( " 06:00" ), _T( " 06:30" ), _T( " 07:00" ), _T( " 07:30" ),
        _T( " 08:00" ), _T( " 08:30" ), _T( " 09:00" ), _T( " 09:30" ), _T( " 10:00" ),
        _T( " 10:30" ), _T( " 11:00" ), _T( " 11:30" ), _T( " 12:00" )};
    int m_UTCOffsetNChoices = sizeof( m_UTCOffsetChoices ) / sizeof( wxString );
    m_pChoiceUTCOffset = new wxChoice( m_pPanelPreferences, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        m_UTCOffsetNChoices, m_UTCOffsetChoices, 0 );
    m_pChoiceUTCOffset->SetSelection( g_iOdoUTCOffset );
    itemFlexGridSizer03->Add( m_pChoiceUTCOffset, 0, wxALIGN_RIGHT | wxALL, 0 );

    wxStaticText* itemStaticText10 = new wxStaticText( m_pPanelPreferences, wxID_ANY, _("Boat speed units:"),
        wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer03->Add( itemStaticText10, 0, wxEXPAND | wxALL, border_size );
    wxString m_SpeedUnitChoices[] = { _("Kts"), _("mph"), _("km/h"), _("m/s") };
    int m_SpeedUnitNChoices = sizeof( m_SpeedUnitChoices ) / sizeof( wxString );
    m_pChoiceSpeedUnit = new wxChoice( m_pPanelPreferences, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        m_SpeedUnitNChoices, m_SpeedUnitChoices, 0 );
    m_pChoiceSpeedUnit->SetSelection( g_iOdoSpeedUnit );
    itemFlexGridSizer03->Add( m_pChoiceSpeedUnit, 0, wxALIGN_RIGHT | wxALL, 0 );

    wxStaticText* itemStaticText11 = new wxStaticText( m_pPanelPreferences, wxID_ANY, _("Distance units:"),
        wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer03->Add( itemStaticText11, 0, wxEXPAND | wxALL, border_size );
    wxString m_DistanceUnitChoices[] = { _("Nautical miles"), _("Statute miles"), _("Kilometers") };
    int m_DistanceUnitNChoices = sizeof( m_DistanceUnitChoices ) / sizeof( wxString );
    m_pChoiceDistanceUnit = new wxChoice( m_pPanelPreferences, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        m_DistanceUnitNChoices, m_DistanceUnitChoices, 0 );
    m_pChoiceDistanceUnit->SetSelection( g_iOdoDistanceUnit );
    itemFlexGridSizer03->Add( m_pChoiceDistanceUnit, 0, wxALIGN_RIGHT | wxALL, 0 );

    wxStaticText* itemStaticText12 = new wxStaticText( m_pPanelPreferences, wxID_ANY,
         _("Automatic trip reset, time since last arrival:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer03->Add( itemStaticText12, 0, wxEXPAND | wxALL, border_size );
    wxString m_TripDelayTime[] = { _("15 minutes"), _("30 minutes"), _("1 hour"), _("2 hours"), _("3 hours"), _("4 hours"), _("5 hours"), _("6 hours"), _("9 hours"), _("12 hours") };
    int m_TripDelayTimeNChoices = sizeof( m_TripDelayTime ) / sizeof( wxString );
    m_pChoiceTripDelayTime = new wxChoice( m_pPanelPreferences, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        m_TripDelayTimeNChoices, m_TripDelayTime, 0 );
    m_pChoiceTripDelayTime->SetSelection( g_iAutoResetTripTime );
    itemFlexGridSizer03->Add( m_pChoiceTripDelayTime, 0, wxALIGN_RIGHT | wxALL, 0 );

    wxStaticText* itemStaticText13 = new wxStaticText( m_pPanelPreferences, wxID_ANY,
         _("Shortest trip stop time to log:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer03->Add( itemStaticText13, 0, wxEXPAND | wxALL, border_size );
    wxString m_MinTripStop[] = { _("All stops"), _("5 minutes"), _("15 minutes"), _("30 minutes"), _("1 hour"), _("2 hours"), _("4 hours"), _("8 hours"), _("No stops logged") };
    int m_MinTripStopChoices = sizeof( m_MinTripStop ) / sizeof( wxString );
    m_pChoiceMinTripStop = new wxChoice( m_pPanelPreferences, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        m_MinTripStopChoices, m_MinTripStop, 0 );
    m_pChoiceMinTripStop->SetSelection( g_iTripStopMinutes );
    itemFlexGridSizer03->Add( m_pChoiceMinTripStop, 0, wxALIGN_RIGHT | wxALL, 0 );

	wxStdDialogButtonSizer* DialogButtonSizer = 
        CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    itemBoxSizerMainPanel->Add(DialogButtonSizer, 0, wxALIGN_RIGHT | wxALL, 5);

    /* NOTE: These are not preferences settings items, there are no change options in Odometer
             besides the ones used when toggling show checkboxes. */
    m_pListCtrlOdometers = new wxListCtrl( this , wxID_ANY, wxDefaultPosition, wxSize(0, 0),
         wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL);
    m_pListCtrlInstruments = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxSize( 0, 0 ),
         wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL | wxLC_SORT_ASCENDING );
    m_pListCtrlInstruments->InsertColumn(0, _("Instruments"));

    UpdateOdometerButtonsState();

    Fit();
    Layout();
    CentreOnScreen();
}

void OdometerPreferencesDialog::OnCloseDialog(wxCloseEvent& event) {
    UpdateOdometerButtonsState();
    SaveOdometerConfig();
    event.Skip();
}

void OdometerPreferencesDialog::SaveOdometerConfig(void) {

    g_iOdoProtocol = m_pChoiceProtocol->GetSelection();
    g_iOdoSpeedMax = m_pSpinSpeedMax->GetValue();
    g_iOdoOnRoute = m_pSpinOnRoute->GetValue();
    g_iOdoUTCOffset = m_pChoiceUTCOffset->GetSelection();
    g_iOdoSpeedUnit = m_pChoiceSpeedUnit->GetSelection();
    g_iOdoDistanceUnit = m_pChoiceDistanceUnit->GetSelection();
    g_iAutoResetTripTime = m_pChoiceTripDelayTime->GetSelection();
    g_iTripStopMinutes = m_pChoiceMinTripStop->GetSelection();


    OdometerWindowContainer *cont = m_Config.Item(0);
    cont->m_bIsVisible = m_pCheckBoxIsVisible->IsChecked();
    cont->m_bShowSpeed = m_pCheckBoxShowSpeed->IsChecked();
    cont->m_bShowDepArrTimes = m_pCheckBoxShowDepArrTimes->IsChecked();
    cont->m_bShowTripLeg = m_pCheckBoxShowTripLeg->IsChecked();
    cont->m_sCaption = m_pTextCtrlCaption->GetValue();

    cont->m_aInstrumentList.Clear();

    // Show speedometer, always first, instrument 0
    if (m_pCheckBoxShowSpeed->IsChecked()) { 
        size_t speedinstr = 0;
        cont->m_aInstrumentList.Add(speedinstr);
    }
    // Total distance and trip distance can not be toggled on/off. Instruments 1 - 2.
    for (size_t distanceinstr = 1; distanceinstr < 3; distanceinstr++) {
        cont->m_aInstrumentList.Add(distanceinstr);
    }
    // Departure and arrival, instruments 3 - 4.
    if (m_pCheckBoxShowDepArrTimes->IsChecked()) {
        for (size_t deparrinstr = 3; deparrinstr < 5; deparrinstr++) {
            cont->m_aInstrumentList.Add(deparrinstr);
        }
    }
    // trip reset button and autoreset can not be toggled on/off, instrument 5 - 6.
    for (size_t resetinstr = 5; resetinstr < 7; resetinstr++) {
        cont->m_aInstrumentList.Add(resetinstr);
    }
    // Trip leg, time plus buttons, instruments 7 - 10.
    if (m_pCheckBoxShowTripLeg->IsChecked()) {
        for (size_t leginstr = 7; leginstr < 11; leginstr++) {
            cont->m_aInstrumentList.Add(leginstr);
        }
    }
}

void OdometerPreferencesDialog::OnOdometerSelected(wxListEvent& event) {
    SaveOdometerConfig();
    UpdateOdometerButtonsState();
}

void OdometerPreferencesDialog::UpdateOdometerButtonsState() {

    long item = -1;

    // Forcing 'item = 0' enables the first (and only) panel in the settings dialogue.
    item = 0;
    bool enable = (item != -1);

    m_pPanelPreferences->Enable( enable );

    OdometerWindowContainer *cont = m_Config.Item(0);
    m_pCheckBoxIsVisible->SetValue(cont->m_bIsVisible);
    m_pCheckBoxShowSpeed->SetValue(cont->m_bShowSpeed);
    m_pCheckBoxShowDepArrTimes->SetValue(cont->m_bShowDepArrTimes);
    m_pCheckBoxShowTripLeg->SetValue(cont->m_bShowTripLeg);
    m_pTextCtrlCaption->SetValue(cont->m_sCaption);

    m_pListCtrlInstruments->DeleteAllItems();
    for (size_t i = 0; i < cont->m_aInstrumentList.GetCount(); i++) {
        wxListItem item;
        GetListItemForInstrument(item, cont->m_aInstrumentList.Item(i));
        item.SetId(m_pListCtrlInstruments->GetItemCount());
        m_pListCtrlInstruments->InsertItem(item);
    }
}


//---------------------------------------------------------------------------------------------------------
//
//    Odometer Window Implementation
//
//---------------------------------------------------------------------------------------------------------

// wxWS_EX_VALIDATE_RECURSIVELY required to push events to parents
OdometerWindow::OdometerWindow(wxWindow *pparent, wxWindowID id,
                                wxAuiManager *auimgr, odometer_pi* plugin,
                                int orient, OdometerWindowContainer* mycont) :
        wxWindow(pparent, id, wxDefaultPosition, wxDefaultSize,
            wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE, _T("Odometer")) {

    m_pauimgr = auimgr;
    m_plugin = plugin;
    m_Container = mycont;

    OdoBoxSizer = new wxBoxSizer(wxVERTICAL); 

    Connect(wxEVT_SIZE, wxSizeEventHandler(OdometerWindow::OnSize), NULL, this);
    Connect(wxEVT_CONTEXT_MENU, wxContextMenuEventHandler(OdometerWindow::OnContextMenu), NULL,
            this);
    Connect(wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(OdometerWindow::OnContextMenuSelect), NULL, this);

    Hide();

    m_binResize = false;
    m_binPinch = false;
}

// Only at shutdown, very last thing
OdometerWindow::~OdometerWindow() {
    for (size_t i = 0; i < m_ArrayOfInstrument.GetCount(); i++) {
        OdometerInstrumentContainer *pdic = m_ArrayOfInstrument.Item(i);
        delete pdic;
    }
}

// Executed at start, when closing setup and when instument size is changed
void OdometerWindow::OnSize(wxSizeEvent& event) {

    event.Skip();
    for (unsigned int i=0; i < m_ArrayOfInstrument.size(); i++) {
        OdometerInstrument* inst = m_ArrayOfInstrument.Item(i)->m_pInstrument;
        inst->SetMinSize (
            inst->GetSize(OdoBoxSizer->GetOrientation(), GetClientSize()));
    }
}

void OdometerWindow::OnContextMenu(wxContextMenuEvent& event) {
    wxMenu* contextMenu = new wxMenu();

    wxAuiPaneInfo &pane = m_pauimgr->GetPane(this);
    if (pane.IsOk() && pane.IsDocked()) {
        contextMenu->Append(ID_ODO_UNDOCK, _("Undock"));
    }
    contextMenu->Append(ID_ODO_PREFS, _("Preferences ..."));
    contextMenu->Append(ID_ODO_VIEWLOG, _("View trip log ..."));
    PopupMenu(contextMenu);
    delete contextMenu;
}

void OdometerWindow::OnContextMenuSelect(wxCommandEvent& event) {

    if (event.GetId() < ID_ODO_PREFS) {
	// Toggle odometer visibility
        m_plugin->ShowOdometer(event.GetId()-1, event.IsChecked());
        SetToolbarItemState(m_plugin->GetToolbarItemId(),
            m_plugin->GetOdometerWindowShownCount() != 0);
    }

    switch(event.GetId()) {
        case ID_ODO_PREFS: {
            m_plugin->ShowPreferencesDialog(this);
            return; // Does it's own save.
        }


        case ID_ODO_VIEWLOG: {
            m_plugin->ShowViewLogDialog(this);
            return; // Does it's own save.
        }
    }
    m_plugin->SaveConfig();
}

void OdometerWindow::SetColorScheme(PI_ColorScheme cs) {
    DimeWindow(this);

    // Improve appearance, especially in DUSK or NIGHT palette
    wxColour col;
    GetGlobalColor(_T("DASHL"), &col);
    SetBackgroundColour(col);
    Refresh(false);
}

bool isArrayIntEqual(const wxArrayInt& l1, const wxArrayOfInstrument &l2) {

    if (l1.GetCount() != l2.GetCount()) {
        return false;
    }
    for (size_t i = 0; i < l1.GetCount(); i++) {
        if (l1.Item(i) != l2.Item(i)->m_ID) {
            return false;
         }
    }
    return true;
}

bool OdometerWindow::isInstrumentListEqual(const wxArrayInt& list) {
    return isArrayIntEqual(list, m_ArrayOfInstrument);
}

// Create and display each instrument in a odometer container
// executed at start and when closing setup
void OdometerWindow::SetInstrumentList(wxArrayInt list) {

    m_ArrayOfInstrument.Clear();
    OdoBoxSizer->Clear(true);  // Need to delete previous windows

    // Initial sizing seems required to force an inital size
    OdoBoxSizer->Add(DefaultWidth,1,0);
    SetSizerAndFit(OdoBoxSizer);
    
    for (size_t i = 0; i < list.GetCount(); i++) {

        int id = list.Item(i);
        OdometerInstrument *instrument = NULL;

        switch (id) {
            case ID_DBP_D_SOG:  // id = 0
                instrument = new OdometerInstrument_Speedometer( this, wxID_ANY,
                    GetInstrumentCaption( id ), OCPN_DBP_STC_SOG, 0, g_iOdoSpeedMax );
                ( (OdometerInstrument_Dial *) instrument )->SetOptionLabel
                    ( g_iOdoSpeedMax / 20 + 1, DIAL_LABEL_HORIZONTAL );
                ( (OdometerInstrument_Dial *) instrument )
                    ->SetOptionMarker( 0.5, DIAL_MARKER_SIMPLE, 2 );
                break;

            case ID_DBP_I_SUMLOG:  // id = 1
                instrument = new OdometerInstrument_Single( this, wxID_ANY,
                    GetInstrumentCaption( id ), OCPN_DBP_STC_SUMLOG, _T("%16.1f") );
                break;

            case ID_DBP_I_TRIPLOG:  // id = 2
                instrument = new OdometerInstrument_Single( this, wxID_ANY,
                    GetInstrumentCaption( id ), OCPN_DBP_STC_TRIPLOG, _T("%16.1f") );
                break;

            case ID_DBP_I_DEPART:  // id = 3
                instrument = new OdometerInstrument_String( this, wxID_ANY,
                    GetInstrumentCaption( id ), OCPN_DBP_STC_DEPART, _T("%1s") );
                break;

            case ID_DBP_I_ARRIV:  // id = 4
                instrument = new OdometerInstrument_String( this, wxID_ANY,
                    GetInstrumentCaption( id ), OCPN_DBP_STC_ARRIV, _T("%1s") );
                break;

            case ID_DBP_B_TRIPRES:  // id = 5
                instrument = new OdometerInstrument_TripResButton( this, wxID_ANY,
                    GetInstrumentCaption( id ), OCPN_DBP_STC_TRIPRES );
                break;

            case ID_DBP_C_AUTORESET:  // id = 6
                instrument = new OdometerInstrument_Checkbox( this, wxID_ANY,
                    GetInstrumentCaption( id ), g_iAutoResetTrip);
                break;

            case ID_DBP_I_LEGDIST:  // id = 7
                instrument = new OdometerInstrument_Single( this, wxID_ANY,
                    GetInstrumentCaption( id ), OCPN_DBP_STC_LEGDIST,_T("%15.2f") );
                break;

            case ID_DBP_I_LEGTIME:  // id = 8
                instrument = new OdometerInstrument_String( this, wxID_ANY,
                    GetInstrumentCaption( id ), OCPN_DBP_STC_LEGTIME,_T("%8s") );
                break;

            case ID_DBP_B_STARTSTOP:  // id = 9
                instrument = new OdometerInstrument_LegStartStopButton( this, wxID_ANY,
                    GetInstrumentCaption( id ), OCPN_DBP_STC_STARTSTOP );
                break;

            case ID_DBP_B_LEGRES:  // id = 10
                instrument = new OdometerInstrument_LegResetButton( this, wxID_ANY,
                    GetInstrumentCaption( id ), OCPN_DBP_STC_LEGRES );
                break;
	    	}

        if (instrument) {
            instrument->instrumentTypeId = id;
            m_ArrayOfInstrument.Add(new OdometerInstrumentContainer(
                id, instrument,instrument->GetCapacity()));
            OdoBoxSizer->Add(instrument, 0, wxEXPAND | wxALL | wxFULL_REPAINT_ON_RESIZE, 0);
        }
    }

    //  In the absense of any other hints, build the default instrument sizes by 
    //  taking the calculated with of the first (and succeeding) instruments as
    //  hints for the next. So, best in default loads to start with an instrument
    //  that accurately calculates its minimum width. e.g.
    //  DashboardInstrument_Position

    wxSize Hint = wxSize(DefaultWidth, DefaultWidth);  // Default width == 150
    for (unsigned int i = 0; i < m_ArrayOfInstrument.size(); i++) {
        OdometerInstrument *inst = m_ArrayOfInstrument.Item(i)->m_pInstrument;
        inst->SetMinSize(inst->GetSize(OdoBoxSizer->GetOrientation(), Hint));
        Hint = inst->GetMinSize();
    }

    OdoBoxSizer->Layout();
    SetSizerAndFit(OdoBoxSizer);
    OdoBoxSizer->Show(1, true);
}

void OdometerWindow::SendSentenceToAllInstruments(int st, double value, wxString unit) {

    for (size_t i = 0; i < m_ArrayOfInstrument.GetCount(); i++) {
		if (m_ArrayOfInstrument.Item(i)->m_cap_flag & st) {
			m_ArrayOfInstrument.Item(i)->m_pInstrument->SetData(st, value, unit);

		}
    }
}

//---------------------------------------------------------------------------------------------------------
//
// View Log Dialog
//
//---------------------------------------------------------------------------------------------------------

OdometerViewLogDialog::OdometerViewLogDialog(
    wxWindow *parent, wxWindowID id):
      wxDialog(parent, id, _("Log View Settings"), wxDefaultPosition, 
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {

      Connect(wxEVT_CLOSE_WINDOW, 
          wxCloseEventHandler(OdometerViewLogDialog::OnCloseLogDialog), NULL, this);

    int border_size = 4;

    wxBoxSizer* itemBoxSizerMainLogViewPanel = new wxBoxSizer(wxVERTICAL);
    SetSizer(itemBoxSizerMainLogViewPanel);

    wxFlexGridSizer *itemFlexGridSizer = new wxFlexGridSizer(1);
    m_pLogPanelPreferences = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxBORDER_NONE);
    itemBoxSizerMainLogViewPanel->Add(m_pLogPanelPreferences, 1, wxEXPAND | wxTOP | wxRIGHT,
        border_size);

    //   Main frame and box for Viewing options
    wxBoxSizer* itemBoxSizerMainFrame = new wxBoxSizer(wxVERTICAL);
    m_pLogPanelPreferences->SetSizer(itemBoxSizerMainFrame);
    itemBoxSizerMainFrame->AddSpacer( 5 );

    wxStaticBox* itemStaticBoxViewOpts = new wxStaticBox(m_pLogPanelPreferences, wxID_ANY,
       _("Display options"));
    wxStaticBoxSizer* itemStaticBoxLogViewSizer01 = new wxStaticBoxSizer(
        itemStaticBoxViewOpts, wxHORIZONTAL);
    itemBoxSizerMainFrame->Add(itemStaticBoxLogViewSizer01, 1, wxEXPAND | wxALL, border_size);


    //   Select log information
    wxArrayString *Trips = new wxArrayString;
        Trips->Add(_("Last trip"));
        Trips->Add(_("Last three trips"));
        Trips->Add(_("All logged trips"));

    wxFlexGridSizer *itemFlexGridLogSizer01 = new wxFlexGridSizer(2);
    itemFlexGridLogSizer01->AddGrowableCol(0);
    itemStaticBoxLogViewSizer01->Add(itemFlexGridLogSizer01, 1, wxEXPAND | wxALL, 0);

    m_pRadioBoxTrips = new wxRadioBox(m_pLogPanelPreferences, wxID_ANY, _("Select trips"),
        wxDefaultPosition, wxDefaultSize, *Trips, 1, wxRA_SPECIFY_COLS );
    m_pRadioBoxTrips->SetSelection (g_iSelectLogTrips);
    itemFlexGridLogSizer01->Add(m_pRadioBoxTrips, 0, wxEXPAND | wxALL, border_size);


    //   Select log view format
    wxArrayString *ViewFormat = new wxArrayString;
        ViewFormat->Add(_("Formatted text"));
        ViewFormat->Add(_("CSV format"));
        ViewFormat->Add(_("HTML format"));

    wxFlexGridSizer *itemFlexGridLogSizer02 = new wxFlexGridSizer(2);
    itemFlexGridLogSizer02->AddGrowableCol(0);
    itemStaticBoxLogViewSizer01->Add(itemFlexGridLogSizer02, 1, wxEXPAND | wxALL, 0);

    m_pRadioBoxFormat = new wxRadioBox(m_pLogPanelPreferences, wxID_ANY, _("View formats"),
        wxDefaultPosition, wxDefaultSize, *ViewFormat, 1, wxRA_SPECIFY_COLS);
    m_pRadioBoxFormat->SetSelection (g_iSelectLogFormat);
    itemFlexGridLogSizer02->Add(m_pRadioBoxFormat, 0, wxEXPAND | wxALL, border_size);


    //   Select log output options
    wxArrayString *Outputs = new wxArrayString;
        Outputs->Add(_("Save to disk"));
        Outputs->Add(_("External viewer"));

    wxFlexGridSizer *itemFlexGridLogSizer03 = new wxFlexGridSizer(2);
    itemFlexGridLogSizer03->AddGrowableCol(0);
    itemStaticBoxLogViewSizer01->Add(itemFlexGridLogSizer03, 1, wxEXPAND | wxALL, 0);

    m_pRadioBoxOutput = new wxRadioBox(m_pLogPanelPreferences, wxID_ANY, _("Output"),
        wxDefaultPosition, wxDefaultSize, *Outputs, 1, wxRA_SPECIFY_COLS);
    m_pRadioBoxOutput->SetSelection (g_iSelectLogOutput);
    itemFlexGridLogSizer03->Add(m_pRadioBoxOutput, 0, wxEXPAND | wxALL, border_size);

	wxStdDialogButtonSizer* ViewDialogButtonSizer = 
        CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    itemBoxSizerMainLogViewPanel->Add(ViewDialogButtonSizer, 0, wxALIGN_RIGHT, 5);

    Fit();
    Layout();
    CentreOnScreen();
}


// Executed when clicking upper right corner 'X', not when clicking 'CANCEL' nor 'OK'
void OdometerViewLogDialog::OnCloseLogDialog(wxCloseEvent& event) {
    event.Skip();
}


void OdometerViewLogDialog::SaveLogConfig() {

    g_iSelectLogTrips = m_pRadioBoxTrips->GetSelection();
    g_iSelectLogFormat = m_pRadioBoxFormat->GetSelection();
    g_iSelectLogOutput = m_pRadioBoxOutput->GetSelection();
}

 
void odometer_pi::ShowViewLogDialog(wxWindow* parent) {
    OdometerViewLogDialog *dialog = new OdometerViewLogDialog(parent, wxID_ANY);

    // Apply clicked, save current alternatives and execute view options
    if (dialog->ShowModal() == wxID_OK) {
        dialog->SaveLogConfig();
        GenerateLogOutput();
    }
    // Invoke the dialog destructor
    dialog->Destroy();
}


//  --------------------------------------------------------------------------------------
//  Select output format
//  --------------------------------------------------------------------------------------

void odometer_pi::GenerateLogOutput() {

    // Define log file location and read full content into a string
    validLog = 0;
    m_LogFile.Clear();
    strLog.clear();
    m_LogFile = g_sDataDir + _T("datalog.log");
    wxFileName logfile(m_LogFile);

    // Ensure log file exist and contains valid data.
    if (m_LogFileName.Open(m_LogFile, wxFile::read)) {
        m_LogFileName.ReadAll(&strLog, wxConvAuto());
        m_LogFileName.Close();

        // strlog contains the complete trip data, trim to log info.
        // Log lines all start with D: so select all lines from the first D: -line
        strLog = strLog.Mid(strLog.Find("D: "));

        size_t logfileLength = strLog.Length();
        if (logfileLength >= 18)  {
            validLog = 1;

            // Remove last \n for the ease of count
            strLog.RemoveLast();

            // Select number of trips shown (1, 3 or All)
            if (g_iSelectLogTrips == 0)  {    // Last trip only
                wxString startChar = "D:";
                strLog = strLog.Mid(strLog.rfind(startChar));
                strLog = strLog.Remove(0,3);

                // Restore 'D: ' to strLog for printout formatting
                size_t startPos = 0;
                strLog = strLog.insert(startPos, "D: ");
            }

            if (g_iSelectLogTrips == 1)  {    // Last three trips
                // Ensure there are at least three departures or reduce the loop
                wxString tripleLog;
                wxString startChar = "D:";
                size_t strLogLength;
                size_t tripleLength;
                size_t cutStrLength;

                // Extract last line and cut it from log file
                tripleLog = strLog;
                strLogLength = strLog.Length();
                tripleLog = strLog.substr(strLog.rfind(startChar) );
                tripleLength = tripleLog.Length();
                cutStrLength = strLogLength - tripleLength;
                strLog = strLog.Truncate(cutStrLength);
                strLogtripsum.clear();
                strLogtripsum = tripleLog;
                strLogtripsum = strLogtripsum.Remove(0,3);

                // Extract second last log line (if exist) then cut it from log file
                if (cutStrLength >= 15)  {
                    strLogLength = cutStrLength;
                    tripleLog = strLog.substr(strLog.rfind(startChar) );
                    tripleLength = tripleLog.Length();
                    cutStrLength = strLogLength - tripleLength;
                    strLog = strLog.Truncate(cutStrLength);
                    strLogtripsum = tripleLog + strLogtripsum;
                    strLogtripsum = strLogtripsum.Remove(0,3);
                }

                // Extract third last log line (if exist) then cut it from log file
                if (cutStrLength >= 15)  {
                    strLogLength = cutStrLength;
                    tripleLog = strLog.substr(strLog.rfind(startChar) );
                    tripleLength = tripleLog.Length();
                    cutStrLength = strLogLength - tripleLength;
                    strLog = strLog.Truncate(cutStrLength);
                    strLogtripsum = tripleLog + strLogtripsum;
                    strLogtripsum = strLogtripsum.Remove(0,3);
                }

                // Restore 'D: ' for printout formatting, need "\n" unless text
                size_t startTriplePos = 0;
                if (g_iSelectLogFormat != 0) {
                    strLogtripsum = strLogtripsum.insert(startTriplePos, "\n");
                }
                strLogtripsum.Replace( "\n", "\nD: " );
                strLog = strLogtripsum;
            } 

            // Put the \n at end of string back again
            strLog = strLog.Append("\n");

            homeDir = wxStandardPaths::Get().GetDocumentsDir();
            TripLogFile = homeDir + wxFileName::GetPathSeparator() + _T("TripLog.txt");

            // Common information for all output formatss
            docTitle = _("Odometer log");
            LogFile.clear();
            LogFile = docTitle;
        }
    }

//  --------------------------------------------------------------------------------------
// Generate text formatted output, emulating tab using space
//  --------------------------------------------------------------------------------------

    if ((g_iSelectLogFormat == 0) && (validLog == 1))  {

        // Set up contents in table format, default to 3 columns
        LogFile = LogFile + "\n\n  " + _("Departure");
        LogFile = LogFile + "              " + _("Arrival");
        LogFile = LogFile + "             " + _("Trip");

        // Any restarts? Then add more headers.
        restartPos = strLog.find("R:", 0);

        if (restartPos >= 1)  {          // -1 if not found
            LogFile = LogFile + "      " + _("Restart");
            LogFile = LogFile + "             " + _("Arrival");
            LogFile = LogFile + "             " + _("Trip");
        }

        // Need spaces for detection
        strLog.Replace( "A:", "   A:" );
        strLog.Replace( "T:", "   T:" );
        strLog.Replace( "R:", "   R:" );

        // Set string positions on first row
        departPos = strLog.find("D:", 0);
        arrivalPos = strLog.find("A:", 0);
        restartPos = strLog.find("R:", 0);
        tripPos = strLog.find("T:", arrivalPos);  // Always after arrival


        // Calculate number of characters in trip distance
        tripDecimalPos = strLog.find(",", tripPos);
        tripLen = (tripDecimalPos - (tripPos + 3));
        EOLPos = strLog.find("\n", 0);

        while (departPos >= 0) {  // Once per row
            numRestarts = 0;

            // Restart on this line?
            restartPos = -1;
            if (restartPos < EOLPos) restartPos = strLog.find("R:", departPos);
            arrivalPos = strLog.find("A:", departPos);
            tripPos = strLog.find("T:", arrivalPos);
            // Distance need be fixed length
            // Trip distance info is terminated by a comma.
            tripDecimalPos = strLog.find(".", tripPos);
            tripLen = (tripDecimalPos - (tripPos + 3));

            if (tripLen <= 0 ) tripLen = 0;
            while ((tripLen < 5 ) && (tripPos >= 0))  {  // Third colu8mn only
                strLog.insert((tripPos + 3), space);  // include tag "T: " in position
                tripLen++;
            }
            EOLPos = strLog.find("\n", departPos);
            if (restartPos > EOLPos) restartPos = -1;

            while (restartPos >= 0)  {  // At least one restart on this departure
                numRestarts = 1;
                arrivalPos = strLog.find("A:", restartPos);
                tripPos = strLog.find("T:", restartPos);

                // Fix size on trip length after restarts, sixth column
                // Five digits plus decimal for trip distance
                tripDecimalPos = strLog.find(".", tripPos);
                tripLen = (tripDecimalPos - (tripPos + 3));
                if (tripLen <= 0 ) tripLen = 0;
                while ((tripLen < 5 ) && (tripPos >= 0))  {  // Sixth column
                    strLog.insert((tripPos + 3), space);  // include tag "T: " in position
                    tripLen++;
                }
                EOLPos = strLog.find("\n", tripPos);

                prevRestartPos = restartPos;
                restartPos = strLog.find("R:", prevRestartPos + 10);

                // A second valid restart found, add space for three first columns
                if ((restartPos >= 0 ) && (restartPos < EOLPos)) {
                    numRestarts = numRestarts + 1;

                    // Need to move restart columns to columns 4 to 6
                    restartLen = 0;
                    moveResColumns = "";
                    while (restartLen < 51 )  {
                        moveResColumns.Append(space);
                        restartLen++;
                    }
                    wxString restartRow = "\n" + moveResColumns;

                    // First departure line messes up lacking previous data positions
                    prevEOLPos = EOLPos;
                    EOLPos = strLog.find("\n", tripPos);

                    int diffprevRestartPos = (restartPos - prevRestartPos);

                    if ((EOLPos > 200) && (diffprevRestartPos > 100)) {
                        strLog.replace(restartPos, 3, restartRow);
                    }
                    if (diffprevRestartPos < 100) {
                        strLog.replace(restartPos, 3, restartRow);
                    }

                    // Remove blanks after restart distance in sixth column
                    int termBlanksPos = 0;
                    termBlanksPos = strLog.find(",", tripPos) + 1;
                    EOLPos = strLog.find("\n", termBlanksPos);
                    int termBlanks = EOLPos - termBlanksPos;
                    if (termBlanks > 0) strLog.Remove(termBlanksPos, termBlanks);
                }
                if (restartPos < 0) numRestarts = 0;
            }
            prevDepartPos = departPos;
            departPos = strLog.find("D:", (prevDepartPos +10));
        }

        strLog.Replace( "D: ", "");
        strLog.Replace( "A: ", "");
        strLog.Replace( "T: ", "");
        strLog.Replace( "R: ", "");
        strLog.Replace( ",", "" );


        LogFile = LogFile + "\n" + strLog;
        strLog = LogFile;
        TripLogFile = homeDir + wxFileName::GetPathSeparator() + _T("OdometerLog.txt");
    }

//  --------------------------------------------------------------------------------------
// Generate CSV formatted output, uses comma as separator
//  --------------------------------------------------------------------------------------

    if ((g_iSelectLogFormat == 1) && (validLog == 1))  {

        // Set up contents in table format, default to 3 columns
        LogFile = LogFile + "\n" + _("Departure") + ",";
        LogFile = LogFile + _("Arrival") + ",";
        LogFile = LogFile + _("Trip");

        // Any restarts? Then add more headers.
        restartPos = strLog.find("R:", 0);

        if (restartPos >= 1)  {          // -1 if not found
            LogFile = LogFile + "," + _("Restart") + ",";
            LogFile = LogFile + _("Arrival") + ",";
            LogFile = LogFile + _("Trip");
        }

        // Set string positions on first row
        departPos = strLog.find("D:", 0);
        arrivalPos = strLog.find("A:", 0);
        restartPos = strLog.find("R:", 0);
        tripPos = strLog.find("T:", arrivalPos);  // Always after arrival

        // Calculate number of characters in trip distance
        tripDecimalPos = strLog.find(",", tripPos);
        tripLen = (tripDecimalPos - (tripPos + 3));
        EOLPos = strLog.find("\n", 0);

        while (departPos >= 0) {  // Once per row
            numRestarts = 0;

            // Restart on this line?
            restartPos = -1;
            if (restartPos < EOLPos) restartPos = strLog.find("R:", departPos);
            arrivalPos = strLog.find("A:", departPos);
            tripPos = strLog.find("T:", arrivalPos);

            EOLPos = strLog.find("\n", departPos);
            if (restartPos > EOLPos) restartPos = -1;

            while (restartPos >= 0)  {  // At least one restart on this departure
                numRestarts = 1;
                arrivalPos = strLog.find("A:", restartPos);
                tripPos = strLog.find("T:", restartPos);

                // Remove comma after restart distance in sixth column
                int endCommaPos = 0;
                endCommaPos = strLog.find(",", tripPos);  // -1 if not found
                EOLPos = strLog.find("\n", tripPos);
                if ((endCommaPos > 0) && (EOLPos > endCommaPos)) strLog.Remove(endCommaPos, 2);

                prevRestartPos = restartPos;
                restartPos = strLog.find("R:", prevRestartPos + 10);

                // A second valid restart found, add space for three first columns
                if ((restartPos >= 0 ) && (restartPos < EOLPos)) {
                    numRestarts = numRestarts + 1;
                    wxString restartRow = "\n,,,";

                    // First departure line messes ups
                    if (EOLPos > 80)  strLog.replace(restartPos, 3,  restartRow);
                    if (EOLPos > 0)  strLog.replace(restartPos, 3, restartRow);

                }
                if (restartPos < 0) numRestarts = 0;
            }
            prevDepartPos = departPos;
            departPos = strLog.find("D:", (prevDepartPos +10));
        }

        strLog.Replace( "D: ", "");
        strLog.Replace( " A: ", "");
        strLog.Replace( " T: ", "");
        strLog.Replace( " R: ", "");
        strLog.Replace( ",,,,", ",,,");

        LogFile = LogFile + "\n" + strLog;
        strLog = LogFile;
        TripLogFile = homeDir + wxFileName::GetPathSeparator() + _T("OdometerLog.csv");
    }

//  --------------------------------------------------------------------------------------
// Generate HTML formatted output
//  --------------------------------------------------------------------------------------

    if ((g_iSelectLogFormat == 2) && (validLog == 1))  {

        wxString cssStyle = "<style>\n.center\n{\nmargin-left: auto;\nmargin-right: auto;\n}\nth, td\n{\ntext-align: center;\npadding-left: 15px;\npadding-right: 15px;\n}\ntable tbody td:nth-child(3){\ntext-align: right;\n}\ntable tbody td:nth-child(6){\ntext-align: right;\n}\n</style>\n";
        LogFile = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n" + cssStyle + "<title> ";
        LogFile = LogFile  + docTitle + "</title>\n</head\n";
        LogFile = LogFile + "<body>\n<h2><center>" + docTitle + "</center></h2>\n";

        // Set up contents in table format, default to 3 columns
        LogFile = LogFile + "<p>\n<table class=\"center\">\n<tr>";
        LogFile = LogFile + "<th>" + _("Departure") + "</th>\n";
        LogFile = LogFile + "<th>" + _("Arrival") + "</th>\n";
        LogFile = LogFile + "<th>" + _("Trip") + "</th>\n";

        // Are there any restarts? Then add headers.
        restartPos = strLog.find("R:", 0);

        if (restartPos >= 1)  {          // -1 if not found
            LogFile = LogFile + "<th>" + _("Restart") + "</th>\n";
            LogFile = LogFile + "<th>" + _("Arrival") + "</th>\n";
            LogFile = LogFile + "<th>" + _("Trip") + "</th>\n";
        }

        // Quit table header row and format table data rows
        LogFile = LogFile + "</tr> ";

        // End of string has no detectable indicator, add one
        strLog = strLog.Append("\n");

        // Add comumn handling when showing restarted trips
        newResRow = "</td></tr><tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>";

        // Rebuild strLog into html code
        // \n need be first or '\n' will be replaced all over
        strLog.Replace( "\n", "</td></tr>\n" );
        strLog.Replace( "D:", "<tr>\n<td>D:" );
        strLog.Replace( "A:", "</td>\n<td>A:" );
        strLog.Replace( "T:", "</td>\n<td>T:" );
        strLog.Replace( "R:", "</td>\n<td>R:" );

        // Set initial string positions
        departPos = strLog.find("D:", 0);
        arrivalPos = strLog.find("A:", 0);
        restartPos = strLog.find("R:", 0);
        tripPos = strLog.find("T:", arrivalPos);
        EOLPos = strLog.find("</td></tr>", 0);
        if (restartPos > EOLPos) restartPos = -1;

        while (departPos >= 0) {
            numRestarts = 0;
            arrivalPos = strLog.find("A:", departPos);
            restartPos = strLog.find("R:", departPos);
            tripPos = strLog.find("T:", arrivalPos);
            EOLPos = strLog.find("</td></tr>", departPos);
            if (restartPos > EOLPos) restartPos = -1;

            while (restartPos >= 0)  {   // One valid restart before EOL
                numRestarts = 1;
                prevRestartPos = restartPos;
                restartPos = strLog.find("R:", prevRestartPos + 10);

                if ((restartPos >= 0) && ((restartPos - prevRestartPos) < 200))  {
                    strLog.replace(restartPos, 3, newResRow);
                }
                if (restartPos < 0) numRestarts = 0;
            }
            prevDepartPos = departPos;
            departPos = strLog.find("D:", (prevDepartPos +10));
        }

        strLog.Replace( "D: ", "");
        strLog.Replace( "A: ", "");
        strLog.Replace( "T: ", "");
        strLog.Replace( "R: ", "");
        strLog.Replace( ",", "" );

        LogFile = LogFile + strLog + "</tr></table>\n</p>";
        LogFile = LogFile + "</table>\n</body>\n</html>" ;
        strLog = LogFile;
        TripLogFile = homeDir + wxFileName::GetPathSeparator() + _T("OdometerLog.html");
    }

//  --------------------------------------------------------------------------------------
// Save log file
//  --------------------------------------------------------------------------------------

    if (validLog == 1) {

        // Save log file in users home directory
        TripLogFileName.Open(TripLogFile, wxFile::write);
        TripLogFileName.Write(strLog);
        TripLogFileName.Close();

        if (g_iSelectLogOutput == 1)  {    // Open the file in a reader/browser
            if (g_iSelectLogFormat == 0)  {  // txt-formatted
                wxMimeTypesManager manager;
                wxFileType* fileType = manager.GetFileTypeFromExtension("txt");

                wxString cmd;
                cmd.Clear();
                cmd = fileType->GetOpenCommand(TripLogFile);
#ifdef __WXOSX__
                cmd = "/bin/bash -c \"open " + TripLogFile + "\"";
#endif
                wxExecute(cmd);
                cmd.Clear();
                delete fileType;
            }

            if (g_iSelectLogFormat == 1)  {  // csv-formatted

                wxMimeTypesManager manager;
                wxFileType* fileType = manager.GetFileTypeFromExtension("csv");

                wxString cmd;
                cmd.Clear();
                cmd = fileType->GetOpenCommand(TripLogFile);

#ifdef __WXOSX__
                cmd = "/bin/bash -c \"open " + TripLogFile + "\"";
#endif
                wxExecute(cmd);
                cmd.Clear();
                delete fileType;
            }

            if (g_iSelectLogFormat == 2) {   // HTML-formatted

                wxMimeTypesManager manager;
                wxFileType* fileType = manager.GetFileTypeFromExtension("html");

                wxString cmd;
                cmd.Clear();
                cmd = fileType->GetOpenCommand(TripLogFile);

                wxExecute(cmd);
                cmd.Clear();
                delete fileType;
            }
        }
    }
}

