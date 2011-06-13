///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __vtui_UI__
#define __vtui_UI__

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/listbox.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/statbox.h>
#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/listctrl.h>

///////////////////////////////////////////////////////////////////////////

#define ID_TEXT 1000
#define ID_LEVEL 1001
#define ID_LEVEL_COPY 1002
#define ID_LEVEL_DEL 1003
#define ID_LEVEL_UP 1004
#define ID_LEVEL_DOWN 1005
#define ID_LINE1 1006
#define ID_EDITHEIGHTS 1007
#define ID_SET_ROOF_TYPE 1008
#define ID_STORIES 1009
#define ID_STORY_HEIGHT 1010
#define ID_MATERIAL1 1011
#define ID_SET_MATERIAL 1012
#define ID_COLOR1 1013
#define ID_SET_COLOR 1014
#define ID_EDGE_SLOPES 1015
#define ID_SET_EDGE_SLOPES 1016
#define ID_EDGES 1017
#define ID_EDGE 1018
#define ID_MATERIAL2 1019
#define ID_FACADE 1020
#define ID_COLOR2 1021
#define ID_EDGE_SLOPE 1022
#define ID_FEATURES 1023
#define ID_FEAT_CLEAR 1024
#define ID_FEAT_WALL 1025
#define ID_FEAT_WINDOW 1026
#define ID_FEAT_DOOR 1027
#define ID_LINEAR_STRUCTURE_STYLE 1028
#define ID_POST_TYPE 1029
#define ID_POST_SPACING_EDIT 1030
#define ID_POST_SPACING_SLIDER 1031
#define ID_POST_HEIGHT_EDIT 1032
#define ID_POST_HEIGHT_SLIDER 1033
#define ID_POST_SIZE_EDIT 1034
#define ID_POST_SIZE_SLIDER 1035
#define ID_CHOICE_EXTENSION 1036
#define ID_CONN_TYPE 1037
#define ID_CONN_MATERIAL 1038
#define ID_CONN_TOP_EDIT 1039
#define ID_CONN_TOP_SLIDER 1040
#define ID_CONN_BOTTOM_EDIT 1041
#define ID_CONN_BOTTOM_SLIDER 1042
#define ID_CONN_WIDTH_EDIT 1043
#define ID_CONN_WIDTH_SLIDER 1044
#define ID_SLOPE 1045
#define ID_SLOPE_SLIDER 1046
#define ID_CHOICE_PROFILE 1047
#define ID_PROFILE_EDIT 1048
#define ID_CONSTANT_TOP 1049
#define ID_RADIO_CONTENT 1050
#define ID_CHOICE_FILE 1051
#define ID_CHOICE_TYPE 1052
#define ID_CHOICE_ITEM 1053
#define ID_RADIO_MODEL 1054
#define ID_MODEL_FILE 1055
#define ID_BROWSE_MODEL_FILE 1056
#define ID_LOCATION 1057
#define ID_RADIO_LINE 1058
#define ID_RADIO_PATH 1059
#define ID_DIST_LOAD_PATH 1060
#define ID_DIST_TOOL_CLEAR 1061
#define ID_MAP_OFFSET 1062
#define ID_UNITS1 1063
#define ID_MAP_DIST 1064
#define ID_UNITS2 1065
#define ID_GEOD_DIST 1066
#define ID_UNITS3 1067
#define ID_GROUND_DIST 1068
#define ID_UNITS4 1069
#define ID_VERTICAL 1070
#define ID_UNITS5 1071
#define ID_CMAP_FILE 1072
#define ID_RELATIVE 1073
#define ID_BLEND 1074
#define ID_COLORLIST 1075
#define ID_CHANGE_COLOR 1076
#define ID_DELETE_ELEVATION 1077
#define ID_HEIGHT_TO_ADD 1078
#define ID_ADD 1079
#define ID_SAVE_CMAP 1080
#define ID_SAVE_AS_CMAP 1081
#define ID_LOAD_CMAP 1082
#define ID_LINE_OF_SIGHT 1083
#define ID_VISIBILITY 1084
#define ID_FRESNEL 1085
#define ID_USE_EFFECTIVE 1086
#define ID_SHOW_CULTURE 1087
#define ID_LINE2 1088
#define ID_HEIGHT1 1089
#define ID_HEIGHT2 1090
#define ID_RF 1091
#define ID_CURVATURE 1092
#define ID_STATUS_TEXT 1093
#define ID_EXPORT_DXF 1094
#define ID_EXPORT_TRACE 1095
#define ID_EXPORT_CSV 1096
#define ID_PROJ 1097
#define ID_SHOW_ALL_DATUMS 1098
#define ID_DATUM 1099
#define ID_ELLIPSOID 1100
#define ID_HORUNITS 1101
#define ID_ZONE 1102
#define ID_PROJPARAM 1103
#define ID_STATEPLANE 1104
#define ID_SET_EPSG 1105
#define ID_PROJ_LOAD 1106
#define ID_PROJ_SAVE 1107
#define ID_STATEPLANES 1108
#define ID_NAD27 1109
#define ID_NAD83 1110
#define ID_RADIO_METERS 1111
#define ID_RADIO_FEET 1112
#define ID_RADIO_FEET_US 1113
#define ID_ADD_POINT 1114
#define ID_MOVE_POINT 1115
#define ID_REMOVE_POINT 1116
#define ID_SAVE_PROF 1117
#define ID_SAVE_AS_PROF 1118
#define ID_LOAD_PROF 1119
#define ID_TAGLIST 1120
#define ID_TEXT_SHOW 1121
#define ID_CHOICE_SHOW 1122
#define ID_TEXT_VERTICAL 1123
#define ID_CHOICE_VERTICAL 1124
#define ID_DEL_HIGH 1125
#define ID_LIST 1126
#define ID_TEXTCTRL 1127
#define ID_SLIDER_RATIO 1128
#define ID_TEXT_X 1129
#define ID_TEXT_Y 1130

