//
// Name:	 frame.cpp
// Purpose:  The frame class for the Content Manager.
//
// Copyright (c) 2001-2011 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/stdpaths.h>

#include "vtlib/vtlib.h"
#include "vtlib/core/NavEngines.h"
#include "vtdata/vtLog.h"
#include "vtdata/DataPath.h"
#include "vtui/Helper.h"	// for ProgressDialog

#include "xmlhelper/easyxml.hpp"

#include "app.h"
#include "frame.h"
#include "canvas.h"
#include "menu_id.h"
#include "TreeView.h"
#include "PropDlg.h"
#include "ModelDlg.h"
#include "SceneGraphDlg.h"
#include "ItemGroup.h"
#include "LightDlg.h"

#ifndef __WXMSW__
#  include "icons/cmanager.xpm"
#  include "bitmaps/axes.xpm"
#  include "bitmaps/contents_open.xpm"
#  include "bitmaps/dummy_26x14.xpm"
#  include "bitmaps/item_new.xpm"
#  include "bitmaps/item_remove.xpm"
#  include "bitmaps/model_add.xpm"
#  include "bitmaps/model_remove.xpm"
#  include "bitmaps/properties.xpm"
#  include "bitmaps/rulers.xpm"
#  include "bitmaps/wireframe.xpm"
#endif


DECLARE_APP(vtApp)

//
// Blank window class to use in bottom half of splitter2
//
class Blank: public wxWindow
{
public:
	Blank(wxWindow *parent) : wxWindow(parent, -1) {}
	void OnPaint( wxPaintEvent &event ) { wxPaintDC dc(this); }
	DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(Blank,wxWindow)
	EVT_PAINT( Blank::OnPaint )
END_EVENT_TABLE()


//////////////////////////////////////////////////////////////////////////
// Splitter window management

void Splitter2::SizeWindows()
{
	if (!bResetting)
	{
		int pos = GetSashPosition();
		if (pos != 0) m_last = pos;
//		int w, h;
//		GetClientSize(&w, &h);
//		if (h > 190)
//			SetSashPosition(h - 180, false);
	}
	wxSplitterWindow::SizeWindows();
}

//////////////////////////////////////////////////////////////////////////
// vtFrame class implementation
//

BEGIN_EVENT_TABLE(vtFrame, wxFrame)
	EVT_CHAR(vtFrame::OnChar)
	EVT_CLOSE(vtFrame::OnClose)
	EVT_IDLE(vtFrame::OnIdle)

	EVT_MENU(wxID_OPEN, vtFrame::OnOpen)
	EVT_MENU(wxID_SAVE, vtFrame::OnSave)
	EVT_MENU(wxID_EXIT, vtFrame::OnExit)
	EVT_MENU(ID_SCENE_SCENEGRAPH, vtFrame::OnSceneGraph)
	EVT_MENU(ID_TEST_XML, vtFrame::OnTestXML)
	EVT_MENU(ID_ITEM_NEW, vtFrame::OnItemNew)
	EVT_MENU(ID_ITEM_DEL, vtFrame::OnItemDelete)
	EVT_MENU(ID_ITEM_ADDMODEL, vtFrame::OnItemAddModel)
	EVT_UPDATE_UI(ID_ITEM_ADDMODEL, vtFrame::OnUpdateItemAddModel)
	EVT_MENU(ID_ITEM_REMOVEMODEL, vtFrame::OnItemRemoveModel)
	EVT_UPDATE_UI(ID_ITEM_REMOVEMODEL, vtFrame::OnUpdateItemModelExists)
	EVT_MENU(ID_ITEM_MODELPROPS, vtFrame::OnItemModelProps)
	EVT_UPDATE_UI(ID_ITEM_MODELPROPS, vtFrame::OnUpdateItemModelExists)
	EVT_MENU(ID_ITEM_ROTMODEL, vtFrame::OnItemRotModel)
	EVT_UPDATE_UI(ID_ITEM_ROTMODEL, vtFrame::OnUpdateItemModelExists)
	EVT_MENU(ID_ITEM_SAVESOG, vtFrame::OnItemSaveSOG)
	EVT_MENU(ID_ITEM_SAVEOSG, vtFrame::OnItemSaveOSG)
	EVT_MENU(ID_ITEM_SAVEIVE, vtFrame::OnItemSaveIVE)

	EVT_MENU(ID_VIEW_ORIGIN, vtFrame::OnViewOrigin)
	EVT_UPDATE_UI(ID_VIEW_ORIGIN, vtFrame::OnUpdateViewOrigin)
	EVT_MENU(ID_VIEW_RULERS, vtFrame::OnViewRulers)
	EVT_UPDATE_UI(ID_VIEW_RULERS, vtFrame::OnUpdateViewRulers)
	EVT_MENU(ID_VIEW_WIREFRAME, vtFrame::OnViewWireframe)
	EVT_UPDATE_UI(ID_VIEW_WIREFRAME, vtFrame::OnUpdateViewWireframe)
	EVT_MENU(ID_VIEW_LIGHTS, vtFrame::OnViewLights)

