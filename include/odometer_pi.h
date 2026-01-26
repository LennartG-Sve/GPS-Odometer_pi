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
#include "../libs/wxJSON/include/json_defs.h"
#include "../libs/wxJSON/include/jsonreader.h"
#include "../libs/wxJSON/include/jsonval.h"
#include "../libs/wxJSON/include/jsonwriter.h"

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
       m_aInstrumentList = inst; m_bIsVisible = false; m_bIsDeleted = false; m_bShowSpeed = true;
       m_bShowDepArrTimes = true; m_bShowTripLeg = true; m_bIncludeTripStops = false; }

	~OdometerWindowContainer(){}

	OdometerWindow *m_pOdometerWindow;
	bool m_bIsVisible;
	bool m_bIsDeleted;
	// Persists visibility, even when Odometer tool is toggled off.
	bool m_bPersVisible;  
	bool m_bShowSpeed;
	bool m_bShowDepArrTimes;
	bool m_bShowTripLeg;
	bool m_bIncludeTripStops;
	bool m_bTripStopMinutes;
	bool m_bAutoResetTrip;
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

class odometer_pi : public opencpn_plugin_118, wxTimer {
public:
	odometer_pi(void *ppimgr);
	~odometer_pi(void);

	// The required OpenCPN PlugIn methods
	int Init(void);
	bool DeInit(void);
    int GetAPIVersionMajor() override;
    int GetAPIVersionMinor() override;
    int GetPlugInVersionMajor() override;
    int GetPlugInVersionMinor() override;
    int GetPlugInVersionPatch() override;
    int GetPlugInVersionPost() override;
    wxBitmap *GetPlugInBitmap() override;
    wxString GetCommonName() override;
    wxString GetShortDescription() override;
    wxString GetLongDescription() override;	
	
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
    void BuildInstrList();
    void ShowViewLogDialog(wxWindow *parent);
    void GenerateLogOutput();
    void rmOldLogFiles();

	  
    int id;
    int i;   // Integer for while loops
    int validLog = 0;
    double TotDist = 0.0;
    double oldTotDist = 0.0;
    double dispTotDist = 0.0;
    wxString DistUnit;
    void SetPluginMessage(wxString &message_id, wxString &message_body);

private:
    wxArrayInt ar;
	bool LoadConfig(void);
    void LoadFont(wxFont **target, wxString native_info);
    void ApplyConfig(void);

    // Send deconstructed NMEA 1083 sentence  values to each display
    void SendSentenceToAllInstruments(int st, double value, wxString unit);
    void TripAutoReset(wxString ArrTime, wxDateTime LocalTime);
    void GetDistance();
    void DefineTripData();
    void WriteTripData();

    // OpenCPN goodness, pointers to Configuration, AUI Manager and Toolbar
    wxFileConfig *m_pconfig;
    wxAuiManager *m_pauimgr;
    int m_toolbar_item_id;

    // Hide/Show Odometer Windows
    wxArrayOfOdometer m_ArrayOfOdometerWindow;
    wxArrayOfOdometer m_ArrayOfOdometerLogWindow;
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
    int PwrOnDelaySecs;
    wxString m_SatsRequired;
    wxTimeSpan PwrOnDelay;
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
    int Initializing = 1;
    int PowerUpActive = 1;
    int mUTC_Watchdog;
    int GNSSok = 0;

    // Used to parse Signal K Sentences
    void ParseSignalK( wxString &msg);
    void handleSKUpdate(wxJSONValue &update);
    void updateSKItem(wxJSONValue &item, wxString &talker, wxString &sfixtime);
    wxString m_self;
    double SKSpeed;
    int SKQuality;
    double SKHDOPlevel;
    int SKSatsUsed;

    // Used to parse NMEA 2000 Sentences 
    // Requires ocpn_plugin.h 117 or greater)
    // N2KParser library added to libss directory
    std::shared_ptr<ObservableListener> listener_129026;
    std::shared_ptr<ObservableListener> listener_129029;
    void HandleN2K_129026(ObservedEvt ev);
    void HandleN2K_129029(ObservedEvt ev);

    // Odometer time
    wxDateTime UTCTime;
    wxTimeSpan TimeOffset;
    wxDateTime LocalTime;
    wxDateTime CurrTime;

    // Odometer trip time
    double NMEASpeed; 
    double CurrSpeed = 0.0; 
    double FilteredSpeed; 
    double OnRouteSpeed;
    int OnRoute = 0;
    wxDateTime EnabledTime;
    wxDateTime DepTime;
    wxDateTime ArrTime;
    wxDateTime ResTime;
    wxDateTime TripResetTime;
    double ResetDelay;
    int ArrTimeShow;
    wxString strDep;
    wxString strArr;
    wxString strRes;
    wxString strReset;
    wxString strLocal;
    int SetDepTime = 0;
    int UseSavedDepTime = 1;
    int UseSavedArrTime = 1;
    int oldMinute;

