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
extern int g_iOdoCurrWidth;


//--------------------------------------------------------------
//
//  Trip reset button
//
//--------------------------------------------------------------

// Executed at start and when closing setup
OdometerInstrument_TripResButton::OdometerInstrument_TripResButton(wxWindow *pparent, 
    wxWindowID id, wxString title, int cap_flag) 
    :OdometerInstrument(pparent, id, title, cap_flag )
{

    m_id = id;
    m_title = title;
    m_cap_flag = cap_flag;
    SetDrawSoloInPane(false);
    wxClientDC dc(this);
    int width;
    dc.GetTextExtent(m_title, &width, &m_TitleHeight, 0, 0, g_pFontTitle);

    Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(OdometerInstrument::OnEraseBackground));
    Connect(wxEVT_PAINT, wxPaintEventHandler(OdometerInstrument::OnPaint));
    Connect( wxEVT_COMMAND_BUTTON_CLICKED, 
        wxCommandEventHandler(OdometerInstrument_TripResButton::OnButtonClickTripReset),
        NULL, this );

#ifdef __WXOSX__
    Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(OdometerInstrument::MouseEvent), NULL, this);
#endif

    b_width = 157;   // Standard initial default width
    b_height = m_TitleHeight + 12;

    wxBoxSizer* buttonbox = new wxBoxSizer(wxHORIZONTAL);
    wxButton* m_pTripResetButton = new wxButton(this, m_id, _( m_title ), 
        wxDefaultPosition, wxSize(b_width,b_height));
    m_pTripResetButton->SetForegroundColour(wxColour(0,0,0));
    m_pTripResetButton->SetFont(*g_pFontTitle);
    buttonbox->Add(m_pTripResetButton, 1, wxEXPAND | wxALL | wxFULL_REPAINT_ON_RESIZE, 0 );

    SetSizerAndFit(buttonbox);
    buttonbox->Show(1, true);
    buttonbox->Layout();
}

OdometerInstrument_TripResButton::~OdometerInstrument_TripResButton(void) {
}

// Executed at start, when closing setup and when instument size is changed
wxSize OdometerInstrument_TripResButton::GetSize(int orient, wxSize hint) {

    wxClientDC dc(this);
    int w;
    dc.GetTextExtent(m_title, &w, &m_TitleHeight, 0, 0, g_pFontTitle);
    return wxSize(wxMax(hint.x, DefaultWidth), m_TitleHeight + 12);
}

void OdometerInstrument_TripResButton::SetData(int st, double data, wxString unit) {
}

void OdometerInstrument_TripResButton::OnButtonClickTripReset( wxCommandEvent& event)  {
    if ( event.GetInt() == 0 )  g_iResetTrip = 1; 
} 

 void OdometerInstrument_TripResButton::Draw(wxGCDC* dc) {
      wxColour cl;

#ifdef __WXMSW__
      wxBitmap tbm(dc->GetSize().x, m_TitleHeight + 12, -1);
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

//--------------------------------------------------------------
//
//  Leg start/stop button
//
//--------------------------------------------------------------

// Executed at start and when closing setup
OdometerInstrument_LegStartStopButton::OdometerInstrument_LegStartStopButton(wxWindow *pparent, 
    wxWindowID id, wxString title, int cap_flag) 
    :OdometerInstrument(pparent, id, title, cap_flag )
{

    m_id = id;
    m_title = title;
    m_cap_flag = cap_flag;
    SetDrawSoloInPane(false);
    wxClientDC dc(this);
    int width;
    dc.GetTextExtent(m_title, &width, &m_TitleHeight, 0, 0, g_pFontTitle);

    Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(OdometerInstrument::OnEraseBackground));
    Connect(wxEVT_PAINT, wxPaintEventHandler(OdometerInstrument::OnPaint));
    Connect( wxEVT_COMMAND_BUTTON_CLICKED, 
        wxCommandEventHandler(OdometerInstrument_LegStartStopButton::OnButtonClickStartStop),
        NULL, this );

#ifdef __WXOSX__
    Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(OdometerInstrument::MouseEvent), NULL, this);
