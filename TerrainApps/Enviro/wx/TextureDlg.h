//
// Name: TextureDlg.h
//
// Copyright (c) 2006-2007 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifndef __TextureDlg_H__
#define __TextureDlg_H__

#include "enviro_wdr.h"
#include "vtui/AutoDialog.h"
#include "vtlib/core/TParams.h"

// WDR: class declarations

//----------------------------------------------------------------------------
// TextureDlg
//----------------------------------------------------------------------------

class TextureDlg: public AutoDialog
{
public:
	// constructors and destructors
	TextureDlg( wxWindow *parent, wxWindowID id, const wxString &title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_DIALOG_STYLE );

	void SetParams(const TParams &Params);
	void GetParams(TParams &Params);
	void UpdateFilenameBases();
	void UpdateTiledTextureFilename();
	void UpdateEnableState();
	void UpdateColorMapChoice();

	// WDR: method declarations for TextureDlg
	wxRadioButton* GetSingle()  { return (wxRadioButton*) FindWindow( ID_SINGLE ); }
	wxRadioButton* GetDerived()  { return (wxRadioButton*) FindWindow( ID_DERIVED ); }
	wxRadioButton* GetTiled4by4()  { return (wxRadioButton*) FindWindow( ID_TILED_4BY4 ); }
	wxChoice* GetColorMap()  { return (wxChoice*) FindWindow( ID_CHOICE_COLORS ); }
	wxTextCtrl* GetTfilename()  { return (wxTextCtrl*) FindWindow( ID_TFILENAME ); }
	wxChoice* GetTilesize()  { return (wxChoice*) FindWindow( ID_CHOICE_TILESIZE ); }
	wxChoice* GetTfileBase()  { return (wxChoice*) FindWindow( ID_TFILE_BASE ); }
	wxChoice* GetChoiceTilesize()  { return (wxChoice*) FindWindow( ID_CHOICE_TILESIZE ); }
	wxButton* GetEditColors()  { return (wxButton*) FindWindow( ID_EDIT_COLORS ); }
	wxChoice* GetChoiceColors()  { return (wxChoice*) FindWindow( ID_CHOICE_COLORS ); }
	wxComboBox* GetTfileSingle()  { return (wxComboBox*) FindWindow( ID_TFILE_SINGLE ); }

private:
	// WDR: member variable declarations for TextureDlg

	// texture
	int	 m_iTexture;
	int	 m_iTilesize;
	wxString	m_strTextureSingle;
	wxString	m_strTextureBase;
	wxString	m_strTexture4x4;
	wxString	m_strTextureFilename;
	wxString	m_strTextureTileset;
	wxString	m_strColorMap;

	bool	m_bSetting;
	vtStringArray m_TextureFiles;
	int		m_iTilesizeIndex;

	wxRadioButton* m_pSingle;
	wxRadioButton* m_pDerived;
	wxRadioButton* m_pTiled;
	wxChoice* m_pColorMap;
	wxComboBox* m_pTextureFileSingle;

private:
	// WDR: handler declarations for TextureDlg
	void OnEditColors( wxCommandEvent &event );
	void OnComboTFileSingle( wxCommandEvent &event );
	void OnRadio( wxCommandEvent &event );
	void OnTextureFileBase( wxCommandEvent &event );
	void OnTileSize( wxCommandEvent &event );
	void OnInitDialog(wxInitDialogEvent& event);

	bool TransferDataToWindow();
	bool TransferDataFromWindow();

private:
	DECLARE_EVENT_TABLE()
};

#endif	// __TextureDlg_H__

