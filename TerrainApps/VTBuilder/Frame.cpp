//
// The main Frame window of the VTBuilder application
//
// Copyright (c) 2001-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/stdpaths.h>

#include "vtdata/ElevationGrid.h"
#include "vtdata/FilePath.h"
#include "vtdata/vtDIB.h"
#include "vtdata/vtLog.h"
#include "vtdata/DataPath.h"
#include "xmlhelper/exception.hpp"
#include <fstream>
#include <float.h>	// for FLT_MIN

#include "Frame.h"
#include "TreeView.h"
#include "MenuEnum.h"
#include "App.h"
#include "Helper.h"
#include "BuilderView.h"
#include "VegGenOptions.h"
#include "vtImage.h"
#include "Options.h"

#include "vtui/Helper.h"
#include "vtui/ProfileDlg.h"

// Layers
#include "ElevLayer.h"
#include "ImageLayer.h"
#include "RawLayer.h"
#include "RoadLayer.h"
#include "StructLayer.h"
#include "UtilityLayer.h"
#include "VegLayer.h"
// Dialogs
#include "DistanceDlg2d.h"
#include "FeatInfoDlg.h"
#include "OptionsDlg.h"
#include "ResampleDlg.h"
#include "SampleImageDlg.h"
#include "vtui/InstanceDlg.h"
#include "vtui/LinearStructDlg.h"
#include "vtui/ProjectionDlg.h"

#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMAC__)
#  include "bld_add_points.xpm"
#  include "bld_delete_points.xpm"
#  include "bld_edit.xpm"
#  include "bld_corner.xpm"
#  include "distance.xpm"
#  include "edit_crossing.xpm"
#  include "edit_delete.xpm"
#  include "edit_offset.xpm"
#  include "elev_box.xpm"
#  include "elev_resample.xpm"

#  include "image_resample.xpm"
#  include "info.xpm"
#  include "instances.xpm"

#  include "layer_export.xpm"
#  include "layer_import.xpm"
#  include "layer_new.xpm"
#  include "layer_open.xpm"
#  include "layer_save.xpm"
#  include "layer_show.xpm"
#  include "layer_up.xpm"

#  include "proj_new.xpm"
#  include "proj_open.xpm"
#  include "proj_save.xpm"

#  include "rd_direction.xpm"
#  include "rd_edit.xpm"
#  include "rd_select_node.xpm"
#  include "rd_select_road.xpm"
#  include "rd_select_whole.xpm"
#  include "rd_shownodes.xpm"

#  include "select.xpm"
#  include "str_add_linear.xpm"
#  include "str_edit_linear.xpm"
#  include "raw_add_point.xpm"
#  include "tin_trim.xpm"

#  include "table.xpm"
#  include "twr_edit.xpm"

#  include "view_hand.xpm"
#  include "view_mag.xpm"
#  include "view_minus.xpm"
#  include "view_plus.xpm"
#  include "view_zoomall.xpm"
#  include "view_zoomexact.xpm"
#  include "view_zoom_layer.xpm"
#  include "view_profile.xpm"
#  include "view_options.xpm"

#	include "VTBuilder.xpm"
#endif

// Singletons
DECLARE_APP(BuilderApp)

////////////////////////////////////////////////////////////////

#if 0
class vtScaleBar : public wxWindow
{
public:
	vtScaleBar(wxWindow *parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = 0, const wxString& name = wxPanelNameStr);

	void OnPaint(wxPaintEvent& event);

protected:
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(vtScaleBar, wxWindow)
EVT_PAINT(OnPaint)
END_EVENT_TABLE()

vtScaleBar::vtScaleBar(wxWindow *parent, wxWindowID id, const wxPoint& pos,
	const wxSize& size, long style, const wxString& name) :
  wxWindow(parent, id, pos, size, style, name)
{
	VTLOG1("vtScaleBar constructor\n");
}

void vtScaleBar::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxCoord w, h;
	GetClientSize(&w,&h);
	dc.DrawRoundedRectangle(0, 0, w, h, 5);
}
#endif

//////////////////////////////////////////////////////////////////

/** You can get at the main frame object from anywhere in the application. */
MainFrame *GetMainFrame()
{
	return (MainFrame *) wxGetApp().GetTopWindow();
}


//////////////////////////////////////////////////////////////////
// Frame constructor
//
MainFrame::MainFrame(wxFrame *frame, const wxString& title,
	const wxPoint& pos, const wxSize& size) :
		wxFrame(frame, wxID_ANY, title, pos, size)
{
	VTLOG("  MainFrame constructor: enter\n");

	// tell wxAuiManager to manage this frame
	m_mgr.SetManagedWindow(this);

	// Inform builder object that this is the top window
	m_pParentWindow = this;

	// init app data
	m_pView = NULL;
	m_pActiveLayer = NULL;
	m_SpeciesListDlg = NULL;
	m_BioRegionDlg = NULL;
	m_pFeatInfoDlg = NULL;
	m_pDistanceDlg = NULL;
	m_pProfileDlg = NULL;
	m_pLinearStructureDlg = NULL;
	m_pInstanceDlg = NULL;
	m_bAdoptFirstCRS = true;
	m_pToolbar = NULL;
	m_pMapServerDlg = NULL;
	for (int i = 0; i < LAYER_TYPES; i++)
		m_pLayBar[i] = NULL;

	// frame icon
	SetIcon(wxICON(vtbuilder));
	VTLOG1("  MainFrame constructor: exit\n");
}

MainFrame::~MainFrame()
{
	VTLOG1("Frame destructor\n");
	WriteXML(APPNAME ".xml");
	DeleteContents();

	m_mgr.UnInit();
}