#endif

    b_width = 157;   // Standard initial default width
    b_height = m_TitleHeight + 12;

    wxBoxSizer* buttonbox = new wxBoxSizer(wxHORIZONTAL);
    wxButton* m_pLegStartStopButton = new wxButton(this, m_id, _( m_title ), 
        wxDefaultPosition, wxSize(b_width,b_height));
    m_pLegStartStopButton->SetForegroundColour(wxColour(0,0,0));
    m_pLegStartStopButton->SetFont(*g_pFontTitle);
    buttonbox->Add(m_pLegStartStopButton, 1, wxEXPAND | wxALL | wxFULL_REPAINT_ON_RESIZE, 0 );

    SetSizerAndFit(buttonbox);
    buttonbox->Show(1, true);
    buttonbox->Layout();
}

OdometerInstrument_LegStartStopButton::~OdometerInstrument_LegStartStopButton(void) {
}

// Executed at start, when closing setup and when instument size is changed
wxSize OdometerInstrument_LegStartStopButton::GetSize(int orient, wxSize hint) {

    wxClientDC dc(this);
    int w;
    dc.GetTextExtent(m_title, &w, &m_TitleHeight, 0, 0, g_pFontTitle);
    return wxSize(wxMax(hint.x, DefaultWidth), m_TitleHeight + 12);
}

void OdometerInstrument_LegStartStopButton::SetData(int st, double data, wxString unit) {
}

void OdometerInstrument_LegStartStopButton::OnButtonClickStartStop( wxCommandEvent& event)  {
    if ( event.GetInt() == 0 )  g_iStartStopLeg = 1; 
} 

 void OdometerInstrument_LegStartStopButton::Draw(wxGCDC* dc) {
      wxColour cl;

#ifdef __WXMSW__
      wxBitmap tbm(dc->GetSize().x, m_TitleHeight + 12, -1);
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


//--------------------------------------------------------------
//
//  Leg reset button
//
//--------------------------------------------------------------

// Executed at start and when closing setup
OdometerInstrument_LegResetButton::OdometerInstrument_LegResetButton(wxWindow *pparent, 
    wxWindowID id, wxString title, int cap_flag) 
    :OdometerInstrument(pparent, id, title, cap_flag )
{

    m_id = id;
    m_title = title;
    m_cap_flag = cap_flag;
    SetDrawSoloInPane(false);
    wxClientDC dc(this);
    int width;
    dc.GetTextExtent(m_title, &width, &m_TitleHeight, 0, 0, g_pFontTitle);

    Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(OdometerInstrument::OnEraseBackground));
    Connect(wxEVT_PAINT, wxPaintEventHandler(OdometerInstrument::OnPaint));
    Connect(wxEVT_COMMAND_BUTTON_CLICKED,
        wxCommandEventHandler(OdometerInstrument_LegResetButton::OnButtonClickLegReset),
        NULL, this );


#ifdef __WXOSX__
    Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(OdometerInstrument::MouseEvent), NULL, this);
#endif

    b_width = 157;   // Standard initial default width
    b_height = m_TitleHeight + 12;


    wxBoxSizer* buttonbox = new wxBoxSizer(wxHORIZONTAL);
    wxButton* m_pLegResetButton = new wxButton(this, m_id, _( m_title ), 
        wxDefaultPosition, wxSize(b_width,b_height));
    m_pLegResetButton->SetForegroundColour(wxColour(0,0,0));
    m_pLegResetButton->SetFont(*g_pFontTitle);
    buttonbox->Add(m_pLegResetButton, 1, wxEXPAND | wxALL | wxFULL_REPAINT_ON_RESIZE, 0 );

    SetSizerAndFit(buttonbox);
    buttonbox->Show(1, true);
    buttonbox->Layout();

}

OdometerInstrument_LegResetButton::~OdometerInstrument_LegResetButton(void) {
}

// Executed at start, when closing setup and when instument size is changed
wxSize OdometerInstrument_LegResetButton::GetSize(int orient, wxSize hint) {

    wxClientDC dc(this);
    int w;
    dc.GetTextExtent(m_title, &w, &m_TitleHeight, 0, 0, g_pFontTitle);
    return wxSize(wxMax(hint.x, DefaultWidth), m_TitleHeight + 12);
}

void OdometerInstrument_LegResetButton::SetData(int st, double data, wxString unit) {
}

void OdometerInstrument_LegResetButton::OnButtonClickLegReset(wxCommandEvent& event)  {
    if ( event.GetInt() == 0 )  g_iResetLeg = 1; 
} 

 void OdometerInstrument_LegResetButton::Draw(wxGCDC* dc) {
      wxColour cl;

#ifdef __WXMSW__
      wxBitmap tbm(dc->GetSize().x, m_TitleHeight + 12, -1);
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
