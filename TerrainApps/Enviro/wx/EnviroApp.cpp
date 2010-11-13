//
// Name:	 EnviroApp.cpp
// Purpose:  The application class for our wxWindows application.
//
// Copyright (c) 2001-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/filename.h>
#include <wx/stdpaths.h>

#include "vtlib/vtlib.h"
#include "vtlib/core/Terrain.h"
#include "../Options.h"
#include "EnviroGUI.h"		// for g_App
#include "vtui/Helper.h"	// for LogWindowsVersion, ProgressDialog
#include "vtdata/vtLog.h"
#include "xmlhelper/easyxml.hpp"

#include "EnviroApp.h"
#include "EnviroFrame.h"
#include "canvas.h"
#include "StartupDlg.h"
#include "TParamsDlg.h"

#ifdef USE_OSG_VIEWER
#include "vtui/GraphicsWindowWX.h"
#endif

// Allow customized versions of Enviro to provide their own Frame
#ifdef FRAME_NAME
  #include FRAME_INCLUDE
#else
  #define FRAME_NAME EnviroFrame
  #define STARTUP_DIALOG StartupDlg
  #define LoadAppCatalog(locale)
#endif

IMPLEMENT_APP(EnviroApp)


EnviroApp::EnviroApp()
{
	m_bShowStartupDialog = true;
}

void EnviroApp::Args(int argc, wxChar **argv)
{
	for (int i = 0; i < argc; i++)
	{
		wxString str1 = argv[i];
		vtString str = (const char *) str1.mb_str(wxConvUTF8);
		if (str == "-no_startup_dialog")
			m_bShowStartupDialog = false;
		else if (str.Left(9) == "-terrain=")
			m_bShowStartupDialog = false;
		else if (str.Left(8) == "-locale=")
			m_locale_name = (const char *) str + 8;

		// also let the core application check the command line
		g_App.StartupArgument(i, str);
	}
}


void EnviroApp::SetupLocale()
{
	wxLog::SetVerbose(true);
//	wxLog::AddTraceMask(_T("i18n"));

	// Locale stuff
	int lang = wxLANGUAGE_DEFAULT;
	int default_lang = m_locale.GetSystemLanguage();

	const wxLanguageInfo *info = wxLocale::GetLanguageInfo(default_lang);
	VTLOG("Default language: %d (%s)\n",
		default_lang, (const char *) info->Description.mb_str(wxConvUTF8));

	// After wx2.4.2, wxWidgets looks in the application's directory for
	//  locale catalogs, not the current directory.  Here we force it to
	//  look in the current directory as well.
	wxString cwd = wxGetCwd();
	m_locale.AddCatalogLookupPathPrefix(cwd);

	bool bSuccess=false;
	if (m_locale_name != "")
	{
		VTLOG("Looking up language: %s\n", (const char *) m_locale_name);
		lang = GetLangFromName(wxString(m_locale_name, wxConvUTF8));
		if (lang == wxLANGUAGE_UNKNOWN)
		{
			VTLOG(" Unknown, falling back on default language.\n");
			lang = wxLANGUAGE_DEFAULT;
		}
		else
		{
			info = m_locale.GetLanguageInfo(lang);
			VTLOG("Initializing locale to language %d, Canonical name '%s', Description: '%s':\n",
				lang,
				(const char *) info->CanonicalName.mb_str(wxConvUTF8),
				(const char *) info->Description.mb_str(wxConvUTF8));
			bSuccess = m_locale.Init(lang, wxLOCALE_CONV_ENCODING);
		}
	}
	if (lang == wxLANGUAGE_DEFAULT)
	{
		VTLOG("Initializing locale to default language:\n");
		bSuccess = m_locale.Init(wxLANGUAGE_DEFAULT, wxLOCALE_CONV_ENCODING);
		if (bSuccess)
			lang = default_lang;
	}
	if (bSuccess)
		VTLOG(" succeeded.\n");
	else
		VTLOG(" failed.\n");

	VTLOG("Attempting to load the 'Enviro.mo' catalog for the current locale.\n");
	bSuccess = m_locale.AddCatalog(wxT("Enviro"));
	if (bSuccess)
		VTLOG(" succeeded.\n");
	else
		VTLOG(" not found.\n");
	VTLOG("\n");

	// Load any other catalogs which may be specific to this application.
	LoadAppCatalog(m_locale);

	// Test it
//	wxString test = _("&File");

	wxLog::SetVerbose(false);
}