	EVT_UPDATE_UI(ID_ITEM_SAVESOG, vtFrame::OnUpdateItemSaveSOG)
	EVT_MENU(ID_HELP_ABOUT, vtFrame::OnHelpAbout)
END_EVENT_TABLE()


vtFrame *GetMainFrame()
{
	return (vtFrame *) wxGetApp().GetTopWindow();
}


// My frame constructor
vtFrame::vtFrame(wxFrame *parent, const wxString& title, const wxPoint& pos,
	const wxSize& size, long style) :
	wxFrame(parent, wxID_ANY, title, pos, size, style)
{
	m_bCloseOnIdle = false;

	VTLOG(" constructing Frame (%x, title, pos, size, %x)\n", parent, style);
#if WIN32
	// Give it an icon
	SetIcon(wxIcon(_T("cmanager")));
#endif

	ReadDataPath();

	VTLOG("Using Datapaths:\n");
	int i, n = vtGetDataPath().size();
	if (n == 0)
		VTLOG("   none.\n");
	for (i = 0; i < n; i++)
		VTLOG("   %s\n", (const char *) vtGetDataPath()[i]);

	m_pCurrentModel = NULL;
	m_pCurrentItem = NULL;

	m_bShowOrigin = true;
	m_bShowRulers = false;
	m_bWireframe = false;

	CreateMenus();
	CreateToolbar();
	CreateStatusBar();

	SetDropTarget(new DnDFile());

	// frame icon
	SetIcon(wxICON(cmanager));

	VTLOG(" creating component windows\n");
	// splitters
	m_splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, wxSP_3D /*| wxSP_LIVE_UPDATE*/);
	m_splitter2 = new Splitter2(m_splitter, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
	m_blank = new Blank(m_splitter2); // (wxWindowID) -1, _T("blank"), wxDefaultPosition);

	m_pTree = new MyTreeCtrl(m_splitter2, ID_TREECTRL,
		wxPoint(0, 0), wxSize(200, 400),
//		wxTR_HAS_BUTTONS |
		wxTR_EDIT_LABELS |
		wxNO_BORDER);
	m_pTree->SetBackgroundColour(*wxLIGHT_GREY);

	// We definitely want full color and a 24-bit Z-buffer!
	int gl_attrib[8] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER,
		WX_GL_BUFFER_SIZE, 24, WX_GL_DEPTH_SIZE, 24, 0, 0 };

	// Make a vtGLCanvas
	VTLOG(" creating canvas\n");
	m_canvas = new vtGLCanvas(m_splitter, wxID_ANY, wxPoint(0, 0), wxSize(-1, -1),
		0, _T("vtGLCanvas"), gl_attrib);

