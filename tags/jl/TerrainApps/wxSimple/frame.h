//
// Name:		frame.h
//
// Copyright (c) 2001 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifndef FRAMEH
#define FRAMEH

class vtFrame: public wxFrame
{
public:
	vtFrame(wxFrame *frame, const wxString& title, const wxPoint& pos,
		const wxSize& size, long style = wxDEFAULT_FRAME_STYLE);
	~vtFrame();

	// command handlers
	void OnExit(wxCommandEvent& event);

public:
	class vtGLCanvas	*m_canvas;

protected:

DECLARE_EVENT_TABLE()
};

#endif