    // Odometer Trip and Sumlog distances
    wxString m_TotDist;
    wxString m_confTotDist;
    wxString m_TripDist;
    wxString m_confTripDist;
    wxString m_DepTime; 
    wxString logDepTime;
    wxString m_ArrTime;
    wxString logArrTime;
    wxString m_ResTime;
    wxString logResTime;
    wxString m_LocalTime;
    wxString strLog;
    double StepDist = 0;
    double TripDist = 0.0;
    double oldTripDist = 0.0;
    double dispTripDist = 0.0;
    int saveTripDist = 0;
    int saveTotDist = 0;
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
    double dispLegDist = 0;
    int CountLeg = 0;
    wxDateTime LegStart;
    wxTimeSpan LegTime;

    // Save log data to data dir
    wxString m_sumlogFile;
    wxFile m_sumLogFileName;
    wxTextFile m_sumLogFileTextReader;
    double sumLog = 0;
    int ReadSumlogFromData;
    wxString m_dataTotDist;
    double dataTotDist = 0.0;
    wxString oldLogFile;
    wxFile m_oldLogFileName;

    double tripLog = 0.0;
    int ReadTripFromData;
    wxString m_dataTripDist;
    double dataTripDist = 0.0;
    wxString departure = " - - - ";
    wxString dataCurrTime;
    int ReadDepFromData;
    wxString arrival = " - - - ";
    wxString restart = " ";
    int ReadArrFromData;
    int ArrTimeSet = 1;
    int ResTimeSet = 1;
    int ReadResFromData;
    int ReadSavedData = 0;
    wxString currLogLine;

    // Log file handling
    wxString m_DataFile;
    wxTextFile m_DataFileTextReader;
    int readDataFile = 0;
    wxString m_LogFile;
    wxFile m_LogFileName;
    int TripMode = 0;
    int StopMinutes = 5;
    int ReadTripModeFromData;
    int saveTripMode = 0;
    wxString strTripMode;
    int AutoReset = 0;
    int ReadTripAutoResetFromData;
    int saveTripAutoReset = 0;
    wxString strTripAutoReset;
    int TimeDateWritten = 0;
    int saveDepTime = 0;
    int DepTimeFound;
    int saveArrTime = 0;
    int ArrTimeFound;
    int saveResTime = 0;
    int ResTimeFound;
    wxString m_ResumeAfter;
    wxString strLogtrip;
    wxString strLogtripsum;
    wxString docTitle;
    wxString LogFile;

    wxString homeDir;
    wxString TripLogFile;
    wxFile TripLogFileName;
    int restartNum;
    wxString arrivalString;
    int arrivalPos;
    int prevArrivalPos;
    wxString departString;
    int departPos;
    int prevDepartPos;
    wxString EOLString;
    int EOLPos;
    int prevEOLPos;
    wxString restartString;
    int restartPos;
    int prevRestartPos = 0;
    int numRestarts = 0;
    wxString tripString;
    int tripPos;
    int prevTripPos;
    int tripLen;
    int tripDecimalPos;
    wxString space = " ";
    wxString moveResColumns;
    wxString newResRow;
    wxString subStrLog;
    int subLength;
    int restartLen;

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
    void OnInstrumentAdd();
    void OnInstrumentDelete();

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
    wxChoice *m_pChoiceTripDelayTime;
    wxChoice *m_pChoiceMinTripStop;
    wxSpinCtrlDouble *m_pSpinDBTOffset;
	wxCheckBox *m_pCheckBoxShowSpeed;
	wxCheckBox *m_pCheckBoxShowDepArrTimes;
	wxCheckBox *m_pCheckBoxShowTripLeg;
	wxCheckBox *m_pCheckBoxIncludeTripStops;
//	wxCheckBox *m_pCheckBoxAutoResetTrip;


private:
	void UpdateOdometerButtonsState(void);
    wxChoice *m_pChoiceOrientation;
	wxListCtrl *m_pListCtrlOdometers;
	wxPanel *m_pPanelPreferences;
	wxTextCtrl *m_pTextCtrlCaption;
	wxCheckBox *m_pCheckBoxIsVisible;
	wxListCtrl *m_pListCtrlInstruments;

};

class OdometerViewLogDialog : public wxDialog {
public:
	OdometerViewLogDialog(wxWindow *pparent, wxWindowID id);
	~OdometerViewLogDialog() {}

	void OnCloseLogDialog(wxCloseEvent& event);
    void SaveLogConfig();

	wxFontPickerCtrl *m_pFontPickerTitle;
	wxFontPickerCtrl *m_pFontPickerData;
	wxFontPickerCtrl *m_pFontPickerLabel;
	wxFontPickerCtrl *m_pFontPickerSmall;
	wxRadioBox *m_pRadioBoxTrips;
	wxRadioBox *m_pRadioBoxFormat;
	wxRadioBox *m_pRadioBoxOutput;


private:
	wxPanel *m_pLogPanelPreferences;
	wxPanel *m_pRadioBoxPanel;
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
    ID_ODO_VIEWLOG,
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
    int orient = 0;  // Always vertical

    void ViewLogDialog(wxWindow *parent);
    wxBoxSizer *OdoBoxSizer;
	OdometerWindow *m_pOdometerWindow;


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
	wxArrayOfInstrument m_ArrayOfInstrument;
    wxArrayOfOdometer m_ArrayOfOdometerWindow;
};



#endif

