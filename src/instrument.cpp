//
// This file is part of GPS Odometer, a plugin for OpenCPN.
// based on the original version of the dashboard.
//
/******************************************************************************
 * $Id: instrument.cpp, v1.0 2010/08/30 SethDart Exp $
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

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers
#include <cmath>

#include "instrument.h"
//#include "wx28compat.h"

extern    int   g_iAutoResetTrip;

//----------------------------------------------------------------
//
//    Generic OdometerInstrument Implementation
//
//----------------------------------------------------------------

OdometerInstrument::OdometerInstrument(wxWindow *pparent, wxWindowID id, wxString title, int cap_flag)
      :wxControl(pparent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE) {

      m_title = title;
      m_cap_flag = cap_flag;

      SetBackgroundStyle(wxBG_STYLE_CUSTOM);
      SetDrawSoloInPane(false);
      wxClientDC dc(this);
      int width;
      dc.GetTextExtent(m_title, &width, &m_TitleHeight, 0, 0, g_pFontTitle);

      Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(OdometerInstrument::OnEraseBackground));
      Connect(wxEVT_PAINT, wxPaintEventHandler(OdometerInstrument::OnPaint));
      
      //  On OSX, there is an orphan mouse event that comes from the automatic
      //  exEVT_CONTEXT_MENU synthesis on the main wxWindow mouse handler.
      //  The event goes to an instrument window (here) that may have been deleted by the
      //  preferences dialog.  Result is NULL deref.
      //  Solution:  Handle right-click here, and DO NOT skip()
      //  Strangely, this does not work for GTK...
      //  See: http://trac.wxwidgets.org/ticket/15417
      
#ifdef __WXOSX__
      Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(OdometerInstrument::MouseEvent), NULL, this);
#endif      
}

void OdometerInstrument::MouseEvent(wxMouseEvent &event) {
    if (event.GetEventType() == wxEVT_RIGHT_DOWN) {
		wxContextMenuEvent evtCtx(wxEVT_CONTEXT_MENU, this->GetId(), this->ClientToScreen(event.GetPosition()));
        evtCtx.SetEventObject(this);
        GetParent()->GetEventHandler()->AddPendingEvent(evtCtx);
    }
}

int OdometerInstrument::GetCapacity() {
	return m_cap_flag;
}

void OdometerInstrument::SetDrawSoloInPane(bool value) {
    m_drawSoloInPane = value;
}

void OdometerInstrument::OnEraseBackground(wxEraseEvent& WXUNUSED(evt)) {
        // intentionally empty
}


void OdometerInstrument::OnPaint(wxPaintEvent& WXUNUSED(event)) {

    wxAutoBufferedPaintDC pdc(this);
    if (!pdc.IsOk()) {
        wxLogMessage(_T("OdometerInstrument::OnPaint() fatal: wxAutoBufferedPaintDC.IsOk() false."));
        return;
    }
    // GetClientSize() reports error when window is resized to cover instruments
//    wxSize size = GetClientSize(); 
    wxSize size = GetMinSize(); 

    if (size.x == 0 || size.y == 0) {
        wxLogMessage(_T("OdometerInstrument::OnPaint() fatal: Zero size DC."));
        return;
    }

#if wxUSE_GRAPHICS_CONTEXT
    wxGCDC dc(pdc);
#else
    wxDC &dc(pdc);
#endif

    wxColour cl;
    GetGlobalColor(_T("DASHB"), &cl);
    dc.SetBackground(cl);
#ifdef __WXGTK__
    dc.SetBrush(cl);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, size.x, size.y);
#endif
    dc.Clear();

    Draw(&dc);

    if (!m_drawSoloInPane) {

    //  Windows GCDC does a terrible job of rendering small texts
    //  Workaround by using plain old DC for title box if text size is too small
#ifdef __WXMSW__
        if (g_pFontTitle->GetPointSize() > 12)
#endif
        {
            wxPen pen;
            pen.SetStyle(wxPENSTYLE_SOLID);
            GetGlobalColor(_T("DASHL"), &cl);
            pen.SetColour(cl);
            dc.SetPen(pen);
            dc.SetBrush(cl);
            dc.DrawRoundedRectangle(0, 0, size.x, m_TitleHeight, 3);

            dc.SetFont(*g_pFontTitle);
            GetGlobalColor(_T("DASHF"), &cl);
            dc.SetTextForeground(cl);
            dc.DrawText(m_title, 5, 0);
        }

#ifdef __WXMSW__
        if (g_pFontTitle->GetPointSize() <= 12) {
            wxColour cl;
            GetGlobalColor(_T("DASHB"), &cl);
            pdc.SetBrush(cl);
            pdc.DrawRectangle(0, 0, size.x, m_TitleHeight);

            wxPen pen;
            pen.SetStyle(wxPENSTYLE_SOLID);
            GetGlobalColor(_T("DASHL"), &cl);
            pen.SetColour(cl);
            pdc.SetPen(pen);
            pdc.SetBrush(cl);
            pdc.DrawRoundedRectangle(0, 0, size.x, m_TitleHeight, 3);

            pdc.SetFont(*g_pFontTitle);
            GetGlobalColor(_T("DASHF"), &cl);
            pdc.SetTextForeground(cl);
            pdc.DrawText(m_title, 5, 0);
        }
#endif
    }
}

//----------------------------------------------------------------
//
//    OdometerInstrument_Single Implementation
//
//----------------------------------------------------------------

OdometerInstrument_Single::OdometerInstrument_Single(wxWindow *pparent, wxWindowID id, wxString title, int cap_flag, wxString format)
      :OdometerInstrument(pparent, id, title, cap_flag) {
      m_format = format;
      m_data = _T("---");
      m_DataHeight = 0;
}

wxSize OdometerInstrument_Single::GetSize(int orient, wxSize hint) {
      wxClientDC dc(this);
      int w;
      dc.GetTextExtent(m_title, &w, &m_TitleHeight, 0, 0, g_pFontTitle);
      dc.GetTextExtent(_T("000"), &w, &m_DataHeight, 0, 0, g_pFontData);

      if (orient == wxHORIZONTAL) {
          return wxSize(DefaultWidth, wxMax(hint.y, m_TitleHeight+m_DataHeight));
      } else {
          return wxSize(wxMax(hint.x, DefaultWidth), m_TitleHeight+m_DataHeight);
      }
}

void OdometerInstrument_Single::Draw(wxGCDC* dc) {
      wxColour cl;
#ifdef __WXMSW__
      wxBitmap tbm(dc->GetSize().x, m_DataHeight, -1);
      wxMemoryDC tdc(tbm);
      wxColour c2;
      GetGlobalColor(_T("DASHB"), &c2);
      tdc.SetBackground(c2);
      tdc.Clear();

      tdc.SetFont(*g_pFontData);
      GetGlobalColor(_T("DASHF"), &cl);
      tdc.SetTextForeground(cl);

      tdc.DrawText(m_data, 10, 0);

      tdc.SelectObject(wxNullBitmap);

      dc->DrawBitmap(tbm, 0, m_TitleHeight, false);
#else
      dc->SetFont(*g_pFontData);
      GetGlobalColor(_T("DASHF"), &cl);
      dc->SetTextForeground(cl);

      dc->DrawText(m_data, 10, m_TitleHeight);

#endif
}

void OdometerInstrument_Single::SetData(int st, double data, wxString unit) {
      if (m_cap_flag & st) {
            if (!std::isnan(data) && (data < 999999)) {
                if (unit == _T("C"))
                  m_data = wxString::Format(m_format, data)+DEGREE_SIGN+_T("C");
                else if (unit == _T("\u00B0"))
                  m_data = wxString::Format(m_format, data)+DEGREE_SIGN;
                else if (unit == _T("\u00B0T"))
                  m_data = wxString::Format(m_format, data)+DEGREE_SIGN+_(" true");
                else if (unit == _T("\u00B0M"))
                  m_data = wxString::Format(m_format, data)+DEGREE_SIGN+_(" mag");
                else if (unit == _T("\u00B0L"))
                  m_data = _T(">")+ wxString::Format(m_format, data)+DEGREE_SIGN;
                else if (unit == _T("\u00B0R"))
                  m_data = wxString::Format(m_format, data)+DEGREE_SIGN+_T("<");
                else if (unit == _T("N")) //Knots
                  m_data = wxString::Format(m_format, data)+_T(" Kts");
/* maybe in the future ...
                else if (unit == _T("M")) // m/s
                  m_data = wxString::Format(m_format, data)+_T(" m/s");
                else if (unit == _T("K")) // km/h
                  m_data = wxString::Format(m_format, data)+_T(" km/h");
 ... to be completed
 */
                else
                  m_data = wxString::Format(m_format, data)+_T(" ")+unit;
            }
            else
                m_data = _T("---");
      }
}


