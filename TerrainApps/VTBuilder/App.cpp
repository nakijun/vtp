//
// App.cpp - Main application class for VTBuilder
//
// Copyright (c) 2001-2011 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "Frame.h"
#include "BuilderView.h"
#include "vtui/Helper.h"
#include "vtui/LogCatcher.h"
#include "gdal_priv.h"
#include "App.h"

#define HEAPBUSTER 0

#if HEAPBUSTER
#include "../HeapBuster/HeapBuster.h"
#endif

IMPLEMENT_APP(BuilderApp)


void BuilderApp::Args(int argc, wxChar **argv)
{
	for (int i = 0; i < argc; i++)
	{
		wxString str = argv[i];
		wxCharBuffer cbuf = str.mb_str(wxConvUTF8);
		if (!strncmp(cbuf, "-locale=", 8))
			m_locale_name = (const char *)cbuf + 8;
	}
}

bool BuilderApp::OnInit()
{
#if WIN32 && defined(_MSC_VER) && DEBUG
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	VTSTARTLOG("debug.txt");
	VTLOG1("VTBuilder");
	VTLOG1("\nBuild:");
#if VTDEBUG
	VTLOG(" Debug");
#else
	VTLOG(" Release");
#endif
#ifdef UNICODE
	VTLOG(" Unicode");
#endif
	VTLOG("\n");
#if WIN32
	VTLOG(" Running on: ");
	LogWindowsVersion();
#endif
	VTLOG1("Build date: ");
	VTLOG1(__DATE__);
	VTLOG1("\n\n");

#if WIN32
	// To work around the _wfopen/gzclose problem, increase the limit on number of open files
	int newmax = _setmaxstdio(2048);
	VTLOG("_setmaxstdio to %d\n", newmax);
#endif

	// Redirect the wxWidgets log messages to our own logging stream
	wxLog *logger = new LogCatcher;
	wxLog::SetActiveTarget(logger);
	wxLog::SetLogLevel(wxLOG_User);
	wxLog::SetVerbose();
	wxLogVerbose("wxWidgets logging enabled");

	Args(argc, argv);

	SetupLocale("VTBuilder");

	VTLOG1("Testing ability to allocate a frame object.\n");
	wxFrame *frametest = new wxFrame(NULL, -1, _T("Title"));
	delete frametest;

	wxString title("VTBuilder", wxConvUTF8);
	VTLOG(" Creating Main Frame Window, title '%s'\n", (const char *) title.mb_str(wxConvUTF8));
	MainFrame *frame = new MainFrame((wxFrame *) NULL, title,
							   wxPoint(50, 50), wxSize(900, 500));

	VTLOG1(" Setting up the UI.\n");
	frame->SetupUI();

	VTLOG1(" Showing the frame.\n");
	frame->Show(TRUE);

	SetTopWindow(frame);

	VTLOG1(" GDAL-supported formats:");
	GDALDriverManager *poDM = GetGDALDriverManager();
	for( int iDriver = 0; iDriver < poDM->GetDriverCount(); iDriver++ )
	{
		if ((iDriver % 13) == 0)
			VTLOG1("\n  ");
		GDALDriver *poDriver = poDM->GetDriver( iDriver );
		const char *name = poDriver->GetDescription();
		VTLOG(" %s", name);
	}
	VTLOG1("\n");

#if HEAPBUSTER
	// Pull in the heap buster
	g_HeapBusterDummy = -1;
#endif

	return TRUE;
}

int BuilderApp::OnExit()
{
	VTLOG("App Exit\n");
	return wxApp::OnExit();
}

