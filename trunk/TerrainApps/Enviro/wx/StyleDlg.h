//
// Name: StyleDlg.h
//
// Copyright (c) 2004-2006 Virtual Terrain Project.
// Free for all uses, see license.txt for details.
//

#ifndef __StyleDlg_H__
#define __StyleDlg_H__

#include "enviro_wdr.h"
#include "vtui/AutoDialog.h"
#include "vtdata/Features.h"

class vtTagArray;

// WDR: class declarations

//----------------------------------------------------------------------------
// StyleDlg
//----------------------------------------------------------------------------

class StyleDlg: public AutoDialog
{
public:
	// constructors and destructors
	StyleDlg( wxWindow *parent, wxWindowID id, const wxString &title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_DIALOG_STYLE );
	
	// This dialog can operate on a featureset in memory.  If you do not give
	//  it a featureset, it will assume the layer's filename is a SHP on disk
	//  and use that instead.
	void SetFeatureSet(const vtFeatureSet *pSet)
	{
		m_pFeatureSet = pSet;
	}

	void SetOptions(const vtStringArray &datapaths, const vtTagArray &Layer);
	void GetOptions(vtTagArray &pLayer);

private:
	// WDR: method declarations for StyleDlg
	wxChoice* GetTextureMode()  { return (wxChoice*) FindWindow( ID_TEXTURE_MODE ); }
	wxCheckBox* GetTextureOverlay()  { return (wxCheckBox*) FindWindow( ID_TEXTURE_OVERLAY ); }
	wxTextCtrl* GetFont()  { return (wxTextCtrl*) FindWindow( ID_FONT ); }
	wxTextCtrl* GetLineWidth()  { return (wxTextCtrl*) FindWindow( ID_LINE_WIDTH ); }
	wxTextCtrl* GetGeomHeight()  { return (wxTextCtrl*) FindWindow( ID_GEOM_HEIGHT ); }
	wxCheckBox* GetTessellate()  { return (wxCheckBox*) FindWindow( ID_TESSELLATE ); }
	wxTextCtrl* GetLabelSize()  { return (wxTextCtrl*) FindWindow( ID_LABEL_SIZE ); }
	wxTextCtrl* GetLabelHeight()  { return (wxTextCtrl*) FindWindow( ID_LABEL_HEIGHT ); }
	wxChoice* GetColorField()  { return (wxChoice*) FindWindow( ID_COLOR_FIELD ); }
	wxChoice* GetTextField()  { return (wxChoice*) FindWindow( ID_TEXT_FIELD ); }
	wxBitmapButton* GetLabelColor()  { return (wxBitmapButton*) FindWindow( ID_LABEL_COLOR ); }
	wxCheckBox* GetTextLabels()  { return (wxCheckBox*) FindWindow( ID_TEXT_LABELS ); }
	wxBitmapButton* GetGeomColor()  { return (wxBitmapButton*) FindWindow( ID_GEOM_COLOR ); }
	wxCheckBox* GetGeometry()  { return (wxCheckBox*) FindWindow( ID_GEOMETRY ); }
	wxTextCtrl* GetFeatureType()  { return (wxTextCtrl*) FindWindow( ID_FEATURE_TYPE ); }

private:
	// WDR: member variable declarations for StyleDlg
	const vtFeatureSet *m_pFeatureSet;
	vtString m_strResolved;

	OGRwkbGeometryType m_type;
	wxString m_strFeatureType;

	bool m_bGeometry;
	RGBi m_GeomColor;
	float m_fGeomHeight;
	float m_fLineWidth;
	bool m_bTessellate;

	bool m_bTextLabels;
	RGBi m_LabelColor;
	int m_iTextField;
	int m_iColorField;
	float m_fLabelHeight;
	float m_fLabelSize;
	wxString m_strFont;

	bool m_bTextureOverlay;
	wxString m_strTextureMode;

	vtFeatureSetPoint2D m_DummyFeatures;
	wxColourData m_ColourData;
	wxColour m_Colour;

private:
	void RefreshFields();
	void UpdateEnabling();
	void UpdateColorButtons();
	RGBi AskColor(const RGBi &input);

	// WDR: handler declarations for StyleDlg
	void OnCheck( wxCommandEvent &event );
	void OnLabelColor( wxCommandEvent &event );
	void OnGeomColor( wxCommandEvent &event );
	void OnInitDialog(wxInitDialogEvent& event);

private:
	DECLARE_EVENT_TABLE()
};

#endif  // __StyleDlg_H__
