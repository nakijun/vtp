//
// Name: NodeDlg.h
//
// Copyright (c) 2002-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifndef __NodeDlg_H__
#define __NodeDlg_H__

#include "VTBuilder_wdr.h"
#include "vtui/AutoDialog.h"
#include "RoadMapEdit.h"
#include "ScaledView.h"

// WDR: class declarations

class NodeDlgView : public vtScaledView
{
public:
	NodeDlgView(wxWindow *parent, wxWindowID id = -1, const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = wxHSCROLL | wxVSCROLL, const wxString& name = _T("")) :
	vtScaledView(parent, id, pos, size, style, name) {}

	NodeEdit *m_pNode;

	void OnDraw(wxDC &dc);
};

//----------------------------------------------------------------------------
// NodeDlg
//----------------------------------------------------------------------------

class NodeDlg: public AutoDialog
{
public:
	// constructors and destructors
	NodeDlg( wxWindow *parent, wxWindowID id, const wxString &title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_DIALOG_STYLE );

	void SetScale(float fScale);
	void SetNode(NodeEdit *pSingleLink, vtRoadLayer *pLayer);
	void ApplyVisualToNode(NodeEdit *pNode, VisualIntersectionType vitype);

	// WDR: method declarations for NodeDlg
	wxListBox* GetBehavior()  { return (wxListBox*) FindWindow( ID_BEHAVIOR ); }
	wxListBox* GetLinkNum()  { return (wxListBox*) FindWindow( ID_ROADNUM ); }
	wxListBox* GetIntType()  { return (wxListBox*) FindWindow( ID_INTTYPE ); }

private:
	// WDR: member variable declarations for NodeDlg
	NodeEdit *m_pNode;
	vtRoadLayer *m_pLayer;
	NodeDlgView *m_pView;

private:
	// WDR: handler declarations for NodeDlg
	void OnBehavior( wxCommandEvent &event );
	void OnLinkNum( wxCommandEvent &event );
	void OnIntType( wxCommandEvent &event );
	void OnOK( wxCommandEvent &event );
	void OnInitDialog(wxInitDialogEvent& event);
	void OnDraw(wxDC &dc);

private:
	DECLARE_EVENT_TABLE()
};

#endif	// __NodeDlg_H__