class LogCatcher : public wxLog
{
	void DoLogString(const wxChar *szString, time_t t)
	{
		VTLOG1(" wxLog: ");
		VTLOG1(szString);
		VTLOG1("\n");
	}
};

//
// Initialize the app object
//
bool EnviroApp::OnInit()
{
#if WIN32 && defined(_MSC_VER) && defined(_DEBUG) && IF_NEEDED
	// sometimes, MSVC seems to need to be told to show unfreed memory on exit
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	StartLog();
	LoadOptions();

	// Redirect the wxWindows log messages to our own logging stream
	wxLog *logger = new LogCatcher;
	wxLog::SetActiveTarget(logger);

	Args(argc, argv);
	SetupLocale();

	// If gdal and proj are not set with environment variables, try to set them
	const char *gdalenv = getenv("GDAL_DATA");
	if (!gdalenv)
		SetEnvironmentVar("GDAL_DATA", "./GDAL-data");
	const char *proj4 = getenv("PROJ_LIB");
	if (!proj4)
		SetEnvironmentVar("PROJ_LIB", "./PROJ4-data");

	CheckForGDALAndWarn();

	// Create and show the Startup Dialog
	if (m_bShowStartupDialog)
	{
		// Look for all terrains on all data paths, so that we have a list
		//  of them even before we call vtlib.
		RefreshTerrainList();

		VTLOG("Opening the Startup dialog.\n");
		wxString appname(STRING_APPNAME, wxConvUTF8);
		appname += _(" Startup");
		STARTUP_DIALOG StartDlg(NULL, -1, appname, wxDefaultPosition);

		StartDlg.GetOptionsFrom(g_Options);
		StartDlg.CenterOnParent();
		int result = StartDlg.ShowModal();
		if (result == wxID_CANCEL)
			return false;

		StartDlg.PutOptionsTo(g_Options);
		g_Options.WriteXML();
	}

	// Now we can create vtTerrain objects for each terrain
	g_App.LoadAllTerrainDescriptions();

	// Load the global content file, if there is one
	g_App.LoadGlobalContent();

	// Create the main frame window
	VTLOG1("Creating the frame window.\n");
	EnviroFrame *frame = CreateMainFrame();
	SetTopWindow(frame);

	// Also let the frame see the command-line arguments
	for (int i = 0; i < argc; i++)
	{
		wxString str = argv[i];
		frame->FrameArgument(i, str.mb_str(wxConvUTF8));
	}

#ifndef __WXMAC__
	bool go = true;
	while (go)
		go = ProcessIdle();
#endif

	g_App.StartControlEngine();

	if (g_Options.m_bFullscreen)
		frame->SetFullScreen(true);

	return true;
}

int EnviroApp::OnExit()
{
	VTLOG("App Exit\n");

	// User might have changed some global options
	g_Options.WriteXML();

	g_App.Shutdown();
	vtGetScene()->Shutdown();

	return wxApp::OnExit();
}

void EnviroApp::StartLog()
{
	g_App.Startup();	// starts log

	VTLOG("Specific application name: %s\n", STRING_APPNAME);
	VTLOG("Application framework: wxWindows v" wxVERSION_NUM_DOT_STRING "\n");
#if WIN32
	VTLOG1(" Running on: ");
	LogWindowsVersion();
#endif
	VTLOG1("Build date: ");
	VTLOG1(__DATE__);
	VTLOG1("\n\n");
}