///////////////////////////////////////////////////////////////////////////////
/// Class BuildingDlgBase
///////////////////////////////////////////////////////////////////////////////
class BuildingDlgBase : public wxDialog 
{
	private:
	
	protected:
		wxPanel* m_panel1;
		wxStaticText* m_text1;
		wxListBox* m_level;
		wxButton* m_level_copy;
		wxButton* m_level_del;
		wxButton* m_level_up;
		wxButton* m_level_down;
		wxStaticLine* m_line1;
		wxButton* m_editheights;
		wxButton* m_set_roof_type;
		wxStaticText* m_text2;
		wxSpinCtrl* m_stories;
		wxStaticText* m_text3;
		wxTextCtrl* m_story_height;
		wxStaticText* m_text4;
		wxStaticText* m_text5;
		wxTextCtrl* m_material1;
		wxButton* m_set_material;
		wxStaticText* m_text6;
		wxStaticBitmap* m_color1;
		wxButton* m_set_color;
		wxStaticText* m_text7;
		wxTextCtrl* m_edge_slopes;
		wxButton* m_set_edge_slopes;
		
		wxButton* m_ok1;
		
		wxButton* m_edges;
		wxPanel* m_panel2;
		wxStaticText* m_text8;
		wxListBox* m_level1;
		wxButton* m_level_copy1;
		wxButton* m_level_del1;
		wxButton* m_level_up1;
		wxButton* m_level_down1;
		wxStaticLine* m_line11;
		wxButton* m_editheights1;
		wxButton* m_set_roof_type1;
		wxStaticText* m_text9;
		wxSpinCtrl* m_stories1;
		wxStaticText* m_text10;
		wxTextCtrl* m_story_height1;
		wxStaticText* m_text11;
		wxStaticText* m_text12;
		wxListBox* m_edge;
		wxStaticText* m_text13;
		wxTextCtrl* m_material2;
		wxButton* m_set_material1;
		wxStaticText* m_text14;
		wxChoice* m_facade;
		wxStaticText* m_text15;
		wxStaticBitmap* m_color2;
		wxButton* m_set_color1;
		wxStaticText* m_text16;
		wxTextCtrl* m_edge_slope;
		wxTextCtrl* m_features;
		wxButton* m_feat_clear;
		wxButton* m_feat_wall;
		wxButton* m_feat_window;
		wxButton* m_feat_door;
		
		wxButton* m_ok2;
		
		wxButton* m_edges1;
	
	public:
		
		BuildingDlgBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxTAB_TRAVERSAL );
		~BuildingDlgBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class LinearStructDlgBase
