//
// This file is part of GPS Odometer, a plugin for OpenCPN.
// based on the original version of the dashboard.
//
/******************************************************************************
 * $Id: button.cpp, v1.0 2010/08/05 SethDart Exp $
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


#include "button.h"
//#include "wx28compat.h"

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

#include <cmath>
#include "wx/tokenzr.h"

extern int g_iResetTrip;
extern int g_iStartStopLeg;
extern int g_iResetLeg;
extern int g_iShowLogDialog;
extern int g_iOdoCurrWidth;


//--------------------------------------------------------------
//
//  Buttons
//
//--------------------------------------------------------------

// Executed at start and when closing setup
OdometerInstrument_Button::OdometerInstrument_Button(wxWindow *pparent, wxWindowID id,
    wxString title, int cap_flag) : OdometerInstrument(pparent, id, title, cap_flag )
{

    m_id = id;
    m_title = title;
    m_cap_flag = cap_flag;
//    instrumentTypeId = 0;
//    SetBackgroundStyle( wxBG_STYLE_CUSTOM );
    SetDrawSoloInPane(false);
    wxClientDC dc(this);
    int width;
    dc.GetTextExtent(m_title, &width, &m_TitleHeight, 0, 0, g_pFontTitle);
    dc.GetTextExtent(_T("000"), &width, &m_DataHeight, 0, 0, g_pFontData);

    Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(OdometerInstrument::OnEraseBackground));
    Connect(wxEVT_PAINT, wxPaintEventHandler(OdometerInstrument::OnPaint));

#ifdef __WXOSX__
    Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(OdometerInstrument::MouseEvent), NULL, this);
#endif

}

OdometerInstrument_Button::~OdometerInstrument_Button(void) {
}

// Executed at start, when closing setup and when instument size is changed
wxSize OdometerInstrument_Button::GetSize(int orient, wxSize hint) {
    wxClientDC dc(this);
    int w;
    dc.GetTextExtent(m_title, &w, &m_TitleHeight, 0, 0, g_pFontTitle);
    dc.GetTextExtent(_T("000"), &w, &m_DataHeight, 0, 0, g_pFontData);

    wxSize size = GetClientSize();
//    wxSize size = GetMinSize();


    // Set resonable values
    if (hint.x < 150) hint.x = 150;
    if (hint.y < 28) hint.y = 28;
    if (hint.y > 30) hint.y = 30;

    b_width = hint.x;
//    b_width = g_iOdoCurrWidth;
    if (b_width < 150) b_width = 150;

    if (size.y < 28) size.y = 28;
    b_height = m_TitleHeight + 12;
    if (b_height < 29) b_height = 29;

    if (m_cap_flag == 32) {
        wxBoxSizer* instrument = new wxBoxSizer(wxVERTICAL);
        wxButton* m_pTripResetButton = new wxButton(this, m_id, _( m_title ), 
            wxDefaultPosition, wxSize(b_width,b_height), wxFULL_REPAINT_ON_RESIZE);
        m_pTripResetButton->SetForegroundColour(wxColour(0,0,0));
        m_pTripResetButton->SetFont(*g_pFontTitle);

        instrument->Add(m_pTripResetButton, 0, wxEXPAND | wxALL | wxFULL_REPAINT_ON_RESIZE, 5 );
        m_pTripResetButton->Raise();
        m_pTripResetButton->Refresh();

        m_pTripResetButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, 
            wxCommandEventHandler(OdometerInstrument_Button::OnButtonClickTripReset), 
            NULL, this );
    }

    if (m_cap_flag == 256) {
        wxBoxSizer* instrument = new wxBoxSizer(wxVERTICAL);
        wxButton* m_pLegStartStopButton = new wxButton(this, m_id, _( m_title ),
            wxDefaultPosition, wxSize(b_width,b_height), wxFULL_REPAINT_ON_RESIZE );
        m_pLegStartStopButton->SetForegroundColour(wxColor(0,0,0));        
        m_pLegStartStopButton->SetFont(*g_pFontTitle);

        instrument->Add(m_pLegStartStopButton, 0, wxEXPAND | wxALL | wxFULL_REPAINT_ON_RESIZE, 5 );
        m_pLegStartStopButton->Raise();
        m_pLegStartStopButton->Refresh();

        m_pLegStartStopButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, 
            wxCommandEventHandler(OdometerInstrument_Button::OnButtonClickStartStop), 
            NULL, this );
    }

    if (m_cap_flag == 512) {
        wxBoxSizer* instrument = new wxBoxSizer(wxVERTICAL);
        wxButton* m_pLegResetButton = new wxButton(this, m_id, _( m_title ), 
            wxDefaultPosition, wxSize(b_width,b_height), wxFULL_REPAINT_ON_RESIZE );
        m_pLegResetButton->SetForegroundColour(wxColor(0,0,0));        
        m_pLegResetButton->SetFont(*g_pFontTitle);

        instrument->Add(m_pLegResetButton, 0, wxEXPAND | wxALL | wxFULL_REPAINT_ON_RESIZE, 5 );
        m_pLegResetButton->Raise();
        m_pLegResetButton->Refresh();

        m_pLegResetButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, 
            wxCommandEventHandler(OdometerInstrument_Button::OnButtonClickLegReset), 
            NULL, this );
    }
    return wxSize(wxMax(g_iOdoCurrWidth, DefaultWidth), b_height);
}

void OdometerInstrument_Button::SetData(int st, double data, wxString unit) {
}

void OdometerInstrument_Button::OnButtonClickTripReset( wxCommandEvent& event)  {

    if ( event.GetInt() == 0 )  g_iResetTrip = 1; 
} 

void OdometerInstrument_Button::OnButtonClickStartStop( wxCommandEvent& event)  {

    if ( event.GetInt() == 0 )  g_iStartStopLeg = 1; 
} 

void OdometerInstrument_Button::OnButtonClickLegReset( wxCommandEvent& event)  {

    if ( event.GetInt() == 0 )  g_iResetLeg = 1; 
} 


void OdometerInstrument_Button::Draw(wxGCDC* dc) {

      wxColour cl;

#ifdef __WXMSW__
      wxBitmap tbm(dc->GetSize().x, m_DataHeight, -1);
      wxMemoryDC tdc(tbm);
      wxColour c2;
      GetGlobalColor(_T("DASHB"), &c2);
      tdc.SetBackground(c2);
      tdc.Clear();

      tdc.SetFont(*g_pFontTitle);
      GetGlobalColor(_T("DASHF"), &cl);
      tdc.SetTextForeground(cl);

      tdc.DrawText(m_data, 10, 0);

      tdc.SelectObject(wxNullBitmap);

      dc->DrawBitmap(tbm, 0, m_TitleHeight, false);
#else
      dc->SetFont(*g_pFontTitle);
      GetGlobalColor(_T("DASHF"), &cl);
      dc->SetTextForeground(cl);
      dc->DrawText(m_data, 10, m_TitleHeight);
#endif

}