void MainFrame::CreateView()
{
	VTLOG1("CreateView\n");
	m_pView = new BuilderView(this, wxID_ANY,
			wxPoint(0, 0), wxSize(200, 400), _T("") );

	m_mgr.AddPane(m_pView, wxAuiPaneInfo().
				  Name(wxT("view")).Caption(wxT("View")).
				  CenterPane().Show(true));
	m_mgr.Update();
	VTLOG1(" refreshing view\n");
	m_pView->Refresh();
}

void MainFrame::ZoomAll()
{
	VTLOG("Zoom All\n");
	m_pView->ZoomToRect(GetExtents(), 0.1f);
}

void MainFrame::SetupUI()
{
	m_statbar = new MyStatusBar(this);
	SetStatusBar(m_statbar);
	m_statbar->Show();
	m_statbar->SetTexts(this);
	PositionStatusBar();

	CreateMenus();
	CreateToolbar();

#if wxUSE_DRAG_AND_DROP
	SetDropTarget(new DnDFile);
#endif

	m_pTree = new MyTreeCtrl(this, LayerTree_Ctrl,
			wxPoint(0, 0), wxSize(200, 400),
			wxTR_HIDE_ROOT | wxNO_BORDER);

	m_mgr.AddPane(m_pTree, wxAuiPaneInfo().
				  Name(_T("layers")).Caption(_("Layers")).
				  Left());
	m_mgr.Update();

	// The following makes the views match, but it looks funny on Linux
//	m_pTree->SetBackgroundColour(*wxLIGHT_GREY);

	CreateView();
	m_pView->SetBackgroundColour(*wxLIGHT_GREY);
	m_pView->Show(FALSE);

	// Read INI file after creating the view
	if (!ReadXML(APPNAME ".xml"))
	{
		// fall back on older ini file
		ReadINI(APPNAME ".ini");
	}

	// Safety checks
	CheckOptionBounds();

	// Apply all the options, from g_Options the rest of the application
	ApplyOptions();

	RefreshToolbars();

	vtProjection proj;
	proj.SetWellKnownGeogCS("WGS84");
	SetProjection(proj);
	RefreshStatusBar();

	// Load structure defaults
	bool foundmaterials = LoadGlobalMaterials();
	if (!foundmaterials)
		DisplayAndLog("The building materials file (Culture/materials.xml) was not found\n"
			" on your Data Path.  Without this file, materials will not be handled\n"
			" correctly.  Please check your Data Paths to avoid this problem.");
	SetupDefaultStructures(FindFileOnPaths(vtGetDataPath(), "BuildingData/DefaultStructures.vtst"));

	// Load content files, which might be referenced by structure layers
	LookForContentFiles();

#if 0
	long style = 0;//wxCAPTION | wxCLOSE_BOX;
	m_pScaleBar = new vtScaleBar(this, wxID_ANY, wxPoint(0,0), wxSize(400,30), style,
		_T("ScaleBarPanel"));
	m_mgr.AddPane(m_pScaleBar, wxAuiPaneInfo().
				  Name(wxT("scalebar")).Caption(wxT("Scale Bar")).
				  Dockable(false).Float().MinSize(400,30).MaxSize(400,60).Resizable(false));
	m_mgr.Update();
#endif

	// Again, for good measure
	m_mgr.Update();

	SetStatusText(_("Ready"));
}

void MainFrame::OnClose(wxCloseEvent &event)
{
	VTLOG("Frame OnClose\n");
	int num = NumModifiedLayers();
	if (num > 0)
	{
		wxString str;
		str.Printf(_("There are %d layers modified but unsaved.\n Are you sure you want to exit?"), num);
		if (wxMessageBox(str, _("Warning"), wxYES_NO) == wxNO)
		{
			event.Veto();
			return;
		}
	}

	if (m_pFeatInfoDlg != NULL)
	{
		// For some reason, destroying the list control in the feature
		//  dialog is dangerous if allowed to occur naturally, but it is
		//  safe to do it at this point.
		m_pFeatInfoDlg->Clear();
	}

	Destroy();
}

void MainFrame::ManageToolbar(const wxString &name, wxToolBar *bar, bool show)
{
	wxAuiPaneInfo api;
	api.Name(name);
	api.ToolbarPane();
	api.Top();
	api.LeftDockable(false);
	api.RightDockable(false);
	api.Show(show);
	m_mgr.AddPane(bar, api);
}

wxToolBar *MainFrame::NewToolbar()
{
	int style = (wxTB_FLAT | wxTB_NODIVIDER);
	wxToolBar *bar = new wxToolBar(this, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, style);
	bar->SetMargins(1, 1);
	bar->SetToolBitmapSize(wxSize(20, 20));
	return bar;
}

void MainFrame::CreateToolbar()
{
	// tool bar
	m_pToolbar = NewToolbar();
	m_pLayBar[LT_RAW] = NewToolbar();
	m_pLayBar[LT_ELEVATION] = NewToolbar();
	m_pLayBar[LT_IMAGE] = NewToolbar();
	m_pLayBar[LT_ROAD] = NewToolbar();
	m_pLayBar[LT_STRUCTURE] = NewToolbar();
	m_pLayBar[LT_VEG] = NewToolbar();
	m_pLayBar[LT_UTILITY] = NewToolbar();

	AddMainToolbars();

	ManageToolbar(_T("toolbar"), m_pToolbar, true);
	ManageToolbar(_T("toolbar_raw"), m_pLayBar[LT_RAW], false);
	ManageToolbar(_T("toolbar_elev"), m_pLayBar[LT_ELEVATION], false);
	ManageToolbar(_T("toolbar_image"), m_pLayBar[LT_IMAGE], false);
	ManageToolbar(_T("toolbar_road"), m_pLayBar[LT_ROAD], false);
	ManageToolbar(_T("toolbar_struct"), m_pLayBar[LT_STRUCTURE], false);
	ManageToolbar(_T("toolbar_veg"), m_pLayBar[LT_VEG], false);
	ManageToolbar(_T("toolbar_util"), m_pLayBar[LT_UTILITY], false);
	m_mgr.Update();
}