///////////////////////////////////////////////////////////////////////////////
class LinearStructDlgBase : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_text17;
		wxChoice* m_linear_structure_style;
		wxStaticText* m_text18;
		wxChoice* m_post_type;
		wxStaticText* m_text19;
		wxTextCtrl* m_post_spacing_edit;
		wxSlider* m_post_spacing_slider;
		wxStaticText* m_text20;
		wxTextCtrl* m_post_height_edit;
		wxSlider* m_post_height_slider;
		wxStaticText* m_text21;
		wxTextCtrl* m_post_size_edit;
		wxSlider* m_post_size_slider;
		wxStaticText* m_text22;
		wxChoice* m_choice_extension;
		wxStaticText* m_text23;
		wxChoice* m_conn_type;
		wxStaticText* m_text24;
		wxChoice* m_conn_material;
		wxStaticText* m_text25;
		wxTextCtrl* m_conn_top_edit;
		wxSlider* m_conn_top_slider;
		wxStaticText* m_text26;
		wxTextCtrl* m_conn_bottom_edit;
		wxSlider* m_conn_bottom_slider;
		wxStaticText* m_text27;
		wxTextCtrl* m_conn_width_edit;
		wxSlider* m_conn_width_slider;
		wxStaticText* m_text28;
		wxTextCtrl* m_slope;
		wxSlider* m_slope_slider;
		wxStaticText* m_text29;
		wxChoice* m_choice_profile;
		wxButton* m_profile_edit;
		wxCheckBox* m_constant_top;
	
	public:
		
		LinearStructDlgBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxTAB_TRAVERSAL );
		~LinearStructDlgBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class InstanceDlgBase
///////////////////////////////////////////////////////////////////////////////
class InstanceDlgBase : public wxDialog 
{
	private:
	
	protected:
		wxRadioButton* m_radio_content;
		
		wxStaticText* m_text30;
		wxChoice* m_choice_file;
		
		wxStaticText* m_text31;
		wxChoice* m_choice_type;
		
		wxStaticText* m_text32;
		wxChoice* m_choice_item;
		wxRadioButton* m_radio_model;
		
		wxTextCtrl* m_model_file;
		wxButton* m_browse_model_file;
		wxStaticText* m_text33;
		wxTextCtrl* m_location;
	
	public:
		
		InstanceDlgBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxTAB_TRAVERSAL );
		~InstanceDlgBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class DistanceDlgBase
///////////////////////////////////////////////////////////////////////////////
class DistanceDlgBase : public wxDialog 
{
	private:
	
	protected:
		
		wxRadioButton* m_radio_line;
		wxRadioButton* m_radio_path;
		
		wxButton* m_dist_load_path;
		wxButton* m_dist_tool_clear;
		wxStaticText* m_text34;
		wxTextCtrl* m_map_offset;
		wxChoice* m_units1;
		wxStaticText* id_text;
		wxTextCtrl* m_map_dist;
		wxChoice* m_units2;
		wxStaticText* m_text35;
		wxTextCtrl* mmeod_dist;
		wxChoice* m_units3;
		wxStaticText* m_text36;
		wxTextCtrl* m_ground_dist;
		wxChoice* m_units4;
		wxStaticText* m_text37;
		wxTextCtrl* m_vertical;
		wxChoice* id_units5;
	
	public:
		
		DistanceDlgBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxTAB_TRAVERSAL );
		~DistanceDlgBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class ColorMapDlgBase
///////////////////////////////////////////////////////////////////////////////
class ColorMapDlgBase : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_text38;
		wxTextCtrl* m_cmap_file;
		wxCheckBox* m_relative;
		wxCheckBox* mmlend;
		wxListCtrl* id_colorlist;
		wxButton* m_change_color;
		wxButton* m_delete_elevation;
		wxStaticText* m_text39;
		wxTextCtrl* mmeight_to_add;
		wxButton* m_add;
		wxButton* m_save_cmap;
		wxButton* id_save_as_cmap;
		wxButton* m_load_cmap;
		wxStaticLine* m_line1;
		wxButton* m_close;
	
	public:
		
		ColorMapDlgBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxTAB_TRAVERSAL );
		~ColorMapDlgBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class ProfileDlgBase
///////////////////////////////////////////////////////////////////////////////
class ProfileDlgBase : public wxDialog 
{
	private:
	
	protected:
		