void EnviroApp::LoadOptions()
{
	// Look these up, we might need them
	wxString Dir1 = wxStandardPaths::Get().GetUserConfigDir();
	wxString Dir2 = wxStandardPaths::Get().GetConfigDir();

	vtString AppDataUser = (const char *) Dir1.mb_str(wxConvUTF8);
	vtString AppDataCommon = (const char *) Dir2.mb_str(wxConvUTF8);

	// Read the vt datapaths
	vtStringArray &dp = vtGetDataPath();
	bool bLoadedDataPaths = vtLoadDataPath(AppDataUser, AppDataCommon);

	// Now look for the Enviro options file.  There are two supported places for it.
	//  1. In the same directory as the executable.
	//  2. On Windows, in the user's "Application Data" folder.
	vtString OptionsFile = STRING_APPNAME ".xml";

#ifndef ENVIRO_NATIVE
	SetupCustomOptions();
#endif

	bool bFound = FileExists(OptionsFile);
	if (!bFound && AppDataUser != "")
	{
		OptionsFile = AppDataUser + "/" + STRING_APPNAME ".xml";
		bFound = FileExists(OptionsFile);
	}
	if (bFound)
	{
		g_Options.ReadXML(OptionsFile);
		g_Options.m_strFilename = OptionsFile;
	}
	else
	{
		// Not found anywhere.  Default to current directory.
		g_Options.m_strFilename = STRING_APPNAME ".xml";
	}

	if (!bLoadedDataPaths)
	{
		if (bFound)
		{
			// We have paths in Enviro.xml, but not in vtp.xml; move them
			dp = g_Options.m_oldDataPaths;
			g_Options.m_oldDataPaths.clear();
		}
		else
		{
			// Set default data path
			dp.push_back(vtString("../Data/"));
			g_Options.m_strContentFile = "common_content.vtco";
		}
		vtSaveDataPath(AppDataUser + "/vtp.xml");
	}

	// Supply the special symbols {appdata} and {appdatacommon}
	for (unsigned int i = 0; i < dp.size(); i++)
	{
		dp[i].Replace("{appdata}", AppDataUser);
		dp[i].Replace("{appdatacommon}", AppDataCommon);
	}

	VTLOG("Using Datapaths:\n");
	int n = dp.size();
	if (n == 0)
		VTLOG("   none.\n");
	for (int d = 0; d < n; d++)
		VTLOG("   %s\n", (const char *) dp[d]);
	VTLOG1("\n");
}

wxString EnviroApp::MakeFrameTitle(vtTerrain *terrain)
{
	wxString title(STRING_APPORG, wxConvUTF8);

	// If we are building the standard VTP, instead of some other flavor, then
	//  put the name of the scene graph library in the title.
	if (!strcmp(STRING_ORGNAME, "VTP"))
	{
	#if VTLIB_PSM
		title += _T(" PSM");
	#elif VTLIB_OSG
		title += _T(" OSG");
	#elif VTLIB_OPENSG
		title += _T(" OpenSG");
	#elif VTLIB_SGL
		title += _T(" SGL");
	#elif VTLIB_SSG
		title += _T(" SSG");
	#endif
	}

	if (terrain)
	{
		title += _T(" - ");
		title += wxString(terrain->GetName(), wxConvUTF8);
	}
	else if (g_App.m_state == AS_Orbit)
	{
		title += _T(" - ");
		title += _("Earth View");
	}
	return title;
}

EnviroFrame *EnviroApp::CreateMainFrame()
{
	wxPoint pos(g_Options.m_WinPos.x, g_Options.m_WinPos.y);
	wxSize size(g_Options.m_WinSize.x, g_Options.m_WinSize.y);
	EnviroFrame *frame = new FRAME_NAME(NULL, MakeFrameTitle(), pos, size);

	// Now we can realize the toolbar
	frame->CreateToolbar();

	// Allow the frame to do something after it's created
	frame->PostConstruction();

	// process some idle messages... let frame open a bit
#ifndef __WXMAC__
	bool go = true;
	while (go)
		go = ProcessIdle();
#endif

	// Initialize the VTP scene
#ifdef USE_OSG_VIEWER
	int MyArgc;
	char** MyArgv;
	ConvertArgcArgv(wxApp::argc, wxApp::argv, &MyArgc, &MyArgv);
	vtGetScene()->Init(MyArgc, MyArgv, g_Options.m_bStereo, g_Options.m_iStereoMode);
	vtGetScene()->SetGraphicsContext(new GraphicsWindowWX(frame->m_canvas, frame->m_canvas->GetGLContext()));
#else
	vtGetScene()->Init(g_Options.m_bStereo, g_Options.m_iStereoMode);
#endif

	// Make sure the scene knows the size of the canvas
	//  (on wxGTK, the first size events arrive too early before the Scene exists)
	wxSize canvas_size = frame->m_canvas->GetClientSize();
	vtGetScene()->SetWindowSize(canvas_size.x, canvas_size.y);

	if (g_Options.m_bLocationInside)
	{
		// they specified the inside (client) location of the window
		// so look at the difference between frame and client sizes
		wxSize size1 = frame->GetSize();
		VTLOG("Frame size %d %d, canvas size %d %d\n", size1.x, size1.y, canvas_size.x, canvas_size.y);
		int dx = size1.x - canvas_size.x;
		int dy = size1.y - canvas_size.y;
		frame->SetSize(-1, -1, size1.x + dx, size1.y + dy);
	}
	return frame;
}