void MainFrame::RefreshToolbars()
{
	vtLayer *lay = GetActiveLayer();
	LayerType lt = LT_UNKNOWN;
	if (lay)
		lt = lay->GetType();

	m_pMenuBar->EnableTop(m_iLayerMenu[LT_ELEVATION], lt == LT_ELEVATION);
	m_pMenuBar->EnableTop(m_iLayerMenu[LT_IMAGE], lt == LT_IMAGE);
	m_pMenuBar->EnableTop(m_iLayerMenu[LT_ROAD], lt == LT_ROAD);
	m_pMenuBar->EnableTop(m_iLayerMenu[LT_UTILITY], lt == LT_UTILITY);
//	m_pMenuBar->EnableTop(m_iLayerMenu[LT_VEG], lt == LT_VEG);
	m_pMenuBar->EnableTop(m_iLayerMenu[LT_STRUCTURE], lt == LT_STRUCTURE);
	m_pMenuBar->EnableTop(m_iLayerMenu[LT_RAW], lt == LT_RAW);

	for (int i = 0; i < LAYER_TYPES; i++)
	{
		wxToolBar *bar = m_pLayBar[i];
		if (bar)
		{
			wxAuiPaneInfo &info = m_mgr.GetPane(bar);
			info.Show(lt == i);
		}
	}
	m_mgr.Update();
}

