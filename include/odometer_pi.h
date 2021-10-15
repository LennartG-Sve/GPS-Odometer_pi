//
// This file is part of GPS Odometer, a plugin for OpenCPN.
// based on the original version of the dashboard.
//
/******************************************************************************
 * $Id: dashboard_pi.h, v1.0 2010/08/05 SethDart Exp $
 *
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin
 * Author:   Jean-Eudes Onfray
 *
 ***************************************************************************
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

#ifndef _ODOMETERPI_H_
#define _ODOMETERPI_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include <wx/notebook.h>
#include <wx/fileconf.h>
#include <wx/listctrl.h>
#include <wx/imaglist.h>
#include <wx/spinctrl.h>
#include <wx/aui/aui.h>
#include <wx/fontpicker.h>
#include "wx/json_defs.h"
#include "wx/jsonreader.h"
#include "wx/jsonval.h"
#include "wx/jsonwriter.h"

// Differs from the built-in plugins, so that we can build outside of OpenCPN source tree
#include "ocpn_plugin.h"

// NMEA0183 Sentence parsing functions
#include "nmea0183.h"

// Odometer instruments/dials/gauges
#include "instrument.h"
#include "speedometer.h"
#include "button.h"
#include "iirfilter.h"

class OdometerWindow;
class OdometerWindowContainer;
class OdometerInstrumentContainer;

// Request default positioning of toolbar tool
#define ODOMETER_TOOL_POSITION -1          

// If no data received in 5 seconds, zero the instrument displays
// #define WATCHDOG_TIMEOUT_COUNT  5
#define gps_watchdog_timeout_ticks  5

class OdometerWindowContainer {
public:
	OdometerWindowContainer(OdometerWindow *odometer_window, wxString name, wxString caption, wxString orientation, wxArrayInt inst) {
       m_pOdometerWindow = odometer_window; m_sName = name; m_sCaption = caption; m_sOrientation = orientation; 
       m_aInstrumentList = inst; m_bIsVisible = false; m_bIsDeleted = false; m_bShowSpeed = true; m_bShowDepArrTimes = true;
       m_bShowTripLeg = true; }

	~OdometerWindowContainer(){}

	OdometerWindow *m_pOdometerWindow;
	bool m_bIsVisible;
	bool m_bIsDeleted;
	// Persists visibility, even when Odometer tool is toggled off.
	bool m_bPersVisible;  
	bool m_bShowSpeed;
	bool m_bShowDepArrTimes;
	bool m_bShowTripLeg;
	wxString m_sName;
	wxString m_sCaption;
	wxString m_sOrientation;
	wxArrayInt m_aInstrumentList;
};

class OdometerInstrumentContainer {
public:
	OdometerInstrumentContainer(int id, OdometerInstrument *instrument, int capa) {
		m_ID = id; m_pInstrument = instrument; m_cap_flag = capa; }

	~OdometerInstrumentContainer(){ delete m_pInstrument; }

	OdometerInstrument *m_pInstrument;
	int m_ID;
	int m_cap_flag;
};

// Dynamic arrays of pointers need explicit macros in wx261
#ifdef __WX261
WX_DEFINE_ARRAY_PTR(OdometerWindowContainer *, wxArrayOfOdometer);
WX_DEFINE_ARRAY_PTR(OdometerInstrumentContainer *, wxArrayOfInstrument);
#else
WX_DEFINE_ARRAY(OdometerWindowContainer *, wxArrayOfOdometer);
WX_DEFINE_ARRAY(OdometerInstrumentContainer *, wxArrayOfInstrument);
#endif


//
// Odometer PlugIn Class Definition
//

class odometer_pi : public opencpn_plugin_116, wxTimer {
public:
	odometer_pi(void *ppimgr);
	~odometer_pi(void);

	// The required OpenCPN PlugIn methods
	int Init(void);
	bool DeInit(void);
	int GetAPIVersionMajor();
	int GetAPIVersionMinor();
	int GetPlugInVersionMajor();
	int GetPlugInVersionMinor();
    wxBitmap *GetPlugInBitmap();
	wxString GetCommonName();
	wxString GetShortDescription();
	wxString GetLongDescription();
	
	// As we inherit from wxTimer, the method invoked each timer interval
	// Used by the plugin to refresh the instruments and to detect stale data 
	void Notify();

	// The optional OpenCPN plugin methods
	void SetNMEASentence(wxString &sentence);
    void SetPositionFix(PlugIn_Position_Fix &pfix);
	int GetToolbarToolCount(void);
	void OnToolbarToolCallback(int id);
	void ShowPreferencesDialog(wxWindow *parent);
	void SetColorScheme(PI_ColorScheme cs);
	void OnPaneClose(wxAuiManagerEvent& event);
	void UpdateAuiStatus(void);
	bool SaveConfig(void);
	void PopulateContextMenu(wxMenu *menu);
	void ShowOdometer(size_t id, bool visible);
	int GetToolbarItemId();
	int GetOdometerWindowShownCount();
    void Odometer();

	  
    int id;
//    wxString dt;

    double TotDist = 0.0;
    wxString DistUnit;
//	wxAuiManager *m_pauimgr;
    void SetPluginMessage(wxString &message_id, wxString &message_body);




private:
	// Load plugin configuraton
    wxArrayInt ar;
	bool LoadConfig(void);
    void LoadFont(wxFont **target, wxString native_info);

	void ApplyConfig(void);
	// Send deconstructed NMEA 1083 sentence  values to each display
	void SendSentenceToAllInstruments(int st, double value, wxString unit);
	void GetDistance();

	// OpenCPN goodness, pointers to Configuration, AUI Manager and Toolbar
	wxFileConfig *m_pconfig;
	wxAuiManager *m_pauimgr;
	int m_toolbar_item_id;

	// Hide/Show Odometer Windows
	wxArrayOfOdometer m_ArrayOfOdometerWindow;
	int m_show_id;
	int m_hide_id;

	// Used to parse NMEA Sentences
	NMEA0183 m_NMEA0183;
    short mPriCOGSOG;
    short mPriDateTime;
    short mPriVar;
    double mVar;
    wxDateTime mUTCDateTime;
    iirfilter mSOGFilter;
    wxString m_SatsRequired;
    wxString m_PwrOnDelSecs;
    wxString m_HDOPdefine;
    wxString m_FilterSOG;
    int FilterSOG = 1;
    int GPSQuality;
    int SignalQuality = 0;
    int SatsRequired;
    int SatsUsed;
    int HDOPdefine;
    double HDOPlevel;
    int validGPS = 0;
    int useNMEA = 0;
    int StartDelay = 1;
    int mUTC_Watchdog;
    int GNSSok = 0;

	// Used to parse Signal K Sentences
    void ParseSignalK( wxString &msg);
    void handleSKUpdate(wxJSONValue &update);
    void updateSKItem(wxJSONValue &item, wxString &sfixtime);
    wxString m_self;
    double SKSpeed;
    int SKQuality;
    double SKHDOPlevel;
    int SKSatsUsed;


    // Odometer time
    wxDateTime UTCTime;
    wxTimeSpan TimeOffset;
    wxDateTime LocalTime;

    // Odometer trip time
    double NMEASpeed; 
    double CurrSpeed; 
    double FilteredSpeed; 
    double m_OnRouteSpeed;
    wxDateTime EnabledTime;
    wxDateTime DepTime;
    wxDateTime ArrTime;
    int ArrTimeShow = 1;
    wxString strDep;
    wxString strArr;
    int SetDepTime = 0;
    int UseSavedDepTime = 1;
    int UseSavedArrTime = 1;

    // Odometer Trip and Sumlog distances
    wxString m_TotDist;
    wxString m_TripDist;
    wxString m_DepTime; 
    wxString m_ArrTime;
    double StepDist = 0;
    double TripDist = 0;
    int ResetDist;
    int CurrSec;
    int PrevSec;
    int SecDiff;
    int UseSavedTrip = 1; 
    double DistDiv = 3600;

    // Odometer leg distance and time 
    int DepTimeShow = 1;
    wxString m_LegDist;
    wxString m_LegTime; 
    double LegDist = 0;
    int CountLeg = 0;
    wxDateTime LegStart;
    wxTimeSpan LegTime;

	// Odometer uses version 2 configuration settings
	int m_config_version;
};

class OdometerPreferencesDialog : public wxDialog {
public:
	OdometerPreferencesDialog(wxWindow *pparent, wxWindowID id, wxArrayOfOdometer config);
	~OdometerPreferencesDialog() {}

	void OnCloseDialog(wxCloseEvent& event);
	void OnOdometerSelected(wxListEvent& event);
	void OnInstrumentSelected(wxListEvent& event);
	void SaveOdometerConfig(void);
    void RecalculateSize( void );

	wxArrayOfOdometer m_Config;
	wxFontPickerCtrl *m_pFontPickerTitle;
	wxFontPickerCtrl *m_pFontPickerData;
	wxFontPickerCtrl *m_pFontPickerLabel;
	wxFontPickerCtrl *m_pFontPickerSmall;
	wxSpinCtrl *m_pSpinSpeedMax;
    wxSpinCtrl *m_pSpinCOGDamp;
    wxSpinCtrl *m_pSpinOnRoute;
    wxChoice *m_pChoiceProtocol;
    wxChoice *m_pChoiceUTCOffset;
    wxChoice *m_pChoiceSpeedUnit;
    wxChoice *m_pChoiceDistanceUnit;
    wxSpinCtrlDouble *m_pSpinDBTOffset;
	wxCheckBox *m_pCheckBoxShowSpeed;
	wxCheckBox *m_pCheckBoxShowDepArrTimes;
	wxCheckBox *m_pCheckBoxShowTripLeg;



private:
	void UpdateOdometerButtonsState(void);
	void UpdateButtonsState(void);
	wxListCtrl *m_pListCtrlOdometers;
	wxPanel *m_pPanelPreferences;
	wxTextCtrl *m_pTextCtrlCaption;
	wxCheckBox *m_pCheckBoxIsVisible;
	wxListCtrl *m_pListCtrlInstruments;
};

class AddInstrumentDlg : public wxDialog {
public:
	AddInstrumentDlg(wxWindow *pparent, wxWindowID id);
	~AddInstrumentDlg() {}

	unsigned int GetInstrumentAdded();

private:
	wxListCtrl *m_pListCtrlInstruments;
};

enum {
	ID_ODOMETER_WINDOW
};

enum {
	ID_ODO_PREFS = 999,
//	ID_DASH_VERTICAL,
//	ID_DASH_HORIZONTAL,
	ID_ODO_UNDOCK
};

enum {
    NMEA_0183,
    SIGNAL_K
};

enum {
    SPEED_KNOTS,
    SPEED_MILES_PER_HOUR,
    SPEED_KILOMETERS_PER_HOUR,
    SPEED_METERS_PER_SECOND
};

enum {
    DISTANCE_NAUTICAL_MILES,
    DISTANCE_STATUTE_MILES,
    DISTANCE_KILOMETERS,
};

class OdometerWindow : public wxWindow {
public:
	OdometerWindow(wxWindow *pparent, wxWindowID id, wxAuiManager *auimgr, odometer_pi* plugin,
             int orient, OdometerWindowContainer* mycont);
    ~OdometerWindow();

    void SetColorScheme(PI_ColorScheme cs);
    void SetSizerOrientation(int orient);
    int GetSizerOrientation();
    void OnSize(wxSizeEvent& evt);
    void OnContextMenu(wxContextMenuEvent& evt);
    void OnContextMenuSelect(wxCommandEvent& evt);
    bool isInstrumentListEqual(const wxArrayInt& list);
    void SetInstrumentList(wxArrayInt list);
    void SendSentenceToAllInstruments(int st, double value, wxString unit);
    void ChangePaneOrientation(int orient, bool updateAUImgr);

	// TODO: OnKeyPress pass event to main window or disable focus

    OdometerWindowContainer *m_Container;

    bool m_binPinch;
    bool m_binPan;
    
    wxPoint m_resizeStartPoint;
    wxSize m_resizeStartSize;
    bool m_binResize;
    bool m_binResize2;


private:
	wxAuiManager *m_pauimgr;
	odometer_pi *m_plugin;
	wxBoxSizer *itemBoxSizer;
	wxArrayOfInstrument m_ArrayOfInstrument;
};

#endif

