//
// VTBuilder Helper.h
//
// Copyright (c) 2001-2007 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifndef HELPERH
#define HELPERH

#include "vtdata/Projections.h"
#include "vtdata/MathTypes.h"

/////

void IncreaseRect(wxRect &rect, int adjust);
void DrawRectangle(wxDC* pDC, const wxRect &rect);

/////

void AddType(wxString &str, const wxString &filter);

/////

#if WIN32
/**
 * Win32 allows us to do a real StretchBlt operation, although it still won't
 * do a StretchBlt with a mask.
 */
class wxDC2 : public wxDC
{
public:
	void StretchBlit(const wxBitmap &bmp, wxCoord x, wxCoord y,
		wxCoord width, wxCoord height, wxCoord src_x, wxCoord src_y,
		wxCoord src_width, wxCoord src_height);
};
#endif	// WIN32

/////////////////
class vtMiniDatabuf;
class ImageGLCanvas;
struct TilingOptions;

void WriteMiniImage(const vtString &fname, const TilingOptions &opts,
					unsigned char *rgb_bytes, vtMiniDatabuf &output_buf,
					int iUncompressedSize, ImageGLCanvas *pCanvas);

#if USE_OPENGL
#include "wx/glcanvas.h"
void DoTextureCompress(unsigned char *rgb_bytes, vtMiniDatabuf &output_buf,
					   GLuint &iTex);

//
// ImageGLCanvas class:
//  We need to open an OpenGL context in order to do the texture compression,
//  so we may as well draw something into it, since it requires little extra
//  work, and provides interesting visual feedback to the user.
//
class ImageGLCanvas : public wxGLCanvas
{
public:
	ImageGLCanvas(wxWindow *parent, const wxWindowID id = -1, const wxPoint& pos = wxDefaultPosition,
	  const wxSize& size = wxDefaultSize, long style = 0, const wxString& name = wxT(""),
	  int* gl_attrib = NULL);
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnEraseBackground(wxEraseEvent& event) {}	// Do nothing, to avoid flashing.

	GLuint m_iTex;
protected:
	DECLARE_EVENT_TABLE()
};

#endif	// USE_OPENGL

#if SUPPORT_SQUISH
void DoTextureSquish(unsigned char *rgb_bytes, vtMiniDatabuf &output_buf, bool bFast);
#endif

#endif	// HELPERH