void MainFrame::AddMainToolbars()
{
	ADD_TOOL(m_pToolbar, ID_FILE_NEW, wxBITMAP(proj_new), _("New Project"));
	ADD_TOOL(m_pToolbar, ID_FILE_OPEN, wxBITMAP(proj_open), _("Open Project"));
	ADD_TOOL(m_pToolbar, ID_FILE_SAVE, wxBITMAP(proj_save), _("Save Project"));
	ADD_TOOL(m_pToolbar, ID_VIEW_OPTIONS, wxBITMAP(view_options), _("View Options"));
	m_pToolbar->AddSeparator();
	ADD_TOOL(m_pToolbar, ID_LAYER_NEW, wxBITMAP(layer_new), _("New Layer"));
	ADD_TOOL(m_pToolbar, ID_LAYER_OPEN, wxBITMAP(layer_open), _("Open Layer"));
	ADD_TOOL(m_pToolbar, ID_LAYER_SAVE, wxBITMAP(layer_save), _("Save Layer"));
	ADD_TOOL(m_pToolbar, ID_LAYER_IMPORT, wxBITMAP(layer_import), _("Import Data"));
	m_pToolbar->AddSeparator();
	ADD_TOOL(m_pToolbar, ID_EDIT_DELETE, wxBITMAP(edit_delete), _("Delete"));
	ADD_TOOL(m_pToolbar, ID_EDIT_OFFSET, wxBITMAP(edit_offset), _("Offset"));
	ADD_TOOL2(m_pToolbar, ID_VIEW_SHOWLAYER, wxBITMAP(layer_show), _("Layer Visibility"), wxITEM_CHECK);
	ADD_TOOL(m_pToolbar, ID_VIEW_LAYER_UP, ToolBitmapsFunc( 0 ), _("Layer Up"));
	ADD_TOOL(m_pToolbar, ID_VIEW_LAYER_DOWN, ToolBitmapsFunc( 1 ), _("Layer Down"));
	m_pToolbar->AddSeparator();
	ADD_TOOL(m_pToolbar, ID_VIEW_ZOOMIN, wxBITMAP(view_plus), _("Zoom In"));
	ADD_TOOL(m_pToolbar, ID_VIEW_ZOOMOUT, wxBITMAP(view_minus), _("Zoom Out"));
	ADD_TOOL(m_pToolbar, ID_VIEW_ZOOMALL, wxBITMAP(view_zoomall), _("Zoom All"));
	ADD_TOOL(m_pToolbar, ID_VIEW_ZOOM_LAYER, wxBITMAP(view_zoom_layer), _("Zoom To Layer"));
	m_pToolbar->AddSeparator();
	ADD_TOOL2(m_pToolbar, ID_VIEW_MAGNIFIER, wxBITMAP(view_mag), _("Magnifier"), wxITEM_CHECK);
	ADD_TOOL2(m_pToolbar, ID_VIEW_PAN, wxBITMAP(view_hand), _("Pan"), wxITEM_CHECK);
	ADD_TOOL2(m_pToolbar, ID_VIEW_DISTANCE, wxBITMAP(distance), _("Distance"), wxITEM_CHECK);
	ADD_TOOL2(m_pToolbar, ID_VIEW_SETAREA, wxBITMAP(elev_box), _("Area Tool"), wxITEM_CHECK);
	ADD_TOOL2(m_pToolbar, ID_VIEW_PROFILE, wxBITMAP(view_profile), _("Elevation Profile"), wxITEM_CHECK);
	m_pToolbar->AddSeparator();
	ADD_TOOL(m_pToolbar, ID_AREA_EXPORT_ELEV, wxBITMAP(elev_resample), _("Merge/Resample Elevation"));
	ADD_TOOL(m_pToolbar, ID_AREA_EXPORT_IMAGE, wxBITMAP(image_resample), _("Merge/Resample Imagery"));
	m_pToolbar->Realize();

	// Raw
	ADD_TOOL2(m_pLayBar[LT_RAW], ID_FEATURE_SELECT, wxBITMAP(select), _("Select Features"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_RAW], ID_FEATURE_PICK, wxBITMAP(info), _("Pick Features"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_RAW], ID_FEATURE_TABLE, wxBITMAP(table), _("Table"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_RAW], ID_RAW_ADDPOINTS, wxBITMAP(raw_add_point), _("Add Points with Mouse"), wxITEM_CHECK);
	m_pLayBar[LT_RAW]->Realize();

	// Elevation
	ADD_TOOL2(m_pLayBar[LT_ELEVATION], ID_ELEV_SELECT, wxBITMAP(select), _("Select Elevation"), wxITEM_CHECK);
	ADD_TOOL(m_pLayBar[LT_ELEVATION], ID_VIEW_FULLVIEW, wxBITMAP(view_zoomexact), _("Zoom to Full Detail"));
	ADD_TOOL2(m_pLayBar[LT_ELEVATION], ID_ELEV_TRIMTIN, wxBITMAP(tin_trim), _("Trim TIN triangles by line segment"), wxITEM_CHECK);
	m_pLayBar[LT_ELEVATION]->Realize();

	// Image
	ADD_TOOL(m_pLayBar[LT_IMAGE], ID_VIEW_FULLVIEW, wxBITMAP(view_zoomexact), _("Zoom to Full Detail"));
	m_pLayBar[LT_IMAGE]->Realize();

	// Road
	ADD_TOOL2(m_pLayBar[LT_ROAD], ID_ROAD_SELECTROAD, wxBITMAP(rd_select_road), _("Select Roads"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_ROAD], ID_ROAD_SELECTNODE, wxBITMAP(rd_select_node), _("Select Nodes"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_ROAD], ID_ROAD_SELECTWHOLE, wxBITMAP(rd_select_whole), _("Select Whole Roads"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_ROAD], ID_ROAD_DIRECTION, wxBITMAP(rd_direction), _("Set Road Direction"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_ROAD], ID_ROAD_EDIT, wxBITMAP(rd_edit), _("Edit Road Points"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_ROAD], ID_ROAD_SHOWNODES, wxBITMAP(rd_shownodes), _("Show Nodes"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_ROAD], ID_EDIT_CROSSINGSELECTION, wxBITMAP(edit_crossing), _("Crossing Selection"), wxITEM_CHECK);
	m_pLayBar[LT_ROAD]->Realize();

	// Structure
	ADD_TOOL2(m_pLayBar[LT_STRUCTURE], ID_FEATURE_SELECT, wxBITMAP(select), _("Select Features"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_STRUCTURE], ID_STRUCTURE_EDIT_BLD, wxBITMAP(bld_edit), _("Edit Buildings"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_STRUCTURE], ID_STRUCTURE_ADD_POINTS, wxBITMAP(bld_add_points), _("Add points to building footprints"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_STRUCTURE], ID_STRUCTURE_DELETE_POINTS, wxBITMAP(bld_delete_points), _("Delete points from building footprints"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_STRUCTURE], ID_STRUCTURE_ADD_LINEAR, wxBITMAP(str_add_linear), _("Add Linear Structures"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_STRUCTURE], ID_STRUCTURE_EDIT_LINEAR, wxBITMAP(str_edit_linear), _("Edit Linear Structures"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_STRUCTURE], ID_STRUCTURE_CONSTRAIN, wxBITMAP(bld_corner), _("Constrain Angles"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_STRUCTURE], ID_STRUCTURE_ADD_INST, wxBITMAP(instances), _("Add Instances"), wxITEM_CHECK);
	m_pLayBar[LT_STRUCTURE]->Realize();

	// Veg
	ADD_TOOL2(m_pLayBar[LT_VEG], ID_FEATURE_SELECT, wxBITMAP(select), _("Select Plants"), wxITEM_CHECK);
	m_pLayBar[LT_VEG]->Realize();

	// Utility
	ADD_TOOL2(m_pLayBar[LT_UTILITY], ID_TOWER_ADD,wxBITMAP(rd_select_node), _("Add Tower"), wxITEM_CHECK);
	m_pLayBar[LT_UTILITY]->AddSeparator();
	ADD_TOOL2(m_pLayBar[LT_UTILITY], ID_TOWER_SELECT,wxBITMAP(select),_("Select Towers"), wxITEM_CHECK);
	ADD_TOOL2(m_pLayBar[LT_UTILITY], ID_TOWER_EDIT, wxBITMAP(twr_edit), _("Edit Towers"), wxITEM_CHECK);
	m_pLayBar[LT_UTILITY]->Realize();
}


////////////////////////////////////////////////////////////////
// Application Methods