		wxCheckBox* m_line_of_sight;
		wxCheckBox* m_visibility;
		wxCheckBox* m_fresnel;
		wxCheckBox* m_use_effective;
		wxButton* m_show_culture;
		wxStaticLine* mmine2;
		wxStaticText* m_text40;
		wxTextCtrl* m_height1;
		wxStaticText* id_text;
		wxTextCtrl* m_height2;
		wxStaticText* m_text41;
		wxTextCtrl* m_rf;
		wxStaticText* m_text42;
		wxStaticText* m_text43;
		wxChoice* m_curvature;
		wxStaticLine* m_line2;
		wxTextCtrl* m_status_text;
		wxStaticText* m_text44;
		wxButton* m_export_dxf;
		wxButton* m_export_trace;
		wxButton* m_export_csv;
	
	public:
		
		ProfileDlgBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxTAB_TRAVERSAL );
		~ProfileDlgBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class ProjectionDlgBase
///////////////////////////////////////////////////////////////////////////////
class ProjectionDlgBase : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_text45;
		wxChoice* m_proj;
		wxStaticText* mmext;
		wxCheckBox* m_show_all_datums;
		wxChoice* m_datum;
		wxStaticText* id_text;
		wxTextCtrl* m_ellipsoid;
		wxStaticText* m_text46;
		wxChoice* m_horunits;
		wxStaticText* m_text47;
		wxChoice* m_zone;
		wxStaticText* m_text48;
		wxListCtrl* m_projparam;
		wxButton* mmtateplane;
		wxButton* m_set_epsg;
		wxButton* m_proj_load;
		wxButton* m_proj_save;
		wxStaticLine* m_line2;
		wxButton* m_ok;
		wxButton* m_cancel;
	
	public:
		
		ProjectionDlgBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxTAB_TRAVERSAL );
		~ProjectionDlgBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class StatePlaneDlgBase
///////////////////////////////////////////////////////////////////////////////
class StatePlaneDlgBase : public wxDialog 
{
	private:
	
	protected:
		wxListBox* id_stateplanes;
		wxRadioButton* m_nad27;
		wxRadioButton* m_nad83;
		wxRadioButton* m_radio_meters;
		wxRadioButton* m_radio_feet;
		wxRadioButton* m_radio_feet_us;
		wxButton* wxID_OK;
		wxButton* wxID_CANCEL;
	
	public:
		
		StatePlaneDlgBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxTAB_TRAVERSAL );
		~StatePlaneDlgBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class ProfileEditDlgBase
///////////////////////////////////////////////////////////////////////////////
class ProfileEditDlgBase : public wxDialog 
{
	private:
	
	protected:
		wxRadioButton* m_add_point;
		wxRadioButton* m_move_point;
		wxRadioButton* m_remove_point;
		wxStaticLine* m_line2;
		wxButton* m_save_prof;
		wxButton* m_save_as_prof;
		wxButton* m_load_prof;
		wxButton* m_close;
	
	public:
		wxBoxSizer* viewsizer;
		
		ProfileEditDlgBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxTAB_TRAVERSAL );
		~ProfileEditDlgBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class TagDlgBase
///////////////////////////////////////////////////////////////////////////////
class TagDlgBase : public wxDialog 
{
	private:
	
	protected:
		wxListCtrl* m_taglist;
	
	public:
		
		TagDlgBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 231,147 ), long style = wxDEFAULT_DIALOG_STYLE|wxTAB_TRAVERSAL );
		~TagDlgBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class FeatureTableDlgBase
///////////////////////////////////////////////////////////////////////////////
class FeatureTableDlgBase : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_text49_show;
		wxChoice* m_choice_show;
		wxStaticText* mmext_vertical;
		wxChoice* m_choice_vertical;
		wxButton* m_del_high;
		wxListCtrl* m_list;
	
	public:
		
		FeatureTableDlgBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxTAB_TRAVERSAL );
		~FeatureTableDlgBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SizeDlgBase
///////////////////////////////////////////////////////////////////////////////
class SizeDlgBase : public wxDialog 
{
	private:
	
	protected:
		wxTextCtrl* m_textctrl;
		wxSlider* m_slider_ratio;
		wxTextCtrl* m_text_x;
		wxTextCtrl* m_text_y;
		
		wxButton* m_ok;
		wxButton* m_cancel;
	
	public:
		
		SizeDlgBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxTAB_TRAVERSAL );
		~SizeDlgBase();
	
};

#endif //__vtui_UI__