//
// Look for all terrains on all data paths
//
void EnviroApp::RefreshTerrainList()
{
	vtStringArray &paths = vtGetDataPath();

	VTLOG("RefreshTerrainList, %d paths:\n", paths.size());

	terrain_files.clear();
	terrain_paths.clear();
	terrain_names.clear();

	bool bShowProgess = paths.size() > 1;
	if (bShowProgess)
		OpenProgressDialog(_("Scanning data paths for terrains"), false, NULL);
	for (unsigned int i = 0; i < paths.size(); i++)
	{
		vtString directory = paths[i] + "Terrains";

		if (bShowProgess)
			UpdateProgressDialog(i * 100 / paths.size(), wxString(paths[i], wxConvUTF8));

		for (dir_iter it((const char *)directory); it != dir_iter(); ++it)
		{
			if (it.is_hidden() || it.is_directory())
				continue;

			std::string name1 = it.filename();
			vtString name = name1.c_str();

			// only look for terrain parameters files
			vtString ext = GetExtension(name, false);
			if (ext != ".xml")
				continue;

			TParams params;
			vtString path = directory + "/" + name;
			if (params.LoadFrom(path))
			{
				terrain_files.push_back(name);
				terrain_paths.push_back(path);
				terrain_names.push_back(params.GetValueString(STR_NAME));
			}
		}
	}
	VTLOG("RefreshTerrainList done.\n");
	if (bShowProgess)
		CloseProgressDialog();
}

//
// Ask the user to choose from a list of all loaded terrain.
//
bool EnviroApp::AskForTerrainName(wxWindow *pParent, wxString &strTerrainName)
{
	vtTerrainScene *ts = vtGetTS();
	int num = 0, first_idx = 0;
	std::vector<wxString> choices;

	for (unsigned int i = 0; i < ts->NumTerrains(); i++)
	{
		vtTerrain *terr = ts->GetTerrain(i);
		wxString wstr(terr->GetName(), wxConvUTF8);
		choices.push_back(wstr);
		if (wstr.Cmp(strTerrainName) == 0)
			first_idx = num;
		num++;
	}

	if (!num)
	{
		wxMessageBox(_("No terrains found (datapath/Terrains/*.xml)"));
		return false;
	}

	wxSingleChoiceDialog dlg(pParent, _("Please choose a terrain"),
		_("Select Terrain"), num, &(choices.front()));
	dlg.SetSelection(first_idx);

	if (dlg.ShowModal() == wxID_OK)
	{
		strTerrainName = dlg.GetStringSelection();
		return true;
	}
	else
		return false;
}

vtString EnviroApp::GetIniFileForTerrain(const vtString &name)
{
	for (unsigned int i = 0; i < terrain_files.size(); i++)
	{
		if (name == terrain_names[i])
			return terrain_paths[i];
	}
	return vtString("");
}

int EditTerrainParameters(wxWindow *parent, const char *filename)
{
	VTLOG("EditTerrainParameters '%s'\n", filename);

	vtString fname = filename;

	TParamsDlg dlg(parent, -1, _("Terrain Creation Parameters"), wxDefaultPosition);

	TParams Params;
	if (!Params.LoadFrom(fname))
	{
		wxMessageBox(_("Couldn't load from that file."));
		return wxID_CANCEL;
	}
	dlg.SetParams(Params);
	dlg.CenterOnParent();
	int result = dlg.ShowModal();
	if (result == wxID_OK)
	{
		dlg.GetParams(Params);

		vtString ext = GetExtension(fname, false);
		if (ext.CompareNoCase(".ini") == 0)
		{
			wxString str = _("Upgrading the .ini to a .xml file.\n");
			str += _("Deleting old file: ");
			str += wxString(fname, wxConvUTF8);
			wxMessageBox(str);

			// Try to get rid of it.  Hope they aren't on read-only FS.
			vtDeleteFile(fname);

			fname = fname.Left(fname.GetLength()-4) + ".xml";
		}

		if (!Params.WriteToXML(fname, STR_TPARAMS_FORMAT_NAME))
		{
			wxString str;
			str.Printf(_("Couldn't save to file %hs.\n"), (const char *)fname);
			str += _("Please make sure the file is not read-only.");
			wxMessageBox(str);
			result = wxID_CANCEL;
		}
	}
	return result;
}