//
// Load a layer from a file without knowing its type
//
void MainFrame::LoadLayer(const wxString &fname_in)
{
	LayerType ltype = LT_UNKNOWN;

	// check file extension
	wxString fname = fname_in;
	wxString ext = fname.AfterLast('.');

	vtLayer *pLayer = NULL;
	if (ext.CmpNoCase(_T("rmf")) == 0)
	{
		vtRoadLayer *pRL = new vtRoadLayer;
		if (pRL->Load(fname))
			pLayer = pRL;
		else
			delete pRL;
	}
	if (ext.CmpNoCase(_T("osm")) == 0)
	{
		OpenProgressDialog(fname, false);
		vtString fname_utf = (const char*)fname.mb_str(wxConvUTF8);
		vtRoadLayer *pRL = new vtRoadLayer;
		if (pRL->ImportFromOSM(fname_utf, progress_callback))
			pLayer = pRL;
		else
			delete pRL;
		CloseProgressDialog();
	}
	if (ext.CmpNoCase(_T("bt")) == 0 ||
		ext.CmpNoCase(_T("tin")) == 0 ||
		ext.CmpNoCase(_T("itf")) == 0 ||
		fname.Right(6).CmpNoCase(_T(".bt.gz")) == 0)
	{
		vtElevLayer *pEL = new vtElevLayer;
		if (pEL->Load(fname))
			pLayer = pEL;
		else
			delete pEL;
	}
#if SUPPORT_TRANSIT
	if (ext.CmpNoCase(_T("xml")) == 0)
	{
		vtTransitLayer *pTL = new vtTransitLayer;
		if (pTL->Load(fname))
			pLayer = pTL;
	}
#endif
	if (ext.CmpNoCase(_T("vtst")) == 0 ||
		fname.Right(8).CmpNoCase(_T(".vtst.gz")) == 0)
	{
		vtStructureLayer *pSL = new vtStructureLayer;
		if (pSL->Load(fname))
			pLayer = pSL;
		else
			delete pSL;
	}
	if (ext.CmpNoCase(_T("vf")) == 0)
	{
		vtVegLayer *pVL = new vtVegLayer;
		if (pVL->Load(fname))
			pLayer = pVL;
		else
			delete pVL;
	}
	if (ext.CmpNoCase(_T("utl")) == 0)
	{
		vtUtilityLayer *pTR = new vtUtilityLayer;
		if(pTR->Load(fname))
			pLayer = pTR;
		else
			delete pTR;
	}
	if (ext.CmpNoCase(_T("shp")) == 0 ||
		ext.CmpNoCase(_T("gml")) == 0 ||
		ext.CmpNoCase(_T("xml")) == 0 ||
		ext.CmpNoCase(_T("igc")) == 0)
	{
		vtRawLayer *pRL = new vtRawLayer;
		if (pRL->Load(fname))
			pLayer = pRL;
		else
			delete pRL;
	}
	if (ext.CmpNoCase(_T("img")) == 0)
	{
		vtImageLayer *pIL = new vtImageLayer;
		if (pIL->Load(fname))
			pLayer = pIL;
		else
			delete pIL;
	}
	if (ext.CmpNoCase(_T("tif")) == 0)
	{
		// If it's a 8-bit or 24-bit TIF, then it's likely to be an image.
		// If it's a 16-bit TIF, then it's likely to be elevation.
		int depth = GetBitDepthUsingGDAL(fname_in.mb_str(wxConvUTF8));
		if (depth == 8 || depth == 24 || depth == 32)
		{
			vtImageLayer *pIL = new vtImageLayer;
			if (pIL->Load(fname))
				pLayer = pIL;
			else
				delete pIL;
		}
		else if (depth == 16)
			ltype = LT_ELEVATION;
	}
	if (pLayer)
	{
		bool success = AddLayerWithCheck(pLayer, true);
		if (!success)
			delete pLayer;
	}
	else
	{
		// try importing
		ImportDataFromArchive(ltype, fname, true);
	}

}

void MainFrame::SetProjection(const vtProjection &p)
{
	Builder::SetProjection(p);

	// inform the world map view
	GetView()->SetWMProj(p);

	// inform the dialogs that care, if they're open
	if (m_pDistanceDlg)
		m_pDistanceDlg->SetProjection(m_proj);
	if (m_pInstanceDlg)
		m_pInstanceDlg->SetProjection(m_proj);
	if (m_pProfileDlg)
		m_pProfileDlg->SetProjection(m_proj);
}

bool MainFrame::AddLayerWithCheck(vtLayer *pLayer, bool bRefresh)
{
	bool result = Builder::AddLayerWithCheck(pLayer, bRefresh);
	if (result && bRefresh)
	{
		// refresh the view
		ZoomAll();
		RefreshToolbars();
		RefreshTreeView();
		RefreshStatusBar();
	}
	return result;
}

void MainFrame::RemoveLayer(vtLayer *lp)
{
	if (!lp)
		return;

	Builder::RemoveLayer(lp);

	// if it was being shown in the feature info dialog, reset that dialog
	if (m_pFeatInfoDlg && m_pFeatInfoDlg->GetLayer() == lp)
	{
		m_pFeatInfoDlg->SetLayer(NULL);
		m_pFeatInfoDlg->SetFeatureSet(NULL);
	}
	m_pView->Refresh();
	m_pTree->RefreshTreeItems(this);
	RefreshToolbars();
}

void MainFrame::SetActiveLayer(vtLayer *lp, bool refresh)
{
	LayerType last = m_pActiveLayer ? m_pActiveLayer->GetType() : LT_UNKNOWN;

	Builder::SetActiveLayer(lp);
	if (refresh)
		m_pTree->RefreshTreeStatus(this);

	// change mouse mode based on layer type
	if (lp == NULL)
		m_pView->SetMode(LB_Mag);

	if (lp != NULL)
	{
		if (lp->GetType() == LT_ELEVATION && last != LT_ELEVATION)
			m_pView->SetMode(LB_TSelect);

		if (lp->GetType() == LT_ROAD && last != LT_ROAD)
			m_pView->SetMode(LB_Link);

		if (lp->GetType() == LT_STRUCTURE && last != LT_STRUCTURE)
			m_pView->SetMode(LB_FSelect);

		if (lp->GetType() == LT_UTILITY && last != LT_UTILITY)
			m_pView->SetMode(LB_FSelect);

		if (lp->GetType() == LT_RAW && last != LT_RAW)
			m_pView->SetMode(LB_FSelect);
	}
}

void MainFrame::RefreshLayerInView(vtLayer *pLayer)
{
	DRECT r;
	pLayer->GetExtent(r);
	wxRect sr = m_pView->WorldToWindow(r);
	IncreaseRect(sr, 5);
	m_pView->Refresh(TRUE, &sr);
}


