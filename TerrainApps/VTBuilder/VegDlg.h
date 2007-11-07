//
// VegDlg.h
//
// Copyright (c) 2001-2004 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifndef VEGDLGH
#define VEGDLGH

#include "VTBuilder_wdr.h"
#include "wx/splitter.h"
#include "wx/treectrl.h"
#include "wx/listctrl.h"
#include "wx/dialog.h"
#include "vtui/AutoDialog.h"

// WDR: class declarations

//----------------------------------------------------------------------------
// SpeciesListDlg
//----------------------------------------------------------------------------

class SpeciesListDlg: public wxDialog
{
public:
	// constructors and destructors
	SpeciesListDlg( wxWindow *parent, wxWindowID id, const wxString &title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_DIALOG_STYLE );

protected:
	// WDR: method declarations for SpeciesListDlg
	void RefreshAppeances();
	void AddAppeance(int idx);

private:
	// WDR: member variable declarations for SpeciesListDlg
	wxSplitterWindow *m_PSplitter;
	wxListCtrl *m_PSTable;
	wxListCtrl *m_PATable;

	int m_idx;

private:
	// WDR: handler declarations for SpeciesListDlg
	void OnSelect( wxListEvent &event );
	void OnOK( wxCommandEvent &event );
	void OnInitDialog(wxInitDialogEvent& event);

private:
	DECLARE_EVENT_TABLE()
};


class BioRegionDlg : public AutoDialog
{
public:
	BioRegionDlg(wxWindow *parent, wxWindowID id, const wxString& title,
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_DIALOG_STYLE)
		: AutoDialog(parent, id, title, pos, size, style)
	{
		RefreshContents();
	}

	void RefreshContents();

	wxTreeCtrl *m_BTree;
};

#endif
