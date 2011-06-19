///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __CManager_UI__
#define __CManager_UI__

#include <wx/intl.h>

#include <wx/treectrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/listctrl.h>
#include <wx/statbox.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/bmpbuttn.h>

///////////////////////////////////////////////////////////////////////////

#define ID_SCENETREE 1000
#define ID_ENABLED 1001
#define ID_ZOOMTO 1002
#define ID_REFRESH 1003
#define ID_LOG 1004
#define ID_TEXT 1005
#define ID_ITEM 1006
#define ID_TYPECHOICE 1007
#define ID_SUBTYPECHOICE 1008
#define ID_ADDTAG 1009
#define ID_REMOVETAG 1010
#define ID_EDITTAG 1011
#define ID_TAGLIST 1012
#define ID_FILENAME 1013
#define ID_DISTANCE 1014
#define ID_SCALE 1015
#define ID_STATUS 1016
#define ID_TAGNAME 1017
#define ID_TAGTEXT 1018
#define ID_LIGHT 1019
#define ID_AMBIENT 1020
#define ID_DIFFUSE 1021
#define ID_DIRX 1022
#define ID_DIRY 1023
#define ID_DIRZ 1024

///////////////////////////////////////////////////////////////////////////////
/// Class SceneGraphDlgBase
///////////////////////////////////////////////////////////////////////////////
class SceneGraphDlgBase : public wxDialog 
{
	private:
	
	protected:
		wxTreeCtrl* m_scenetree;
		wxCheckBox* m_enabled;
		wxButton* m_zoomto;
		wxButton* m_refresh;
		wxButton* m_log;
	
	public:
		
		SceneGraphDlgBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxTAB_TRAVERSAL );
		~SceneGraphDlgBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class PropPanelBase
///////////////////////////////////////////////////////////////////////////////
class PropPanelBase : public wxPanel 
{
	private:
	
	protected:
		wxStaticText* m_text1;
		wxTextCtrl* m_item;
		wxStaticText* m_text2;
		wxChoice* m_typechoice;
		wxStaticText* m_text3;
		wxChoice* m_subtypechoice;
		wxStaticText* m_text4;
		wxButton* m_addtag;
		wxButton* m_removetag;
		wxButton* m_edittag;
		wxListCtrl* m_taglist;
	
	public:
		
		PropPanelBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL );
		~PropPanelBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class ModelPanelBase
///////////////////////////////////////////////////////////////////////////////
class ModelPanelBase : public wxPanel 
{
	private:
	
	protected:
		wxStaticText* m_text5;
		wxTextCtrl* m_filename;
		wxStaticText* m_text6;
		wxTextCtrl* m_distance;
		wxStaticText* m_text7;
		wxTextCtrl* m_scale;
		wxStaticText* m_text8;
		wxStaticText* m_text9;
		wxTextCtrl* m_status;
	
	public:
		
		ModelPanelBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL );
		~ModelPanelBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class TagDlgBase
///////////////////////////////////////////////////////////////////////////////
class TagDlgBase : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_text10;
		wxChoice* m_tagname;
		wxStaticText* m_text11;
		wxTextCtrl* m_tagtext;
		wxButton* wxID_OK;
		wxButton* wxID_CANCEL;
	
	public:
		
		TagDlgBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxTAB_TRAVERSAL );
		~TagDlgBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class LightDlgBase
///////////////////////////////////////////////////////////////////////////////
class LightDlgBase : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_text12;
		wxChoice* m_light;
		wxStaticText* m_text13;
		wxBitmapButton* m_ambient;
		wxStaticText* m_text14;
		wxBitmapButton* m_diffuse;
		wxStaticText* m_text15;
		wxTextCtrl* m_dirx;
		wxTextCtrl* m_diry;
		wxTextCtrl* m_dirz;
	
	public:
		
		LightDlgBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxTAB_TRAVERSAL );
		~LightDlgBase();
	
};

#endif //__CManager_UI__