//
// read / write ini file
//
bool MainFrame::ReadINI(const char *fname)
{
	FILE *fpIni = vtFileOpen(fname, "rb+");

	if (!fpIni)
		return false;

	int ShowMap, ShowElev, ShadeQuick, DoMask, DoUTM, ShowPaths, DrawWidth,
		CastShadows, ShadeDot=0, Angle=30, Direction=45;
	float Ambient = 0.1f;
	float Gamma = 0.8f;
	fscanf(fpIni, "%d %d %d %d %d %d %d %d %d %d %d %f %f", &ShowMap,
		&ShowElev, &ShadeQuick, &DoMask, &DoUTM, &ShowPaths, &DrawWidth,
		&CastShadows, &ShadeDot, &Angle, &Direction, &Ambient, &Gamma);

	vtElevLayer::m_draw.m_bShowElevation = (ShowElev != 0);
	vtElevLayer::m_draw.m_bShadingQuick = (ShadeQuick != 0);
	vtElevLayer::m_draw.m_bShadingDot = (ShadeDot != 0);
	vtElevLayer::m_draw.m_bDoMask = (DoMask != 0);
	vtElevLayer::m_draw.m_bCastShadows = (CastShadows != 0);
	vtElevLayer::m_draw.m_iCastAngle = Angle;
	vtElevLayer::m_draw.m_iCastDirection = Direction;
	vtElevLayer::m_draw.m_fAmbient = Ambient;
	vtElevLayer::m_draw.m_fGamma = Gamma;
	m_pView->SetShowMap(ShowMap != 0);
	m_pView->m_bShowUTMBounds = (DoUTM != 0);
	m_pTree->SetShowPaths(ShowPaths != 0);
	vtRoadLayer::SetDrawWidth(DrawWidth != 0);

	char buf[4000];
	if (fscanf(fpIni, "\n%s\n", buf) == 1)
	{
		wxString str(buf, wxConvUTF8);
		m_mgr.LoadPerspective(str, false);
	}

	return true;
}

//
// read / write options file
//
bool MainFrame::ReadXML(const char *fname)
{
	return g_Options.LoadFromXML(fname);
}

void MainFrame::ApplyOptions()
{
	// Apply all the options, from g_Options the rest of the application
	m_pView->SetShowMap(g_Options.GetValueBool(TAG_SHOW_MAP));
	m_pView->m_bShowUTMBounds = g_Options.GetValueBool(TAG_SHOW_UTM);
	m_pTree->SetShowPaths(g_Options.GetValueBool(TAG_SHOW_PATHS));
	vtRoadLayer::SetDrawWidth(g_Options.GetValueBool(TAG_ROAD_DRAW_WIDTH));

	vtImage::bTreatBlackAsTransparent = g_Options.GetValueBool(TAG_BLACK_TRANSP);
	vtElevLayer::m_bDefaultGZip = g_Options.GetValueBool(TAG_DEFAULT_GZIP_BT);
	vtElevLayer::m_draw.SetFromTags(g_Options);

	vtString str;
	if (g_Options.GetValueString("UI_Layout", str))
	{
		wxString str2(str, wxConvUTF8);
		m_mgr.LoadPerspective(str2, false);
	}
}

bool MainFrame::WriteXML(const char *fname)
{
	// Gather all the options into g_Options
	g_Options.SetValueBool(TAG_SHOW_MAP, m_pView->GetShowMap());
	g_Options.SetValueBool(TAG_SHOW_UTM, m_pView->m_bShowUTMBounds);
	g_Options.SetValueBool(TAG_SHOW_PATHS, m_pTree->GetShowPaths());
	g_Options.SetValueBool(TAG_ROAD_DRAW_WIDTH, vtRoadLayer::GetDrawWidth());

	vtElevLayer::m_draw.SetToTags(g_Options);

	wxString str = m_mgr.SavePerspective();
	g_Options.SetValueString("UI_Layout", (const char *) str.mb_str(wxConvUTF8));

	// Write it to XML
	return g_Options.WriteToXML(fname, "Options");
}

void MainFrame::RefreshTreeView()
{
	if (m_pTree)
		m_pTree->RefreshTreeItems(this);
}

void MainFrame::RefreshTreeStatus()
{
	if (m_pTree)
		m_pTree->RefreshTreeStatus(this);
}

void MainFrame::RefreshStatusBar()
{
	m_statbar->SetTexts(this);
}

