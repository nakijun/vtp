//
// Name: UtilDlg.cpp
//
// Copyright (c) 2002-2006 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifdef __GNUG__
	#pragma implementation "UtilDlg.cpp"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#include "vtlib/vtlib.h"
#include "vtlib/core/Route.h"
#include "UtilDlg.h"
#include "EnviroGUI.h"


// WDR: class implementations

//----------------------------------------------------------------------------
// UtilDlg
//----------------------------------------------------------------------------

// WDR: event table for UtilDlg

BEGIN_EVENT_TABLE(UtilDlg,AutoDialog)
	EVT_INIT_DIALOG (UtilDlg::OnInitDialog)
	EVT_CHOICE( ID_STRUCTTYPE, UtilDlg::OnStructType )
END_EVENT_TABLE()

UtilDlg::UtilDlg( wxWindow *parent, wxWindowID id, const wxString &title,
	const wxPoint &position, const wxSize& size, long style ) :
	AutoDialog( parent, id, title, position, size, style )
{
	UtilDialogFunc( this, TRUE );

	m_pChoice = GetStructtype();
	m_iType = 0;

	AddValidator(ID_STRUCTTYPE, &m_iType);
}

// WDR: handler implementations for UtilDlg

void UtilDlg::OnStructType( wxCommandEvent &event )
{
	TransferDataFromWindow();
	wxString val = m_pChoice->GetStringSelection();
	g_App.SetRouteOptions((const char *) val.mb_str(wxConvUTF8));
	g_App.start_new_fence();
}

void UtilDlg::OnInitDialog(wxInitDialogEvent& event)
{
	vtContentManager &mng = vtGetContent();

	m_pChoice->Clear();
	for (unsigned int i = 0; i < mng.NumItems(); i++)
	{
		vtString str;
		vtItem *item = mng.GetItem(i);
		if (item->GetValueString("type", str))
		{
			if (str == "utility pole")
				m_pChoice->Append(wxString::FromAscii(item->m_name));
		}
	}
	TransferDataToWindow();

	wxString val = m_pChoice->GetStringSelection();
	g_App.SetRouteOptions((const char *) val.mb_str(wxConvUTF8));
}
