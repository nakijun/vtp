//
// Name: OptionsDlg.cpp
//
// Copyright (c) 2002-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#include "OptionsDlg.h"
#include "RenderOptionsDlg.h"

// WDR: class implementations

//----------------------------------------------------------------------------
// OptionsDlg
//----------------------------------------------------------------------------

// WDR: event table for OptionsDlg

BEGIN_EVENT_TABLE(OptionsDlg, AutoDialog)
	EVT_INIT_DIALOG (OptionsDlg::OnInitDialog)
	EVT_RADIOBUTTON( ID_RADIO_OUTLINE_ONLY, OptionsDlg::OnRadio )
	EVT_RADIOBUTTON( ID_RADIO_COLOR, OptionsDlg::OnRadio )
	EVT_BUTTON( ID_RENDER_OPTIONS, OptionsDlg::OnRenderOptions )
	EVT_CHECKBOX( ID_CHECK_HIDE_UNKNOWN, OptionsDlg::OnHideUnknown )
END_EVENT_TABLE()

OptionsDlg::OptionsDlg( wxWindow *parent, wxWindowID id, const wxString &title,
	const wxPoint &position, const wxSize& size, long style ) :
	AutoDialog( parent, id, title, position, size, style )
{
	OptionsDialogFunc( this, TRUE );

	GetElevUnit()->Append(_("Meters"));
	GetElevUnit()->Append(_("Feet (International)"));
	GetElevUnit()->Append(_("Feet (U.S. Survey)"));

	AddValidator(this, ID_MINUTES, &m_bShowMinutes);
	AddValidator(this, ID_ELEVUNIT, &m_iElevUnits);

	AddValidator(this, ID_RADIO_OUTLINE_ONLY, &m_bShowOutlines);
	AddValidator(this, ID_RADIO_COLOR, &m_opt.m_bShowElevation);
	AddValidator(this, ID_CHECK_HIDE_UNKNOWN, &m_opt.m_bDoMask);
	AddValidator(this, ID_CHECK_DRAW_TIN_SIMPLE, &m_bDrawTinSimple);

	AddValidator(this, ID_CHECK_SHOW_ROAD_WIDTH, &m_bShowRoadWidth);
	AddValidator(this, ID_CHECK_DRAW_RAW_SIMPLE, &m_bDrawRawSimple);
	AddValidator(this, ID_PATHNAMES, &m_bShowPath);
}

// WDR: handler implementations for OptionsDlg

void OptionsDlg::OnHideUnknown( wxCommandEvent &event )
{
}

void OptionsDlg::OnRenderOptions( wxCommandEvent &event )
{
	// Ask them how to render elevation layers
	RenderOptionsDlg dlg(this, -1, _("Rendering options"));
	dlg.SetOptions(m_opt);
	if (dlg.ShowModal() != wxID_OK)
		return;
	m_opt = dlg.m_opt;
}

void OptionsDlg::OnRadio( wxCommandEvent &event )
{
	TransferDataFromWindow();
	UpdateEnables();
}

void OptionsDlg::UpdateEnables()
{
	GetRenderOptions()->Enable(m_opt.m_bShowElevation);
	GetCheckHideUnknown()->Enable(m_opt.m_bShowElevation);
}

void OptionsDlg::OnInitDialog(wxInitDialogEvent& event)
{
	UpdateEnables();
	wxDialog::OnInitDialog(event);
}

void OptionsDlg::SetElevDrawOptions(const ElevDrawOptions &opt)
{
	m_opt = opt;
	m_bShowOutlines = !opt.m_bShowElevation;
}

void OptionsDlg::GetElevDrawOptions(ElevDrawOptions &opt)
{
	opt = m_opt;
}

