//
// This file is part of GPS Odometer, a plugin for OpenCPN.
// based on the original version of the dashboard.
//
/***************************************************************************
 * $Id: instrument.h, v1.0 2010/08/30 SethDart Exp $
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

#ifndef _INSTRUMENT_H_
#define _INSTRUMENT_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#if !wxUSE_GRAPHICS_CONTEXT
#define wxGCDC wxDC
#endif

// Required GetGlobalColor
#include "ocpn_plugin.h"
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>         // supplemental, for Mac

// This is the degree sign in UTF8. It should be correctly handled on both Win & Unix
const wxString DEGREE_SIGN = wxString::Format(_T("%c"), 0x00B0); 

#define DefaultWidth 150

extern wxFont *g_pFontTitle;
extern wxFont *g_pFontData;
extern wxFont *g_pFontLabel;
extern wxFont *g_pFontSmall;

wxString toSDMM(int NEflag, double a);

class OdometerInstrument;
class OdometerInstrument_Single;
class OdometerInstrument_String;

enum {
	OCPN_DBP_STC_SOG      = 1 << 0,
    OCPN_DBP_STC_SUMLOG   = 1 << 1,
    OCPN_DBP_STC_TRIPLOG  = 1 << 2,
    OCPN_DBP_STC_DEPART   = 1 << 3,
    OCPN_DBP_STC_ARRIV    = 1 << 4,
    OCPN_DBP_STC_TRIPRES  = 1 << 5,  // Number referenced in button module, do not change!
    OCPN_DBP_STC_LEGDIST  = 1 << 6,
    OCPN_DBP_STC_LEGTIME  = 1 << 7,
    OCPN_DBP_STC_LEGRES   = 1 << 8,  // Number referenced in button module, do not change!
};


class OdometerInstrument : public wxControl {
public:
	OdometerInstrument(wxWindow *pparent, wxWindowID id, wxString title, int cap_flag);
	~OdometerInstrument(){}

	int GetCapacity();
	void OnEraseBackground(wxEraseEvent &WXUNUSED(evt));
	virtual wxSize GetSize(int orient, wxSize hint) = 0;
	void OnPaint(wxPaintEvent &WXUNUSED(event));
	virtual void SetData(int st, double data, wxString unit) = 0;
	void SetDrawSoloInPane(bool value);
	void MouseEvent(wxMouseEvent &event);
      
	int instrumentTypeId;

protected:
	int m_id;
	int m_cap_flag;
	int m_TitleHeight;
	wxString m_title;
	virtual void Draw(wxGCDC *dc) = 0;

private:
	bool m_drawSoloInPane;
};

class OdometerInstrument_Single : public OdometerInstrument {
public:
	OdometerInstrument_Single(wxWindow *pparent, wxWindowID id, wxString title, int cap, wxString format);
	~OdometerInstrument_Single(){}

	wxSize GetSize(int orient, wxSize hint);
	void SetData(int st, double data, wxString unit);

protected:
	wxString m_data;
	wxString m_format;
	int m_DataHeight;
	
	void Draw(wxGCDC *dc);
};

class OdometerInstrument_String : public OdometerInstrument {
public:
	OdometerInstrument_String(wxWindow *pparent, wxWindowID id, wxString title, int cap, wxString format);
	~OdometerInstrument_String(){}

	wxSize GetSize(int orient, wxSize hint);
	void SetData(int st, double data, wxString unit);

protected:
	wxString m_data;
	wxString m_format;
	int m_DataHeight;
	
	void Draw(wxGCDC *dc);
};

#endif // _INSTRUMENT_H_