//----------------------------------------------------------------
//
//    OdometerInstrument_String Implementation
//
//----------------------------------------------------------------

OdometerInstrument_String::OdometerInstrument_String(wxWindow *pparent, wxWindowID id, wxString title, int cap_flag, wxString format)
      :OdometerInstrument(pparent, id, title, cap_flag) {
      m_format = format;
      m_data = _T("---");
      m_DataHeight = 0;
}

wxSize OdometerInstrument_String::GetSize(int orient, wxSize hint) {
      wxClientDC dc(this);
      int w;
//      dc.SetTextAlign(TA_CENTER);
      dc.GetTextExtent(m_title, &w, &m_TitleHeight, 0, 0, g_pFontTitle);
      dc.GetTextExtent(_T("000"), &w, &m_DataHeight, 0, 0, g_pFontData);

      if (orient == wxHORIZONTAL) {
          return wxSize(DefaultWidth, wxMax(hint.y, m_TitleHeight+m_DataHeight));
      } else {
          return wxSize(wxMax(hint.x, DefaultWidth), m_TitleHeight+m_DataHeight);
      }
}

void OdometerInstrument_String::Draw(wxGCDC* dc) {
      wxColour cl;
#ifdef __WXMSW__
      wxBitmap tbm(dc->GetSize().x, m_DataHeight, -1);
      wxMemoryDC tdc(tbm);
      wxColour c2;
      GetGlobalColor(_T("DASHB"), &c2);
      tdc.SetBackground(c2);
      tdc.Clear();

      tdc.SetFont(*g_pFontData);
      GetGlobalColor(_T("DASHF"), &cl);
      tdc.SetTextForeground(cl);

      tdc.DrawText(m_data, 10, 0);

      tdc.SelectObject(wxNullBitmap);

      dc->DrawBitmap(tbm, 0, m_TitleHeight, false);
#else
      dc->SetFont(*g_pFontData);
      GetGlobalColor(_T("DASHF"), &cl);
      dc->SetTextForeground(cl);

      dc->DrawText(m_data, 10, m_TitleHeight);

#endif
}

