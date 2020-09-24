//
// This file is part of GPS Odometer, a plugin for OpenCPN.
// based on the original version of the dashboard.
//
/******************************************************************************
 * $Id: button.h, v1.0 2010/08/05 SethDart Exp $
 *
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin
 * Author:   Jean-Eudes Onfray
 *           (Inspired by original work from Andreas Heiming)
 *
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.             *
 ***************************************************************************
 */

#ifndef _BUTTON_H_
#define _BUTTON_H_

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "instrument.h"

// Required GetGlobalColor
#include "ocpn_plugin.h"
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>         // supplemental, for Mac
#include <string> 

extern wxFont *g_pFontTitle;
extern wxFont *g_pFontData;
extern wxFont *g_pFontLabel;
extern wxFont *g_pFontSmall;


//------------------------------------------------------------------------------
//
// CLASS:
//    OdometerInstrument_Button
//
// DESCRIPTION:
//    This class creates a button used for reset functions
//
//+------------------------------------------------------------------------------

class OdometerInstrument_Button: public OdometerInstrument {
public:
	OdometerInstrument_Button(wxWindow *parent, wxWindowID id, wxString title, int cap_flag );
	~OdometerInstrument_Button(void);

	wxSize GetSize(int orient, wxSize hint);
	void SetData(int, double, wxString);
    void OnButtonClickTripReset( wxCommandEvent& event);
    void OnButtonClickLegReset( wxCommandEvent& event);

    int b_width = 150;
    int b_height = 33;
    int GetNumber;

private:


protected:
	wxString m_data;
	int m_DataHeight;

	virtual void Draw(wxGCDC* dc);

};

#endif // _BUTTON_H_

