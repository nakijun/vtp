//
// Name:	 canvas.cpp
// Purpose: Implements the canvas class for the wxSimple application.
//
// Copyright (c) 2001-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

// Headers for the VTP libraries
#include "vtlib/vtlib.h"
#include "vtlib/core/Event.h"
#include "vtdata/vtLog.h"

#include "canvas.h"
#include "frame.h"
#include "app.h"

DECLARE_APP(vtApp);

// Demonstrate how to use the SpaceNavigator
#include "vtlib/core/SpaceNav.h"
vtSpaceNav g_SpaceNav;

/*
 * vtGLCanvas implementation
 */
BEGIN_EVENT_TABLE(vtGLCanvas, wxGLCanvas)
EVT_SIZE(vtGLCanvas::OnSize)
EVT_PAINT(vtGLCanvas::OnPaint)
EVT_CHAR(vtGLCanvas::OnChar)
EVT_MOUSE_EVENTS(vtGLCanvas::OnMouseEvent)
EVT_ERASE_BACKGROUND(vtGLCanvas::OnEraseBackground)
EVT_IDLE(vtGLCanvas::OnIdle)
END_EVENT_TABLE()

vtGLCanvas::vtGLCanvas(wxWindow *parent, wxWindowID id,
	const wxPoint& pos, const wxSize& size, long style, const wxString& name, int* gl_attrib):
wxGLCanvas(parent, id, pos, size, style, name, gl_attrib)
{
	VTLOG("vtGLCanvas constructor\n");

	parent->Show(TRUE);
	SetCurrent();

	wxGLContext *context = GetContext();
	if (context)
		VTLOG("OpenGL context: %lx\n", context);
	else
	{
		VTLOG("No OpenGL context.\n");
		return;
	}
	VTLOG("OpenGL version: %s\n", (const char *) glGetString(GL_VERSION));

	m_bPainting = false;
	m_bRunning = true;

	// Initialize spacenavigator, if there is one present
	g_SpaceNav.Init();
}

vtGLCanvas::~vtGLCanvas(void)
{
}

#if WIN32
WXLRESULT vtGLCanvas::MSWDefWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
	// Catch SpaceNavigator messages; all others pass through
	if (nMsg == WM_INPUT)
		g_SpaceNav.ProcessWM_INPUTEvent(lParam, vtGetScene()->GetCamera());
	return wxWindowMSW::MSWDefWindowProc(nMsg, wParam, lParam);
}
#endif

void vtGLCanvas::OnPaint( wxPaintEvent& event )
{
	// Prevent this function from ever being called nested, it is not re-entrant
	static bool bInside = false;
	if (bInside)
		return;
	bInside = true;

	// place the dc inside a scope, to delete it before the end of function
	if (1)
	{
		// This is a dummy, to avoid an endless succession of paint messages.
		// OnPaint handlers must always create a wxPaintDC.
		wxPaintDC dc(this);

		if (!GetContext() || m_bPainting || !m_bRunning)
		{
			bInside = false;
			return;
		}

		m_bPainting = true;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Updated and render the scene
		vtGetScene()->DoUpdate();

		SwapBuffers();

		// We use refresh-on-idle, so we don't explicitly send ourselves
		//  another Paint message here.

		m_bPainting = false;
	}

	// Must allow some idle processing to occur - or the toolbars will not
	// update, and the close box will not respond!
	wxGetApp().ProcessIdle();

	bInside = false;
}

static void Reshape(int width, int height)
{
	printf("Reshape: %d, %d\n", width, height);
	glViewport(0, 0, (GLint)width, (GLint)height);
}


void vtGLCanvas::OnSize(wxSizeEvent& event)
{
	// Presumably this is a wxMSWism.
	// For wxGTK & wxMotif, all canvas resize events occur before the context
	//   is set.  So ignore this context check and grab the window width/height
	//   when we get it so it (and derived values such as aspect ratio and
	//   viewport parms) are computed correctly.
#ifdef __WXMSW__
	if (!GetContext()) return;
#endif

	SetCurrent();
	int width, height;
	GetClientSize(& width, & height);
	Reshape(width, height);

	vtGetScene()->SetWindowSize(width, height);
}

void vtGLCanvas::OnChar(wxKeyEvent& event)
{
	long key = event.GetKeyCode();

	if ( key == WXK_ESCAPE || key == 'q' || key == 'Q' )
		wxGetApp().GetTopWindow()->Close();

	// pass the char to the vtlib Scene
	vtGetScene()->OnKey(key, 0);
}

void vtGLCanvas::OnMouseEvent(wxMouseEvent& event1)
{
	// turn WX mouse event into a VT mouse event
	vtMouseEvent event;
	wxEventType type = event1.GetEventType();

	if ( type == wxEVT_LEFT_DOWN )
	{
		event.type = VT_DOWN;
		event.button = VT_LEFT;
	}
	else if ( type == wxEVT_LEFT_UP )
	{
		event.type = VT_UP;
		event.button = VT_LEFT;
	}
	else if ( type == wxEVT_MIDDLE_DOWN )
	{
		event.type = VT_DOWN;
		event.button = VT_MIDDLE;
	}
	else if ( type == wxEVT_MIDDLE_UP )
	{
		event.type = VT_UP;
		event.button = VT_MIDDLE;
	}
	else if ( type == wxEVT_RIGHT_DOWN )
	{
		event.type = VT_DOWN;
		event.button = VT_RIGHT;
	}
	else if ( type == wxEVT_RIGHT_UP )
	{
		event.type = VT_UP;
		event.button = VT_RIGHT;
	}
	else if ( type == wxEVT_MOTION )
	{
		event.type = VT_MOVE;
		event.button = VT_NONE;
	}
	else
	{
		// ignore other mouse events, such as wxEVT_LEAVE_WINDOW
		return;
	}

	event.flags = 0;
	wxCoord xpos, ypos;
	event1.GetPosition(&xpos, &ypos);
	event.pos.Set(xpos, ypos);

	if (event1.ControlDown())
		event.flags |= VT_CONTROL;

	if (event1.ShiftDown())
		event.flags |= VT_SHIFT;

	vtGetScene()->OnMouse(event);
}

void vtGLCanvas::OnEraseBackground(wxEraseEvent& event)
{
	// Do nothing, to avoid flashing.
}

void vtGLCanvas::OnIdle(wxIdleEvent &event)
{
	// We use the "Refresh on Idle" approach to continuous rendering.
	if (m_bRunning)
		Refresh(FALSE);
}

