//
// Name: GenGridDlg.cpp
//
// Copyright (c) 2007 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
	#pragma implementation "GenGridDlg.cpp"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
	#pragma hdrstop
#endif

#include "GenGridDlg.h"

// WDR: class implementations

//----------------------------------------------------------------------------
// GenGridDlg
//----------------------------------------------------------------------------

// WDR: event table for GenGridDlg

BEGIN_EVENT_TABLE(GenGridDlg,AutoDialog)
	EVT_TEXT( ID_SPACINGX, GenGridDlg::OnSpacingXY )
	EVT_TEXT( ID_SPACINGY, GenGridDlg::OnSpacingXY )
	EVT_TEXT( ID_SIZEY, GenGridDlg::OnSizeXY )
	EVT_TEXT( ID_SIZEY, GenGridDlg::OnSizeXY )
END_EVENT_TABLE()

GenGridDlg::GenGridDlg( wxWindow *parent, wxWindowID id, const wxString &title,
	bool bIsGeo, const wxPoint &position, const wxSize& size, long style ) :
	AutoDialog( parent, id, title, position, size, style )
{
	// WDR: dialog function GenGridDialogFunc for GenGridDlg
	GenGridDialogFunc(this, TRUE);

	m_bSetting = false;

	// sampling
	int digits;
	if (bIsGeo)
		digits = 9;
	else
		digits = 3;
	AddNumValidator(ID_SPACINGX, &m_fSpacingX, digits);
	AddNumValidator(ID_SPACINGY, &m_fSpacingY, digits);
	AddNumValidator(ID_SIZEX, &m_iSizeX);
	AddNumValidator(ID_SIZEY, &m_iSizeY);
	AddNumValidator(ID_TEXT_DIST_CUTOFF, &m_fDistanceCutoff);
}

void GenGridDlg::RecomputeSize()
{
	m_fSpacingX = m_fAreaX / m_iSizeX;
	m_fSpacingY = m_fAreaY / m_iSizeY;
}


// WDR: handler implementations for GenGridDlg

void GenGridDlg::OnSizeXY( wxCommandEvent &event )
{
	if (m_bSetting)
		return;

	TransferDataFromWindow();
	RecomputeSize();

	m_bSetting = true;
	TransferDataToWindow();
	m_bSetting = false;
}

void GenGridDlg::OnSpacingXY( wxCommandEvent &event )
{
	if (m_bSetting)
		return;

	TransferDataFromWindow();
	m_iSizeX = (int) (m_fAreaX / m_fSpacingX);
	m_iSizeY = (int) (m_fAreaY / m_fSpacingY);

	m_bSetting = true;
	GetSizeX()->GetValidator()->TransferToWindow();
	GetSizeY()->GetValidator()->TransferToWindow();
	m_bSetting = false;
}


