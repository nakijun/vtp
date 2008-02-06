//
// Name: RenderDlg.h
//
// Copyright (c) 2004-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifndef __RenderDlg_H__
#define __RenderDlg_H__

#include "VTBuilder_wdr.h"
#include "vtdata/vtString.h"
#include "vtdata/MathTypes.h"
#include "vtui/AutoDialog.h"

// WDR: class declarations

//----------------------------------------------------------------------------
// RenderDlg
//----------------------------------------------------------------------------

class RenderDlg: public AutoDialog
{
public:
	// constructors and destructors
	RenderDlg( wxWindow *parent, wxWindowID id,
		const wxString &title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_DIALOG_STYLE );

	// WDR: method declarations for RenderDlg
	wxRadioButton* GetJpeg()  { return (wxRadioButton*) FindWindow( ID_JPEG ); }
	wxRadioButton* GetGeotiff()  { return (wxRadioButton*) FindWindow( ID_GEOTIFF ); }
	wxButton* GetDotdotdot()  { return (wxButton*) FindWindow( ID_DOTDOTDOT ); }
	wxTextCtrl* GetTextToFile()  { return (wxTextCtrl*) FindWindow( ID_TEXT_TO_FILE ); }
	wxCheckBox* GetTiling()  { return (wxCheckBox*) FindWindow( ID_TILING ); }
	wxButton* GetBigger()  { return (wxButton*) FindWindow( ID_BIGGER ); }
	wxButton* GetSmaller()  { return (wxButton*) FindWindow( ID_SMALLER ); }
	wxTextCtrl* GetSizeY()  { return (wxTextCtrl*) FindWindow( ID_SIZEY ); }
	wxTextCtrl* GetSizeX()  { return (wxTextCtrl*) FindWindow( ID_SIZEX ); }
	wxChoice* GetColorMap()  { return (wxChoice*) FindWindow( ID_CHOICE_COLORS ); }
	wxBitmapButton* GetColorNodata()  { return (wxBitmapButton*) FindWindow( ID_COLOR_NODATA ); }

	void RecomputeSize();
	void UpdateEnabling();
	void UpdateColorMapChoice();

	bool m_bToFile;
	wxString m_strToFile;
	bool m_bJPEG;

	wxString   m_strColorMap;
	bool m_bShading;

	int  m_iSizeX;
	int  m_iSizeY;
	bool m_bConstraint;
	bool m_bTiling;
	int  m_power;

	RGBi m_ColorNODATA;

	bool m_bSetting;

private:
	// WDR: member variable declarations for RenderDlg

private:
	// WDR: handler declarations for RenderDlg
	void OnColorNODATA( wxCommandEvent &event );
	void OnEditColors( wxCommandEvent &event );
	void OnBigger( wxCommandEvent &event );
	void OnSmaller( wxCommandEvent &event );
	void OnConstrain( wxCommandEvent &event );
	void OnDotdotdot( wxCommandEvent &event );
	void OnRadio( wxCommandEvent &event );
	void OnInitDialog(wxInitDialogEvent& event);

private:
	DECLARE_EVENT_TABLE()
};

#endif