	VTLOG(" creating scenegraphdialog\n");
	m_pSceneGraphDlg = new SceneGraphDlg(this, wxID_ANY, _T("Scene Graph"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
	m_pSceneGraphDlg->SetSize(250, 350);

	m_pPropDlg = new PropPanel(m_splitter2, wxID_ANY,
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);

	m_pModelDlg = new ModelPanel(m_splitter2, wxID_ANY,
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
	m_pModelDlg->Show(false);
	m_pModelDlg->InitDialog();

	m_pLightDlg = new LightDlg(this, -1, _T("Lights"));

	m_splitter->Initialize(m_splitter2);

	////////////////////////
	m_pTree->Show(true);
	m_canvas->Show(true);
	m_splitter->SplitVertically( m_splitter2, m_canvas, 260);

	m_splitter2->SplitHorizontally( m_pTree, m_blank, 200);
	m_pPropDlg->Show(true);
	m_pPropDlg->InitDialog();

	// Show the frame
	Show(true);

	// Load the font
	vtString fontfile = "Arial.ttf";
	m_pFont = osgText::readFontFile((const char *)fontfile);
	if (!m_pFont.valid())
	{
		vtString fontname = "Fonts/" + fontfile;
		vtString font_path = FindFileOnPaths(vtGetDataPath(), fontname);
		if (font_path != "")
			m_pFont = osgText::readFontFile((const char *)font_path);
		if (!m_pFont.valid())
		{
			VTLOG("Couldn't find or read font from file '%s'\n",
				(const char *) fontname);
		}
	}

#if 0
	// TEST CODE
	osg::Node *node = osgDB::readNodeFile("in.obj");
	osgDB::Registry::instance()->writeNode(*node, "out.osg");
#endif

	SetCurrentItemAndModel(NULL, NULL);

	m_pTree->RefreshTreeItems(this);
}

vtFrame::~vtFrame()
{
	VTLOG(" destructing Frame\n");
	delete m_canvas;
	delete m_pSceneGraphDlg;
	delete m_pLightDlg;
}

void vtFrame::UseLight(vtTransform *pLight)
{
	m_pLightDlg->UseLight(pLight);
}

//////////////////////////////

using namespace std;

void vtFrame::ReadDataPath()
{
	// Look these up, we might need them
	wxString Dir1 = wxStandardPaths::Get().GetUserConfigDir();
	wxString Dir2 = wxStandardPaths::Get().GetConfigDir();

	vtString AppDataUser = (const char *) Dir1.mb_str(wxConvUTF8);
	vtString AppDataCommon = (const char *) Dir2.mb_str(wxConvUTF8);

	// Read the vt datapaths
	vtLoadDataPath(AppDataUser, AppDataCommon);

	vtStringArray &dp = vtGetDataPath();
	// Supply the special symbols {appdata} and {appdatacommon}
	for (unsigned int i = 0; i < dp.size(); i++)
	{
		dp[i].Replace("{appdata}", AppDataUser);
		dp[i].Replace("{appdatacommon}", AppDataCommon);
	}
}

void vtFrame::CreateMenus()
{
	// Make menus
	wxMenu *fileMenu = new wxMenu;
	fileMenu->Append(wxID_OPEN, _T("Open Content File"), _T("Open"));
	fileMenu->Append(wxID_SAVE, _T("&Save Content File"));
	fileMenu->AppendSeparator();
	fileMenu->Append(ID_SCENE_SCENEGRAPH, _T("Scene Graph"));
	fileMenu->AppendSeparator();
	fileMenu->Append(ID_TEST_XML, _T("Test XML"));
	fileMenu->AppendSeparator();
	fileMenu->Append(ID_SET_DATA_PATH, _T("Set Data Path"));
	fileMenu->AppendSeparator();
	fileMenu->Append(wxID_EXIT, _T("E&xit\tEsc"), _T("Exit"));

	wxMenu *itemMenu = new wxMenu;
	itemMenu->Append(ID_ITEM_NEW, _T("New Item"));
	itemMenu->Append(ID_ITEM_DEL, _T("Delete Item"));
	itemMenu->AppendSeparator();
	itemMenu->Append(ID_ITEM_ADDMODEL, _T("Add Model"));
	itemMenu->Append(ID_ITEM_REMOVEMODEL, _T("Remove Model"));
	itemMenu->Append(ID_ITEM_MODELPROPS, _T("Model Properties"));
	itemMenu->AppendSeparator();
	itemMenu->Append(ID_ITEM_ROTMODEL, _T("Rotate Model Around X Axis"));
	itemMenu->AppendSeparator();
	itemMenu->Append(ID_ITEM_SAVESOG, _T("Save Model as SOG"));
	itemMenu->Append(ID_ITEM_SAVEOSG, _T("Save Model as OSG"));
	itemMenu->Append(ID_ITEM_SAVEIVE, _T("Save Model as IVE"));

	wxMenu *viewMenu = new wxMenu;
	viewMenu->AppendCheckItem(ID_VIEW_ORIGIN, _T("Show Local Origin"));
	viewMenu->AppendCheckItem(ID_VIEW_RULERS, _T("Show Rulers"));
	viewMenu->AppendCheckItem(ID_VIEW_WIREFRAME, _T("&Wireframe\tCtrl+W"));
	viewMenu->Append(ID_VIEW_LIGHTS, _T("Lights"));

	wxMenu *helpMenu = new wxMenu;
	helpMenu->Append(ID_HELP_ABOUT, _T("About VTP Content Manager..."));

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(fileMenu, _T("&File"));
	menuBar->Append(itemMenu, _T("&Item"));
	menuBar->Append(viewMenu, _T("&View"));
	menuBar->Append(helpMenu, _T("&Help"));
	SetMenuBar(menuBar);
}

void vtFrame::CreateToolbar()
{
	// tool bar
	m_pToolbar = CreateToolBar(wxTB_HORIZONTAL | wxNO_BORDER | wxTB_DOCKABLE);
	m_pToolbar->SetMargins(2, 2);
	m_pToolbar->SetToolBitmapSize(wxSize(20, 20));

	ADD_TOOL(wxID_OPEN, wxBITMAP(contents_open), _("Open Contents File"), false);
	m_pToolbar->AddSeparator();
	ADD_TOOL(ID_ITEM_NEW, wxBITMAP(item_new), _("New Item"), false);
	ADD_TOOL(ID_ITEM_DEL, wxBITMAP(item_remove), _("Delete Item"), false);
	m_pToolbar->AddSeparator();
	ADD_TOOL(ID_ITEM_ADDMODEL, wxBITMAP(model_add), _("Add Model"), false);
	ADD_TOOL(ID_ITEM_REMOVEMODEL, wxBITMAP(model_remove), _("Remove Model"), false);
	ADD_TOOL(ID_ITEM_MODELPROPS, wxBITMAP(properties), _("Model Properties"), false);
	m_pToolbar->AddSeparator();
	ADD_TOOL(ID_VIEW_ORIGIN, wxBITMAP(axes), _("Show Axes"), true);
	ADD_TOOL(ID_VIEW_RULERS, wxBITMAP(rulers), _("Show Rulers"), true);
	ADD_TOOL(ID_VIEW_WIREFRAME, wxBITMAP(wireframe), _("Wireframe"), true);

	m_pToolbar->Realize();
}

//
// Utility methods
//

void vtFrame::OnChar(wxKeyEvent& event)
{
	long key = event.GetKeyCode();

	if (key == 27)
	{
		// Esc: exit application
		// It's not safe to close immediately, as that will kill the canvas,
		//  and it might some Canvas event that caused us to close.  So,
		//  simply stop rendering, and delay closing until the next Idle event.
		m_canvas->m_bRunning = false;
		m_bCloseOnIdle = true;
	}
}

void vtFrame::OnClose(wxCloseEvent &event)
{
	VTLOG("Frame OnClose\n");

	// Turn on lots of debugging info in case of problems on exit
//	wxLog::SetVerbose(true);
//	wxLog::SetTraceMask(0xf);

	if (m_canvas)
	{
		m_canvas->m_bRunning = false;
		delete m_canvas;
		m_canvas = NULL;
	}
	FreeContents();
	event.Skip();
}

void vtFrame::OnIdle(wxIdleEvent& event)
{
	// Check if we were requested to close on the next Idle event.
	if (m_bCloseOnIdle)
	{
		VTLOG("CloseOnIdle, calling Close()\n");
		Close();
	}
	else
		event.Skip();
}


//
// Intercept menu commands
//

void vtFrame::OnOpen(wxCommandEvent& event)
{
	m_canvas->m_bRunning = false;
	wxFileDialog loadFile(NULL, _T("Load Content File"), _T(""), _T(""),
		_T("Content XML Files (*.vtco)|*.vtco"), wxFD_OPEN);
	loadFile.SetFilterIndex(1);
	if (loadFile.ShowModal() == wxID_OK)
		LoadContentsFile(loadFile.GetPath());

	m_canvas->m_bRunning = true;
}


void vtFrame::OnSave(wxCommandEvent& event)
{
	m_canvas->m_bRunning = false;
	wxFileDialog loadFile(NULL, _T("Save Content File"), _T(""), _T(""),
		_T("Content XML Files (*.vtco)|*.vtco"), wxFD_SAVE);
	loadFile.SetFilterIndex(1);
	if (loadFile.ShowModal() == wxID_OK)
		SaveContentsFile(loadFile.GetPath());

	m_canvas->m_bRunning = true;
}


void vtFrame::LoadContentsFile(const wxString &fname)
{
	VTLOG("LoadContentsFile '%s'\n", (const char *) fname.mb_str(wxConvUTF8));
	FreeContents();
	try
	{
		m_Man.ReadXML(fname.mb_str(wxConvUTF8));
	}
	catch (xh_io_exception &e)
	{
		string str = e.getFormattedMessage();
		DisplayMessageBox(wxString(str.c_str(), wxConvUTF8));
		return;
	}
	SetCurrentItem(NULL);
	SetCurrentModel(NULL);
	m_pTree->RefreshTreeItems(this);
}

void vtFrame::FreeContents()
{
	VTLOG("FreeContents\n");
	for (unsigned int i = 0; i < m_Man.NumItems(); i++)
	{
		vtItem *item = m_Man.GetItem(i);
		ItemGroup *ig = m_itemmap[item];
		delete ig;
	}
	m_itemmap.clear();
	m_nodemap.clear();
	m_Man.Empty();
	m_pCurrentItem = NULL;
	m_pCurrentModel = NULL;
}

void vtFrame::SaveContentsFile(const wxString &fname)
{
	VTLOG("SaveContentsFile '%s'\n", (const char *) fname.mb_str(wxConvUTF8));
	try
	{
		m_Man.WriteXML(fname.mb_str(wxConvUTF8));
	}
	catch (xh_io_exception &e)
	{
		string str = e.getFormattedMessage();
		DisplayMessageBox(wxString(str.c_str(), wxConvUTF8));
	}
}

/// String comparison which considers '/' and '\' equivalent
bool SamePath(const vtString &s1, const vtString &s2)
{
	int i, n = s1.GetLength();
	for (i = 0; i < n; i++)
	{
		if (s1[i] != s2[i] &&
			!(s1[i] == '/' && s2[i] == '\\') &&
			!(s1[i] == '\\' && s2[i] == '/'))
			break;
	}
	return i == n;
}

void vtFrame::AddModelFromFile(const wxString &fname1)
{
	vtString fname = (const char *) fname1.mb_str();
	VTLOG("AddModelFromFile '%s'\n", (const char *) fname);

	// Change backslashes to slashes.
	for (int j = 0; j < fname.GetLength(); j++)
	{
		if (fname.GetAt(j) == '\\')
			fname.SetAt(j, '/');
	}
	// Check if its on the known data path.
	vtStringArray &paths = vtGetDataPath();
	for (unsigned int i = 0; i < paths.size(); i++)
	{
		int n = paths[i].GetLength();
		if (SamePath(paths[i], fname.Left(n)))
		{
			// found it
			fname = fname.Right(fname.GetLength() - n);
			break;
		}
	}

	vtModel *nm = AddModel(wxString(fname, *wxConvCurrent));
	if (nm)
		SetCurrentItemAndModel(m_pCurrentItem, nm);
}

void vtFrame::ModelNameChanged(vtModel *model)
{
	vtTransform *trans = m_nodemap[model];
	if (trans)
		m_nodemap[model] = NULL;

	model->m_attempted_load = false;

	DisplayCurrentModel();

	// update tree view
	m_pTree->RefreshTreeItems(this);

	// update 3d scene graph
	UpdateItemGroup(m_pCurrentItem);
}

int vtFrame::GetModelTriCount(vtModel *model)
{
	vtTransform *trans = m_nodemap[model];
	if (!trans)
		return 0;
	return 0;
}

void vtFrame::OnExit(wxCommandEvent& event)
{
	VTLOG("Got Exit event, shutting down.\n");
	if (m_canvas)
	{
		m_canvas->m_bRunning = false;
		delete m_canvas;
		m_canvas = NULL;
	}
	Destroy();
}

void vtFrame::OnSceneGraph(wxCommandEvent& event)
{
	m_pSceneGraphDlg->Show(true);
}

void vtFrame::OnItemNew(wxCommandEvent& event)
{
	AddNewItem();
	m_pTree->RefreshTreeItems(this);
}

void vtFrame::OnItemDelete(wxCommandEvent& event)
{
	if (!m_pCurrentItem)
		return;

	m_Man.RemoveItem(m_pCurrentItem);
	SetCurrentItemAndModel(NULL, NULL);

	m_pTree->RefreshTreeItems(this);
}

void vtFrame::OnItemAddModel(wxCommandEvent& event)
{
	wxFileDialog loadFile(NULL, _T("Load 3d Model"), _T(""), _T(""),
		_T("All 3D Models (*.3ds, *.flt, *.lwo, *.obj, *.ive, *.osg, *.wrl)|*.3ds;*.flt;*.lwo;*.obj;*.ive;*.osg;*.wrl|")
		_T("3D Studio Files (*.3ds)|*.3ds|")
		_T("OpenFlight Files (*.flt)|*.flt|")
		_T("LightWave Files (*.lwo)|*.lwo|")
		_T("Wavefront Files (*.obj)|*.obj|")
		_T("IVE Files (*.ive)|*.ive|")
		_T("OSG Files (*.osg)|*.osg|")
		_T("VRML Files (*.wrl)|*.wrl|")
		_T("All Files (*.*)|*.*"), wxFD_OPEN);
	loadFile.SetFilterIndex(0);
	if (loadFile.ShowModal() != wxID_OK)
		return;

	AddModelFromFile(loadFile.GetPath());
}

void vtFrame::OnUpdateItemAddModel(wxUpdateUIEvent& event)
{
	event.Enable(m_pCurrentItem != NULL);
}

void vtFrame::OnItemRemoveModel(wxCommandEvent& event)
{
	vtModel *previous = m_pCurrentModel;

	m_pCurrentItem->RemoveModel(m_pCurrentModel);
	SetCurrentItemAndModel(m_pCurrentItem, NULL);

	// update tree view
	m_pTree->RefreshTreeItems(this);

	// update 3d scene graph
	UpdateItemGroup(m_pCurrentItem);

	// free memory
	m_nodemap.erase(previous);
}

void vtFrame::OnItemModelProps(wxCommandEvent& event)
{
	vtModel *mod = m_pCurrentModel;
	osg::Node *node = m_nodemap[mod];
	if (!node)
		return;
	FBox3 box;
	GetNodeBoundBox(node, box);
	wxString str, s;
	s.Printf(_T("Extents:\n  %f %f (width %f)\n  %f %f (height %f)\n  %f %f (depth %f)\n"),
		box.min.x, box.max.x, box.max.x - box.min.x,
		box.min.y, box.max.y, box.max.y - box.min.y,
		box.min.z, box.max.z, box.max.z - box.min.z);
	str += s;

	vtPrimInfo info;
	GetNodePrimCounts(node, info);
	s.Printf(_T("\nPrimitives:\n"));
	str += s;
	s.Printf(_T("  Vertices: %d\n"), info.MemVertices);
	str += s;
	s.Printf(_T("  Vertices Drawn: %d\n"), info.Vertices);
	if (info.Vertices != info.MemVertices) str += s;
	s.Printf(_T("  Primitives: %d\n"), info.Primitives);
	str += s;
	s.Printf(_T("  Points: %d\n"), info.Points);
	if (info.Points) str += s;
	s.Printf(_T("  TriStrips: %d\n"), info.TriStrips);
	if (info.TriStrips) str += s;
	s.Printf(_T("  TriFans: %d\n"), info.TriFans);
	if (info.TriFans) str += s;
	s.Printf(_T("  Triangles: %d\n"), info.Triangles);
	if (info.Triangles) str += s;
	s.Printf(_T("  Quads: %d\n"), info.Quads);
	if (info.Quads) str += s;
	s.Printf(_T("  QuadStrips: %d\n"), info.QuadStrips);
	if (info.QuadStrips) str += s;
	s.Printf(_T("  Polygons: %d\n"), info.Polygons);
	if (info.Polygons) str += s;
	s.Printf(_T("  LineStrips: %d\n"), info.LineStrips);
	if (info.LineStrips) str += s;
	s.Printf(_T("  LineSegments: %d\n"), info.LineSegments);
	if (info.LineSegments) str += s;

	wxMessageBox(str, _T("Model Properties"));
}

void vtFrame::OnUpdateItemModelExists(wxUpdateUIEvent& event)
{
	event.Enable(m_pCurrentItem && m_pCurrentModel);
}

void vtFrame::OnItemRotModel(wxCommandEvent& event)
{
	vtModel *mod = m_pCurrentModel;
	osg::Node *node = m_nodemap[mod];
	// this node is actually the scaling transform; we want its child
	vtTransform *transform = dynamic_cast<vtTransform*>(node);
	if (!transform)
		return;
	osg::Node *node2 = transform->getChild(0);

	ApplyVertexRotation(node2, FPoint3(1,0,0), -PID2f);
}

#include "vtlib/core/vtSOG.h"

vtString GetSaveName(const char *format, const char *wildcard)
{
	wxString msg, filter;

	msg = _("Save ");
	msg += wxString(format, wxConvUTF8);
	filter = wxString(format, wxConvUTF8);
	filter += _(" Files (");
	filter += wxString(wildcard, wxConvUTF8);
	filter += _T(")|");
	filter += wxString(wildcard, wxConvUTF8);

	wxFileDialog saveFile(NULL, msg, _T(""), _T(""), filter, wxFD_SAVE);
	bool bResult = (saveFile.ShowModal() == wxID_OK);
	if (!bResult)
		return vtString("");

	vtString vs = (const char *) saveFile.GetPath().mb_str(wxConvUTF8);
	return vs;
}

void vtFrame::OnItemSaveSOG(wxCommandEvent& event)
{
	vtTransform *trans = m_nodemap[m_pCurrentModel];
	if (!trans)
		return;
	vtGeode *geode = dynamic_cast<vtGeode*>(trans->getChild(0));
	if (!geode)
		return;

	vtString fname = GetSaveName("SOG", "*.sog");
	if (fname == "")
		return;

	OpenProgressDialog(_T("Writing file"), false, this);
	OutputSOG osog;
	FILE *fp = fopen(fname, "wb");
	osog.WriteHeader(fp);
	osog.WriteSingleGeometry(fp, geode);
	fclose(fp);
	CloseProgressDialog();
}

void vtFrame::OnItemSaveOSG(wxCommandEvent& event)
{
	vtTransform *trans = m_nodemap[m_pCurrentModel];
	if (!trans)
		return;
	osg::Node *node = dynamic_cast<osg::Node*>(trans->getChild(0));
	if (!node)
		return;

	vtString fname = GetSaveName("OSG", "*.osg");
	if (fname == "")
		return;

	OpenProgressDialog(_T("Writing file"), false, this);

	// OSG/IVE has a different axis convention that VTLIB does (Z up, not Y up)
	//  So we must rotate before saving, then rotate back again
	ApplyVertexRotation(node, FPoint3(1,0,0), PID2f);

	bool success = vtSaveModel(node, fname);

	// Rotate back again
	ApplyVertexRotation(node, FPoint3(1,0,0), -PID2f);

	CloseProgressDialog();
	if (success)
		wxMessageBox(_("File saved.\n"));
	else
		wxMessageBox(_("Error in writing file.\n"));
}

void vtFrame::OnItemSaveIVE(wxCommandEvent& event)
{
	vtTransform *trans = m_nodemap[m_pCurrentModel];
	if (!trans)
		return;
	osg::Node *node = dynamic_cast<osg::Node*>(trans->getChild(0));
	if (!node)
		return;

	vtString fname = GetSaveName("IVE", "*.ive");
	if (fname == "")
		return;

	OpenProgressDialog(_T("Writing file"), false, this);

	// OSG/IVE has a different axis convention that VTLIB does (Z up, not Y up)
	//  So we must rotate before saving, then rotate back again
	ApplyVertexRotation(node, FPoint3(1,0,0), PID2f);

	bool success = vtSaveModel(node, fname);

	// Rotate back again
	ApplyVertexRotation(node, FPoint3(1,0,0), -PID2f);

	if (success)
		wxMessageBox(_("File saved.\n"));
	else
		wxMessageBox(_("Error in writing file.\n"));
}

void vtFrame::OnUpdateItemSaveSOG(wxUpdateUIEvent& event)
{
	vtTransform *trans;
	vtGeode *geode;

	bool enable = true;
	if (m_pCurrentModel == NULL)
		enable = false;
	if (enable && !(trans = m_nodemap[m_pCurrentModel]))
		enable = false;
	if (enable && !(geode = dynamic_cast<vtGeode*>(trans->getChild(0))))
		enable = false;
	event.Enable(enable);
}

void vtFrame::UpdateWidgets()
{
	if (!m_pCurrentItem)
		return;
	ItemGroup *ig = m_itemmap[m_pCurrentItem];
	if (ig)
	{
		ig->ShowOrigin(m_bShowOrigin);
		ig->ShowRulers(m_bShowRulers);
	}
}

void vtFrame::OnViewOrigin(wxCommandEvent& event)
{
	m_bShowOrigin = !m_bShowOrigin;
	if (m_bShowOrigin)
		m_bShowRulers = false;
	UpdateWidgets();
	m_canvas->Refresh(false);
}

void vtFrame::OnUpdateViewOrigin(wxUpdateUIEvent& event)
{
	event.Check(m_bShowOrigin);
}

void vtFrame::OnViewRulers(wxCommandEvent& event)
{
	m_bShowRulers = !m_bShowRulers;
	if (m_bShowRulers)
		m_bShowOrigin = false;
	UpdateWidgets();
	m_canvas->Refresh(false);
}

void vtFrame::OnUpdateViewRulers(wxUpdateUIEvent& event)
{
	event.Check(m_bShowRulers);
}

void vtFrame::OnViewWireframe(wxCommandEvent& event)
{
	m_bWireframe = !m_bWireframe;
	vtGetScene()->SetGlobalWireframe(m_bWireframe);
	m_canvas->Refresh(false);
}

void vtFrame::OnUpdateViewWireframe(wxUpdateUIEvent& event)
{
	event.Check(m_bWireframe);
}

void vtFrame::OnViewLights(wxCommandEvent& event)
{
	m_pLightDlg->Show(true);
}

void vtFrame::OnHelpAbout(wxCommandEvent& event)
{
	m_canvas->m_bRunning = false;	// stop rendering

	wxString str = _T("VTP Content Manager\n\n");
	str += _T("Manages sources of 3d models for the Virtual Terrain Project software.\n\n");
	str += _T("Please read the HTML documentation and license.\n");
	str += _T("Send feedback to: ben@vterrain.org\n\n");
	str += _T("Build date: ");
	str += _T(__DATE__);
	wxMessageBox(str, _T("About CManager"));

	m_canvas->m_bRunning = true;	// start rendering again
	m_canvas->Refresh(false);
}

//////////////////////////////////////////////////////////////////////////

void vtFrame::AddNewItem()
{
	VTLOG("Creating new Item\n");
	vtItem *pItem = new vtItem;
	pItem->m_name = "untitled";
	m_Man.AddItem(pItem);
	SetCurrentItemAndModel(pItem, NULL);
}

vtModel *vtFrame::AddModel(const wxString &fname_in)
{
	VTLOG("AddModel %s\n", (const char *) fname_in.mb_str());
#if 0
	const char *fname = StartOfFilename(fname_in.mb_str());

	vtString onpath = FindFileOnPaths(vtGetDataPaths(), fname);
	if (onpath == "")
	{
		// Warning!  May not be on the data path.
		wxString str;
		str.Printf(_T("That file:\n%hs\ndoes not appear to be on the data")
			_T(" paths:"), fname);
		for (int i = 0; i < vtGetDataPaths().GetSize(); i++)
		{
			vtString *vts = vtGetDataPaths()[i];
			const char *cpath = (const char *) *vts;
			wxString path = cpath;
			str += _T("\n");
			str += path;
		}
		DisplayMessageBox(str);
		return NULL;
	}
#else
	// data path code is too complicated, just store absolute paths
	vtString fname = (const char *) fname_in.mb_str();
#endif

	// If there is no item, make a new one.
	if (!m_pCurrentItem)
		AddNewItem();

	vtModel *new_model = new vtModel;
	new_model->m_filename = fname;

	osg::Node *node = AttemptLoad(new_model);
	if (!node)
	{
		delete new_model;
		return NULL;
	}

	// add to current item
	m_pCurrentItem->AddModel(new_model);

	// update tree view
	m_pTree->RefreshTreeItems(this);

	// update 3d scene graph
	UpdateItemGroup(m_pCurrentItem);

	return new_model;
}

vtTransform *vtFrame::AttemptLoad(vtModel *model)
{
	VTLOG("AttemptLoad '%s'\n", (const char *) model->m_filename);
	model->m_attempted_load = true;

	// stop rendering while progress dialog is open
	m_canvas->m_bRunning = false;

	OpenProgressDialog(_T("Reading file"), false, this);

	wxString str(model->m_filename, wxConvUTF8);
	UpdateProgressDialog(1, str);

	osg::Node *pNode = NULL;
	vtString fullpath = FindFileOnPaths(vtGetDataPath(), model->m_filename);
	if (fullpath != "")
	{
		UpdateProgressDialog(5, str);
		pNode = vtLoadModel(fullpath);
	}
	CloseProgressDialog();

	// resume rendering after progress dialog is closed
	m_canvas->m_bRunning = true;

	if (!pNode)
	{
		str.Printf(_T("Sorry, couldn't load model from %hs"), (const char *) model->m_filename);
		VTLOG(str.mb_str());
		DisplayMessageBox(str);
		return NULL;
	}
	else
		VTLOG("  Loaded OK.\n");

	// check
	FSphere sphere;
	s2v(pNode->getBound(), sphere);

	// Wrap in a transform node so that we can scale/rotate the node
	vtTransform *pTrans = new vtTransform;
	pTrans->setName("Scaling Transform");
	pTrans->addChild(pNode);

	// Add to map of model -> nodes
	m_nodemap[model] = pTrans;

	UpdateTransform(model);

	return pTrans;
}

void vtFrame::SetCurrentItemAndModel(vtItem *item, vtModel *model)
{
	m_blank->Show(item == NULL && model == NULL);
	m_pModelDlg->Show(item != NULL && model != NULL);
	m_pPropDlg->Show(item != NULL && model == NULL);

	SetCurrentItem(item);
	SetCurrentModel(model);

	if (item != NULL && model == NULL)
	{
		DisplayCurrentItem();
		m_splitter2->ReplaceWindow(m_splitter2->GetWindow2(), m_pPropDlg);
		ZoomToCurrentItem();
	}
	else if (item != NULL && model != NULL)
		m_splitter2->ReplaceWindow(m_splitter2->GetWindow2(), m_pModelDlg);
	else
		m_splitter2->ReplaceWindow(m_splitter2->GetWindow2(), m_blank);
}

void vtFrame::SetCurrentItem(vtItem *item)
{
	VTLOG("SetCurrentItem(%s)\n", item == NULL ? "none" : (const char *) item->m_name);

	if (item == m_pCurrentItem)
		return;

	if (m_pCurrentItem)
		GetItemGroup(m_pCurrentItem)->GetTop()->SetEnabled(false);

	m_pCurrentItem = item;
	m_pCurrentModel = NULL;

	if (item)
	{
		UpdateItemGroup(item);
		m_pPropDlg->SetCurrentItem(item);
	}
	m_pTree->RefreshTreeStatus(this);

	if (m_pCurrentItem)
		GetItemGroup(m_pCurrentItem)->GetTop()->SetEnabled(true);
}

ItemGroup *vtFrame::GetItemGroup(vtItem *item)
{
	ItemGroup *ig = m_itemmap[item];
	if (!ig)
	{
		ig = new ItemGroup(item);
		m_itemmap[item] = ig;
		ig->CreateNodes();

		vtScene *pScene = vtGetScene();
		vtGroup *pRoot = pScene->GetRoot();
		pRoot->addChild(ig->GetTop());
	}
	return ig;
}

void vtFrame::UpdateItemGroup(vtItem *item)
{
	ItemGroup *ig = GetItemGroup(item);
	ig->AttemptToLoadModels();
	ig->AttachModels(m_pFont);
	ig->ShowOrigin(m_bShowOrigin);
	ig->ShowRulers(m_bShowRulers);
	ig->SetRanges();
}

//
// True to show the current item as an LOD'd object
//
void vtFrame::ShowItemGroupLOD(bool bTrue)
{
	if (!m_pCurrentItem)
		return;
	ItemGroup *ig = GetItemGroup(m_pCurrentItem);
	if (ig)
		ig->ShowLOD(bTrue);
}

void vtFrame::SetCurrentModel(vtModel *model)
{
	VTLOG("SetCurrentModel(%s)\n", model == NULL ? "none" : (const char *) model->m_filename);

	if (model == m_pCurrentModel)
		return;

	// 3d scene graph: turn off previous node
	if (m_pCurrentModel)
	{
		vtTransform *trans = m_nodemap[m_pCurrentModel];
		if (trans)
			trans->SetEnabled(false);
	}
	m_pCurrentModel = model;

	// update properties dialog
	m_pModelDlg->SetCurrentModel(model);

	// update 3d scene graph
	if (model)
	{
		DisplayCurrentModel();
		ZoomToCurrentModel();
	}
	// update tree view
	m_pTree->RefreshTreeStatus(this);
}

//
// Update 3d scene graph and 3d view
// also attempt to load any models which have not yet been loaded, and put
// the status to the properties dialog
//
void vtFrame::DisplayCurrentModel()
{
	// show this individual model, not the LOD'd item
	ShowItemGroupLOD(false);

	vtTransform *trans = m_nodemap[m_pCurrentModel];
	if (!trans && !m_pCurrentModel->m_attempted_load)
	{
		trans = AttemptLoad(m_pCurrentModel);
	}
	if (trans)
	{
		trans->SetEnabled(true);
		m_pModelDlg->SetModelStatus("Good");
	}
	else
	{
		m_pModelDlg->SetModelStatus("Failed to load.");
	}
}

void vtFrame::ZoomToCurrentModel()
{
	ZoomToModel(m_pCurrentModel);
}

void vtFrame::ZoomToModel(vtModel *model)
{
	vtTransform *trans = m_nodemap[model];
	if (!trans)
		return;

	vtCamera *pCamera = vtGetScene()->GetCamera();
	float fYon = pCamera->GetFOV();

	FSphere sph;
	trans->GetBoundSphere(sph);

	// consider the origin-center bounding sphere
	float origin_centered = sph.center.Length() + sph.radius;

	// how far back does the camera have to be to see the whole sphere
	float dist = origin_centered / sinf(fYon / 2);

	wxGetApp().m_pTrackball->SetRadius(dist);
	wxGetApp().m_pTrackball->SetZoomScale(sph.radius);
	wxGetApp().m_pTrackball->SetTransScale(sph.radius/2);
	wxGetApp().m_pTrackball->SetTrans(FPoint3(0,0,0));

	pCamera->SetYon(sph.radius * 100.0f);
}

void vtFrame::DisplayCurrentItem()
{
	ShowItemGroupLOD(true);
}

void vtFrame::ZoomToCurrentItem()
{
	if (!m_pCurrentItem)
		return;
	if (m_pCurrentItem->NumModels() < 1)
		return;

	vtModel *model = m_pCurrentItem->GetModel(0);
	if (model)
		ZoomToModel(model);
}

void vtFrame::RefreshTreeItems()
{
	m_pTree->RefreshTreeItems(this);
}

void vtFrame::SetItemName(vtItem *item, const vtString &name)
{
	item->m_name = name;
	m_pPropDlg->SetCurrentItem(item);
}


//////////////////////////////////////////////////////////////////////////

bool DnDFile::OnDropFiles(wxCoord, wxCoord, const wxArrayString& filenames)
{
	size_t nFiles = filenames.GetCount();
	for ( size_t n = 0; n < nFiles; n++ )
	{
		wxString str = filenames[n];
		if (str.Right(4).CmpNoCase(_T("vtco")) == 0)
			GetMainFrame()->LoadContentsFile(str);
		else
			GetMainFrame()->AddModelFromFile(str);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

void vtFrame::OnTestXML(wxCommandEvent& event)
{
#if 0
	vtContentManager Man;
	try {
		Man.ReadXML("content3.vtco");
		Man.WriteXML("content4.vtco");
	}
	catch (xh_io_exception &e)
	{
		string str = e.getFormattedMessage();
		DisplayMessageBox(wxString(str.c_str(), wxConvUTF8));
		return;
	}
#elif 0
	vtImage *image = new vtImage("C:/TEMP/test_transparent.png");

	// Compress
	osg::ref_ptr<osg::State> state = new osg::State;

	// get OpenGL driver to create texture from image.
	vtMaterial::s_bTextureCompression = true;
	vtMaterial *mat = new vtMaterial;
	mat->SetTexture(image);
	mat->m_pTexture->apply(*state);
	image->GetOsgImage()->readImageFromCurrentTexture(0,true);
	osgDB::ReaderWriter::WriteResult wr;
	osgDB::Registry *reg = osgDB::Registry::instance();
	wr = reg->writeImage(*(image->GetOsgImage()), "C:/TEMP/test_transparent.dds");
#endif
}

void vtFrame::DisplayMessageBox(const wxString &str)
{
	m_canvas->m_bRunning = false;
	wxMessageBox(str);
	m_canvas->m_bRunning = true;
	m_canvas->Refresh(false);
}


void vtFrame::UpdateCurrentModelLOD()
{
	// safety
	if (!m_pCurrentItem)
		return;

	ItemGroup *ig = m_itemmap[m_pCurrentItem];
	if (!ig)
		return;

	ig->SetRanges();
}

void vtFrame::UpdateScale(vtModel *model)
{
	UpdateTransform(model);
	UpdateItemGroup(m_pCurrentItem);
}

void vtFrame::UpdateTransform(vtModel *model)
{
	// scale may occasionally be 0 while the user is typing a new value.
	if (model->m_scale == 0.0f)
		return;

	vtTransform *trans = m_nodemap[model];
	if (!trans)
		return;

	trans->Identity();

	vtString ext = GetExtension(model->m_filename, false);
	trans->Scale3(model->m_scale, model->m_scale, model->m_scale);
}

void vtFrame::RenderingPause()
{
	m_canvas->m_bRunning = false;
}

void vtFrame::RenderingResume()
{
	m_canvas->m_bRunning = true;
}