void OdometerInstrument_String::SetData(int st, double data, wxString unit) {
    if (m_cap_flag & st) {
        if (!std::isnan(data) && (data < 999999)) {
            m_data = wxString::Format( m_format , " " )+unit; 
        }
    else
            m_data = _T("---");
    }
}

//----------------------------------------------------------------
//
//    OdometerInstrument_Checkbox Implementation
//
//----------------------------------------------------------------

OdometerInstrument_Checkbox::OdometerInstrument_Checkbox(wxWindow *pparent, wxWindowID id,
     wxString title, int autoReset) :OdometerInstrument(pparent, id, title, autoReset)
{

    m_id = id;
    m_title = title;
    wxClientDC dc(this);
    int width;
    dc.GetTextExtent(m_title, &width, &m_TitleHeight, 0, 0, g_pFontTitle);

    Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(OdometerInstrument::OnEraseBackground));
    Connect(wxEVT_PAINT, wxPaintEventHandler(OdometerInstrument::OnPaint));

#ifdef __WXOSX__
    Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(OdometerInstrument::MouseEvent), NULL, this);
#endif

    int c_autoReset = autoReset;
    int c_width;
    int c_height;
    c_width = width;
    c_height = m_TitleHeight;


    wxBoxSizer* instrument = new wxBoxSizer(wxVERTICAL);
    SetSizer(instrument);

    m_AutoTripReset = new wxCheckBox(this, m_id, _( m_title ), wxDefaultPosition,
        wxSize(c_width,c_height));
    m_AutoTripReset->SetForegroundColour(wxColor(0,0,0));
    m_AutoTripReset->SetFont(*g_pFontTitle);
    m_AutoTripReset->SetValue(c_autoReset);

    instrument->Add(m_AutoTripReset, 1, wxEXPAND);
    m_title = ( "" );  // Remove the title row from behind the checkbox

    m_AutoTripReset->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &OdometerInstrument_Checkbox::autoreset,
        this);
}

void OdometerInstrument_Checkbox::autoreset(wxCommandEvent &event) {
    if (m_AutoTripReset->GetValue())
        g_iAutoResetTrip = 1;
     else
        g_iAutoResetTrip = 0;
}

wxSize OdometerInstrument_Checkbox::GetSize(int orient, wxSize hint) {
      wxClientDC dc(this);
      int w;
      dc.GetTextExtent(m_title, &w, &m_TitleHeight, 0, 0, g_pFontTitle);
      dc.GetTextExtent(_T("000"), &w, &m_DataHeight, 0, 0, g_pFontData);

      if (orient == wxHORIZONTAL) {
          return wxSize(DefaultWidth, wxMax(hint.y, m_TitleHeight+m_DataHeight + 6 ));
      } else {
          return wxSize(wxMax(hint.x, DefaultWidth), m_TitleHeight+m_DataHeight + 6 );
      }
}

void OdometerInstrument_Checkbox::Draw(wxGCDC* dc) {
      wxColour cl;
#ifdef __WXMSW__
      wxBitmap tbm(dc->GetSize().x, m_DataHeight, -1);
      wxMemoryDC tdc(tbm);
      wxColour c2;
      GetGlobalColor(_T("DASHB"), &c2);
      tdc.SetBackground(c2);
      tdc.Clear();

      tdc.SetFont(*g_pFontData);
      GetGlobalColor(_T("DASHF"), &cl);
      tdc.SetTextForeground(cl);

      tdc.DrawText(m_data, 10, 0);

      tdc.SelectObject(wxNullBitmap);

      dc->DrawBitmap(tbm, 0, m_TitleHeight, false);
#else
      dc->SetFont(*g_pFontData);
      GetGlobalColor(_T("DASHF"), &cl);
      dc->SetTextForeground(cl);

      dc->DrawText(m_data, 10, m_TitleHeight);

#endif
}

void OdometerInstrument_Checkbox::SetData(int st, double data, wxString unit) {

}