FeatInfoDlg	*MainFrame::ShowFeatInfoDlg()
{
	if (!m_pFeatInfoDlg)
	{
		// Create new Feature Info Dialog
		m_pFeatInfoDlg = new FeatInfoDlg(this, wxID_ANY, _("Feature Info"),
				wxPoint(120, 80), wxSize(600, 200), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
		m_pFeatInfoDlg->SetView(GetView());
	}
	m_pFeatInfoDlg->Show(true);
	return m_pFeatInfoDlg;
}


DistanceDlg2d *MainFrame::ShowDistanceDlg()
{
	if (!m_pDistanceDlg)
	{
		// Create new Distance Dialog
		m_pDistanceDlg = new DistanceDlg2d(this, wxID_ANY, _("Distance Tool"),
				wxPoint(200, 200), wxSize(600, 200), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
		m_pDistanceDlg->SetProjection(m_proj);
		m_pDistanceDlg->SetFrame(this);
	}
	m_pDistanceDlg->Show(true);
	return m_pDistanceDlg;
}

void MainFrame::UpdateDistance(const DPoint2 &p1, const DPoint2 &p2)
{
	DistanceDlg2d *pDlg = ShowDistanceDlg();
	if (pDlg)
	{
		pDlg->SetPoints(p1, p2, true);
		float h1 = GetHeightFromTerrain(p1);
		float h2 = GetHeightFromTerrain(p2);
		float diff = FLT_MIN;
		if (h1 != INVALID_ELEVATION && h2 != INVALID_ELEVATION)
			diff = h2 - h1;
		if (pDlg)
			pDlg->SetGroundAndVertical(FLT_MIN, diff, false);
	}

	ProfileDlg *pDlg2 = m_pProfileDlg;
	if (pDlg2)
		pDlg2->SetPoints(p1, p2);
}

void MainFrame::UpdateDistance(const DLine2 &path)
{
	DistanceDlg2d *pDlg = ShowDistanceDlg();
	if (pDlg)
	{
		pDlg->SetPath(path, true);
		//float h1 = GetHeightFromTerrain(p1);
		//float h2 = GetHeightFromTerrain(p2);
		//float diff = FLT_MIN;
		//if (h1 != INVALID_ELEVATION && h2 != INVALID_ELEVATION)
		//	diff = h2 - h1;
		//if (pDlg)
		//	pDlg->SetGroundAndVertical(FLT_MIN, diff, false);
	}

	ProfileDlg *pDlg2 = m_pProfileDlg;
	if (pDlg2)
		pDlg2->SetPath(path);
}

void MainFrame::ClearDistance()
{
	UpdateDistance(DPoint2(0,0), DPoint2(0,0));
	UpdateDistance(DLine2());

	// erase previous
	GetView()->ClearDistanceTool();
}


class LinearStructureDlg2d: public LinearStructureDlg
{
public:
	LinearStructureDlg2d(wxWindow *parent, wxWindowID id, const wxString &title,
		const wxPoint& pos, const wxSize& size, long style) :
	LinearStructureDlg(parent, id, title, pos, size, style) {}
	void OnSetOptions(const vtLinearParams &opt)
	{
		g_bld->m_LSOptions = opt;
	}
	MainFrame *m_pFrame;
};

LinearStructureDlg *MainFrame::ShowLinearStructureDlg(bool bShow)
{
	if (bShow && !m_pLinearStructureDlg)
	{
		// Create new Distance Dialog
		m_pLinearStructureDlg = new LinearStructureDlg2d(this, -1,
			_("Linear Structures"), wxPoint(120, 80), wxSize(600, 200),
			wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
		m_pLinearStructureDlg->m_pFrame = this;
	}
	if (m_pLinearStructureDlg)
		m_pLinearStructureDlg->Show(bShow);
	return m_pLinearStructureDlg;
}


InstanceDlg *MainFrame::ShowInstanceDlg(bool bShow)
{
	if (bShow && !m_pInstanceDlg)
	{
		// Create new Distance Dialog
		m_pInstanceDlg = new InstanceDlg(this, -1,
			_("Structure Instances"), wxPoint(120, 80), wxSize(600, 200));

		for (unsigned int i = 0; i < m_contents.size(); i++)
			m_pInstanceDlg->AddContent(m_contents[i]);
		m_pInstanceDlg->SetProjection(m_proj);
	}
	if (m_pInstanceDlg)
		m_pInstanceDlg->Show(bShow);
	return m_pInstanceDlg;
}

class BuildingProfileCallback : public ProfileCallback
{
public:
	void Begin()
	{
		m_elevs.clear();
		m_frame->ElevLayerArray(m_elevs);
	}
	float GetElevation(const DPoint2 &p)
	{
		return ElevLayerArrayValue(m_elevs, p);
	}
	float GetCultureHeight(const DPoint2 &p)
	{
		return INVALID_ELEVATION;
	}
	MainFrame *m_frame;
	std::vector<vtElevLayer*> m_elevs;
};

ProfileDlg *MainFrame::ShowProfileDlg()
{
	if (!m_pProfileDlg)
	{
		// Create new Feature Info Dialog
		m_pProfileDlg = new ProfileDlg(this, wxID_ANY, _("Elevation Profile"),
				wxPoint(120, 80), wxSize(730, 500), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
		BuildingProfileCallback *callback = new BuildingProfileCallback;
		callback->m_frame = this;
		m_pProfileDlg->SetCallback(callback);
		m_pProfileDlg->SetProjection(m_proj);
	}
	m_pProfileDlg->Show(true);

	// This might be the first time it's displayed, so we need to get
	//  the point values from the distance tool
	GetView()->UpdateDistance();

	return m_pProfileDlg;
}

void MainFrame::OnSelectionChanged()
{
	if (m_pFeatInfoDlg && m_pFeatInfoDlg->IsShown())
	{
		vtRawLayer *pRL = GetActiveRawLayer();
		m_pFeatInfoDlg->SetFeatureSet(pRL->GetFeatureSet());
		m_pFeatInfoDlg->ShowSelected();
	}
}

void MainFrame::UpdateFeatureDialog(vtRawLayer *raw,
									vtFeatureSetPoint2D *pSetP2, int iEntity)
{
	DPoint2 &p2 = pSetP2->GetPoint(iEntity);
	vtArray<int> found;
	pSetP2->FindAllPointsAtLocation(p2, found);

	FeatInfoDlg	*fdlg = ShowFeatInfoDlg();
	fdlg->SetLayer(raw);
	fdlg->SetFeatureSet(pSetP2);
	pSetP2->DePickAll();
	for (unsigned int i = 0; i < found.GetSize(); i++)
		pSetP2->Pick(found[i]);
	fdlg->ShowPicked();
}

void MainFrame::OnSetMode(LBMode m)
{
	// Show this dialog only in AddLinear mode
	ShowLinearStructureDlg(m == LB_AddLinear);

	// Show this dialog only in AddInstance mode
	ShowInstanceDlg(m == LB_AddInstance);
}


////////////////////////////////////////////////////////////////
// Project operations

void MainFrame::LoadProject(const wxString &strPathName)
{
	vtString fname = (const char *) strPathName.mb_str(wxConvUTF8);
	VTLOG("Loading project: '%s'\n", (const char *) fname);

	// avoid trying to draw while we're loading the project
	m_bDrawDisabled = true;

	Builder::LoadProject(fname, m_pView);

	// refresh the view
	m_bDrawDisabled = false;

	RefreshTreeView();
	RefreshToolbars();
}

void MainFrame::SaveProject(const wxString &strPathName) const
{
	// Avoid trouble with '.' and ',' in Europe
	LocaleWrap normal_numbers(LC_NUMERIC, "C");

	// write project file
	FILE *fp = vtFileOpen(strPathName.mb_str(wxConvUTF8), "wb");
	if (!fp)
		return;

	// write projection info
	char *wkt;
	m_proj.exportToWkt(&wkt);
	fprintf(fp, "Projection %s\n", wkt);
	OGRFree(wkt);

	if (m_strSpeciesFilename != "")
	{
		fprintf(fp, "PlantList %s\n", (const char *) m_strSpeciesFilename);
	}

	if (m_strBiotypesFilename != "")
	{
		fprintf(fp, "BioTypes %s\n", (const char *) m_strBiotypesFilename);
	}

	// write list of layers
	int iLayers = m_Layers.GetSize();
	fprintf(fp, "layers: %d\n", iLayers);

	vtLayer *lp;
	for (int i = 0; i < iLayers; i++)
	{
		lp = m_Layers.GetAt(i);

		bool bNative = lp->IsNative();

		fprintf(fp, "type %d, %s", lp->GetType(), bNative ? "native" : "import");
		if (!lp->GetVisible())
			fprintf(fp, " hidden");
		fprintf(fp, "\n");

		wxString fname = lp->GetLayerFilename();
		if (!bNative)
		{
			if (lp->GetImportedFrom() != _T(""))
				fname = lp->GetImportedFrom();
		}
		fprintf(fp, "%s\n", (const char *) fname.mb_str(wxConvUTF8));
	}

	// write area
	fprintf(fp, "area %lf %lf %lf %lf\n", m_area.left, m_area.top,
		m_area.right, m_area.bottom);

	// write view rectangle
	DRECT rect = m_pView->GetWorldRect();
	fprintf(fp, "view %lf %lf %lf %lf\n", rect.left, rect.top,
		rect.right, rect.bottom);

	// done
	fclose(fp);
}

void MainFrame::ShowOptionsDialog()
{
	OptionsDlg dlg(this, -1, _("Options"));

	dlg.m_bShowToolbar = m_pToolbar->IsShown();
	dlg.m_bShowMinutes = m_statbar->m_bShowMinutes;
	dlg.m_iElevUnits = (int)(m_statbar->m_ShowVertUnits) - 1;

	dlg.SetElevDrawOptions(vtElevLayer::m_draw);

	dlg.m_bShowRoadWidth = vtRoadLayer::GetDrawWidth();
	bool bDrawRawSimple = g_Options.GetValueBool(TAG_DRAW_RAW_SIMPLE);
	dlg.m_bDrawRawSimple = bDrawRawSimple;
	dlg.m_bShowPath = m_pTree->GetShowPaths();

	if (dlg.ShowModal() != wxID_OK)
		return;

	bool bNeedRefresh = false;

	if (dlg.m_bShowToolbar != m_pToolbar->IsShown())
	{
		m_pToolbar->Show(dlg.m_bShowToolbar);
		// send a fake OnSize event so the frame will draw itself correctly
		wxSizeEvent dummy;
		wxFrame::OnSize(dummy);
	}
	m_statbar->m_bShowMinutes = dlg.m_bShowMinutes;
	m_statbar->m_ShowVertUnits = (LinearUnits) (dlg.m_iElevUnits + 1);

	ElevDrawOptions opt;
	dlg.GetElevDrawOptions(opt);

	if (vtElevLayer::m_draw != opt)
	{
		vtElevLayer::m_draw = opt;

		// tell them to redraw themselves
		for (unsigned int i = 0; i < m_Layers.GetSize(); i++)
		{
			vtLayer *lp = m_Layers.GetAt(i);
			if (lp->GetType() == LT_ELEVATION)
			{
				vtElevLayer *elp = (vtElevLayer *)lp;
				elp->ReRender();
				bNeedRefresh = true;
			}
		}
	}

	bool bWidth = dlg.m_bShowRoadWidth;
	if (vtRoadLayer::GetDrawWidth() != bWidth && LayersOfType(LT_ROAD) > 0)
		bNeedRefresh = true;
	vtRoadLayer::SetDrawWidth(bWidth);

	g_Options.SetValueBool(TAG_DRAW_RAW_SIMPLE, dlg.m_bDrawRawSimple);
	if (dlg.m_bDrawRawSimple != bDrawRawSimple)
		bNeedRefresh = true;

	if (dlg.m_bShowPath != m_pTree->GetShowPaths())
	{
		m_pTree->SetShowPaths(dlg.m_bShowPath);
		m_pTree->RefreshTreeItems(this);
	}

	if (bNeedRefresh)
		m_pView->Refresh();
}

#if wxUSE_DRAG_AND_DROP
///////////////////////////////////////////////////////////////////////
// Drag-and-drop functionality
//

bool DnDFile::OnDropFiles(wxCoord, wxCoord, const wxArrayString& filenames)
{
	size_t nFiles = filenames.GetCount();
	for ( size_t n = 0; n < nFiles; n++ )
	{
		wxString str = filenames[n];
		if (!str.Right(3).CmpNoCase(_T("vtb")))
			GetMainFrame()->LoadProject(str);
		else
			GetMainFrame()->LoadLayer(str);
	}
	return TRUE;
}
#endif


//////////////////
// Keyboard shortcuts

void MainFrame::OnChar(wxKeyEvent& event)
{
	m_pView->OnChar(event);
}

void MainFrame::OnKeyDown(wxKeyEvent& event)
{
	m_pView->OnChar(event);
}

void MainFrame::OnMouseWheel(wxMouseEvent& event)
{
	m_pView->OnMouseWheel(event);
}

