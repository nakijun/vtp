//
//  The export functions of the VTBuilder application.
//
// Copyright (c) 2001-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/progdlg.h>

#include "vtdata/ChunkLOD.h"
#include "vtdata/vtDIB.h"
#include "vtdata/vtLog.h"
#include "vtdata/DataPath.h"

#include "vtui/Helper.h"

#include "BuilderView.h"
#include "FileFilters.h"
#include "Builder.h"
#include "Helper.h"
#include "vtBitmap.h"
#include "vtImage.h"
#include "minidata/LocalDatabuf.h"
#include "Options.h"
// Layers
#include "ElevLayer.h"
#include "ImageLayer.h"
// Dialogs
#include "ChunkDlg.h"
#include "RenderDlg.h"
#include "RenderOptionsDlg.h"
#include "TileDlg.h"

#if USE_OPENGL
	#include "wx/glcanvas.h"	// needed for writing pre-compressed textures
#endif

void Builder::ExportASC()
{
	// check spacing
	vtElevationGrid *grid = GetActiveElevLayer()->m_pGrid;
	DPoint2 spacing = grid->GetSpacing();
	double ratio = spacing.x / spacing.y;
	if (ratio < 0.999 || ratio > 1.001)
	{
		wxString str, str2;
		str = _("The Arc ASCII format only supports evenly spaced elevation grids.\n");
		str2.Printf(_("The spacing of this grid is %g x %g\n"), spacing.x, spacing.y);
		str += str2;
		str += _("The result my be stretched.  Do you want to continue anyway?");
		int result = wxMessageBox(str, _("Warning"), wxYES_NO | wxICON_QUESTION, m_pParentWindow);
		if (result != wxYES)
			return;
	}

	vtString fname = GetActiveLayer()->GetExportFilename(FSTRING_ASC);
	if (fname == "")
		return;
	bool success = grid->SaveToASC(fname);
	if (success)
		DisplayAndLog("Successfully wrote file '%s'", (const char *) fname);
	else
		DisplayAndLog("Error writing file.");
}

void Builder::ExportTerragen()
{
	vtString fname = GetActiveLayer()->GetExportFilename(FSTRING_TER);
	if (fname == "")
		return;
	bool success = GetActiveElevLayer()->m_pGrid->SaveToTerragen(fname);
	if (success)
		DisplayAndLog("Successfully wrote file '%s'", (const char *) fname);
	else
		DisplayAndLog("Error writing file.");
}

void Builder::ExportGeoTIFF()
{
	vtString fname = GetActiveLayer()->GetExportFilename(FSTRING_TIF);
	if (fname == "")
		return;
	bool success = GetActiveElevLayer()->m_pGrid->SaveToGeoTIFF(fname);
	if (success)
		DisplayAndLog("Successfully wrote file '%s'", (const char *) fname);
	else
		DisplayAndLog("Error writing file.");
}

void Builder::ExportBMP()
{
	vtString fname = GetActiveLayer()->GetExportFilename(FSTRING_BMP);
	if (fname == "")
		return;
	bool success = GetActiveElevLayer()->m_pGrid->SaveToBMP(fname);
	if (success)
		DisplayAndLog("Successfully wrote file '%s'", (const char *) fname);
	else
		DisplayAndLog("Error writing file.");
}

void Builder::ExportSTM()
{
	vtString fname = GetActiveLayer()->GetExportFilename(FSTRING_STM);
	if (fname == "")
		return;
	bool success = GetActiveElevLayer()->m_pGrid->SaveToSTM(fname);
	if (success)
		DisplayAndLog("Successfully wrote file '%s'", (const char *) fname);
	else
		DisplayAndLog("Error writing file.");
}

void Builder::ExportTIN()
{
	vtString fname = GetActiveLayer()->GetExportFilename(FSTRING_TIN);
	if (fname == "")
		return;
	vtTin2d *tin = new vtTin2d(GetActiveElevLayer()->m_pGrid);
	bool success = tin->Write(fname);
	if (success)
		DisplayAndLog("Successfully wrote file '%s'", (const char *) fname);
	else
		DisplayAndLog("Error writing file.");
	delete tin;
}

void Builder::ExportPlanet()
{
	static wxString default_path = wxEmptyString;

	// ask the user for a directory
	wxDirDialog getDir(m_pParentWindow, _("Export Planet Data to Directory"),
		default_path, wxDD_DEFAULT_STYLE);
	getDir.SetWindowStyle(getDir.GetWindowStyle() | wxDD_NEW_DIR_BUTTON);
	bool bResult = (getDir.ShowModal() == wxID_OK);
	if (!bResult)
		return;

	wxString strDirName = getDir.GetPath();
	default_path = strDirName;	// save it for next time

	if (strDirName == _T(""))
		return;
	bool success = GetActiveElevLayer()->m_pGrid->SaveToPlanet(strDirName.mb_str(wxConvUTF8));
	if (success)
		DisplayAndLog("Successfully wrote Planet dataset to '%s'",
		(const char *) strDirName.mb_str(wxConvUTF8));
	else
		DisplayAndLog("Error writing file.");
}

void Builder::ExportVRML()
{
	vtString fname = GetActiveLayer()->GetExportFilename(FSTRING_WRL);
	if (fname == "")
		return;
	bool success = GetActiveElevLayer()->m_pGrid->SaveToVRML(fname);
	if (success)
		DisplayAndLog("Successfully wrote file '%s'", (const char *) fname);
	else
		DisplayAndLog("Error writing file.");
}

void Builder::ExportXYZ()
{
	vtString fname = GetActiveLayer()->GetExportFilename(FSTRING_TXT);
	if (fname == "")
		return;
	bool success = GetActiveElevLayer()->m_pGrid->SaveToXYZ(fname);
	if (success)
		DisplayAndLog("Successfully wrote file '%s'", (const char *) fname);
	else
		DisplayAndLog("Error writing file.");
}

void Builder::ExportRAWINF()
{
	vtString fname = GetActiveLayer()->GetExportFilename(FSTRING_RAW);
	if (fname == "")
		return;
	bool success = GetActiveElevLayer()->m_pGrid->SaveToRAWINF(fname);
	if (success)
		DisplayAndLog("Successfully wrote file '%s'", (const char *) fname);
	else
		DisplayAndLog("Error writing file.");
}

void Builder::ExportChunkLOD()
{
	// Check dimensions; must be 2^n+1 for .chu
	vtElevationGrid *grid = GetActiveElevLayer()->m_pGrid;
	int x, y;
	grid->GetDimensions(x, y);
	bool good = false;
	for (int i = 3; i < 20; i++)
		if (x == ((1<<i)+1) && y == ((1<<i)+1))
			good = true;
	if (!good)
	{
		DisplayAndLog("The elevation grid has dimensions %d x %d.  They must be\n"
					  "a power of 2 plus 1 for .chu, e.g. 1025x1025.", x, y);
		return;
	}

	// Ask for filename
	vtString fname = GetActiveLayer()->GetExportFilename(FSTRING_CHU);
	if (fname == "")
		return;

	ChunkDlg dlg(m_pParentWindow, -1, _("Export ChunkLOD"));
	dlg.m_iDepth = 6;
	dlg.m_fMaxError = 1.0f;
	if (dlg.ShowModal() == wxID_CANCEL)
		return;

	FILE *out = vtFileOpen(fname, "wb");
	if (out == 0) {
		DisplayAndLog("Error: can't open %s for output.", (const char *) fname);
		return;
	}

	const int CHUNKLOD_MAX_HEIGHT = 10000;
	float vertical_scale = CHUNKLOD_MAX_HEIGHT / 32767.0f;
	float input_vertical_scale = 1.0f;

	OpenProgressDialog(_T("Writing ChunkLOD"), false, m_pParentWindow);

	// Process the data.
	HeightfieldChunker hc;
	bool success = hc.ProcessGrid(grid, out, dlg.m_iDepth, dlg.m_fMaxError,
		vertical_scale, input_vertical_scale, progress_callback);
	fseek(out, 0, SEEK_END);
	g_chunkstats.output_size = ftell(out);
	fclose(out);

	CloseProgressDialog();

	if (success)
	{
		vtString msg, str;
		msg.Format("Successfully wrote file '%s'\n", (const char *) fname);

		float verts_per_chunk = g_chunkstats.output_vertices / (float) g_chunkstats.output_chunks;

		str.Format(" Average verts/chunk: %.0f\n", verts_per_chunk);
		msg += str;
		str.Format(" Output filesize: %dk\n", (int) (g_chunkstats.output_size / 1024.0f));
		msg += str;
		str.Format(" Bytes/input vert: %.2f\n", g_chunkstats.output_size / (float) g_chunkstats.input_vertices);
		msg += str;
		str.Format(" Bytes/output vert: %.2f\n", g_chunkstats.output_size / (float) g_chunkstats.output_vertices);
		msg += str;

		if (verts_per_chunk < 500)
		{
			str.Format("NOTE: verts/chunk is low; for higher poly throughput\nconsider setting depth to %d and reprocessing.\n",
				dlg.m_iDepth - 1);
			msg += "\n";
			msg += str;
		} else if (verts_per_chunk > 5000)
		{
			str.Format("NOTE: verts/chunk is high; for smoother framerate\nconsider setting depth to %d and reprocessing.\n",
				dlg.m_iDepth + 1);
			msg += "\n";
			msg += str;
		}
		DisplayAndLog(msg);
	}
	else
	{
		// TODO: Politely delete the incomplete file?
		if (g_chunkstats.output_most_vertices_per_chunk > (1<<16))
		{
			DisplayAndLog("Error: chunk contains > 64K vertices.  Try processing again, but use\n"\
				"a deeper chunk tree, for fewer vertices per chunk.");
		}
		else
			DisplayAndLog("Error writing file.");
	}
}

void Builder::ExportPNG16()
{
	vtString fname = GetActiveLayer()->GetExportFilename(FSTRING_PNG);
	if (fname == "")
		return;
	bool success = GetActiveElevLayer()->m_pGrid->SaveToPNG16(fname);
	if (success)
		DisplayAndLog("Successfully wrote file '%s'", (const char *) fname);
	else
		DisplayAndLog("Error writing file.");
}

void Builder::Export3TX()
{
	vtElevationGrid *grid = GetActiveElevLayer()->m_pGrid;
	int col, row;
	grid->GetDimensions(col, row);
	if (col != 1201 || row != 1201)
	{
		DisplayAndLog("3TX expects grid dimensions of 1201 x 1201");
		return;
	}
	vtString fname = GetActiveLayer()->GetExportFilename(FSTRING_3TX);
	if (fname == "")
		return;
	bool success = grid->SaveTo3TX(fname);
	if (success)
		DisplayAndLog("Successfully wrote file '%s'", (const char *) fname);
	else
		DisplayAndLog("Error writing file.");
}

void Builder::ElevExportTiles(BuilderView *pView)
{
	vtElevLayer *pEL = GetActiveElevLayer();
	vtElevationGrid *grid = pEL->m_pGrid;
	bool floatmode = (grid->IsFloatMode() || grid->GetScale() != 1.0f);
	DPoint2 spacing = grid->GetSpacing();
	DRECT area = grid->GetEarthExtents();

	TilingOptions tileopts;
	tileopts.cols = 4;
	tileopts.rows = 4;
	tileopts.lod0size = 256;
	tileopts.numlods = 3;

	TileDlg dlg(m_pParentWindow, -1, _("Tiling Options"));
	dlg.m_fEstX = spacing.x;
	dlg.m_fEstY = spacing.y;
	dlg.SetElevation(true);
	dlg.SetArea(area);
	dlg.SetTilingOptions(tileopts);
	dlg.SetView(pView);

	int ret = dlg.ShowModal();
	if (pView)
		pView->HideGridMarks();
	if (ret == wxID_CANCEL)
		return;

	dlg.GetTilingOptions(tileopts);

	// Also write derived image tiles?
	int res = wxMessageBox(_("Also derive and export color-mapped image tiles?"), _("Tiled output"), wxYES_NO | wxCANCEL);
	if (res == wxCANCEL)
		return;
	if (res == wxYES)
	{
		// Ask them where to write the image tiles
		tileopts.bCreateDerivedImages = true;
		wxString filter = FSTRING_INI;
		wxFileDialog saveFile(NULL, _T(".Ini file"), _T(""), _T(""), filter, wxFD_SAVE);
		bool bResult = (saveFile.ShowModal() == wxID_OK);
		if (!bResult)
			return;
		wxString str = saveFile.GetPath();
		tileopts.fname_images = str.mb_str(wxConvUTF8);

		// Ask them how to render the image tiles
		RenderOptionsDlg dlg(m_pParentWindow, -1, _("Rendering options"));
		dlg.SetOptions(tileopts.draw);
		if (dlg.ShowModal() != wxID_OK)
			return;
		dlg.m_opt.m_strColorMapFile = dlg.m_strColorMap.mb_str(wxConvUTF8);
		tileopts.draw = dlg.m_opt;
	}
	else
		tileopts.bCreateDerivedImages = false;

	OpenProgressDialog2(_T("Writing tiles"), true);
	bool success = pEL->WriteGridOfElevTilePyramids(tileopts, pView);
	if (pView)
		pView->HideGridMarks();
	CloseProgressDialog2();
	if (success)
		DisplayAndLog("Successfully wrote to '%s'", (const char *) tileopts.fname);
	else
		DisplayAndLog("Did not successfully write to '%s'", (const char *) tileopts.fname);

	if (tileopts.iNoDataFilled != 0)
		DisplayAndLog("Filled %d unknown heixels in output tiles.", tileopts.iNoDataFilled);
}

void Builder::ExportBitmap(vtElevLayer *pEL, RenderOptions &ropt)
{
	int xsize = ropt.m_iSizeX;
	int ysize = ropt.m_iSizeY;

	ColorMap cmap;
	vtString fname = (const char *) ropt.m_strColorMap.mb_str(wxConvUTF8);
	vtString path = FindFileOnPaths(vtGetDataPath(), "GeoTypical/" + fname);
	if (path == "")
	{
		DisplayAndLog("Couldn't find color map.");
		return;
	}
	if (!cmap.Load(path))
	{
		DisplayAndLog("Couldn't load color map.");
		return;
	}

	// Get attributes of existing layer
	DRECT area;
	vtProjection proj;
	pEL->GetExtent(area);
	pEL->GetProjection(proj);

	// Elevation is pixel-is-point, but Imagery is pixel-is-area, so expand
	//  the area slightly: Imagery is a half-cell larger in each direction.
	DPoint2 spacing = pEL->m_pGrid->GetSpacing();
	area.Grow(spacing.x/2, spacing.y/2);

	vtImageLayer *pOutputLayer = NULL;
	vtBitmapBase *pBitmap = NULL;
	vtDIB dib;

	if (ropt.m_bToFile)
	{
		if (!dib.Create(xsize, ysize, 24))
		{
			DisplayAndLog("Failed to create bitmap.");
			return;
		}
		pBitmap = &dib;
	}
	else
	{
		pOutputLayer = new vtImageLayer(area, xsize, ysize, proj);
		pBitmap = pOutputLayer->GetImage()->GetBitmap();
	}

	pEL->m_pGrid->ColorDibFromElevation(pBitmap, &cmap, 8000,
		ropt.m_ColorNODATA, progress_callback);

	if (ropt.m_bShading)
	{
		if (vtElevLayer::m_draw.m_bShadingQuick)
			pEL->m_pGrid->ShadeQuick(pBitmap, SHADING_BIAS, true, progress_callback);
		else
		{
			// Quick and simple sunlight vector
			FPoint3 light_dir = LightDirection(vtElevLayer::m_draw.m_iCastAngle,
				vtElevLayer::m_draw.m_iCastDirection);

			if (vtElevLayer::m_draw.m_bCastShadows)
				pEL->m_pGrid->ShadowCastDib(pBitmap, light_dir, 1.0f,
					vtElevLayer::m_draw.m_fAmbient, progress_callback);
			else
				pEL->m_pGrid->ShadeDibFromElevation(pBitmap, light_dir, 1.0f,
					vtElevLayer::m_draw.m_fAmbient, vtElevLayer::m_draw.m_fGamma,
					true, progress_callback);
		}
	}

	if (ropt.m_bToFile)
	{
		UpdateProgressDialog(1, _("Writing file"));
		vtString fname = (const char *) ropt.m_strToFile.mb_str(wxConvUTF8);
		bool success;
		if (ropt.m_bJPEG)
			success = dib.WriteJPEG(fname, 99, progress_callback);
		else
			success = dib.WriteTIF(fname, &area, &proj, progress_callback);
		if (success)
			DisplayAndLog("Successfully wrote to file '%s'", (const char *) fname);
		else
			DisplayAndLog("Couldn't open file for writing.");
	}
	else
	{
		AddLayerWithCheck(pOutputLayer);
	}
#if 0
	// TEST - try coloring from water polygons
	int layers = m_Layers.GetSize();
	for (int l = 0; l < layers; l++)
	{
		vtLayer *lp = m_Layers.GetAt(l);
		if (lp->GetType() == LT_WATER)
			((vtWaterLayer*)lp)->PaintDibWithWater(&dib);
	}
#endif
}

void Builder::ImageExportTiles(BuilderView *pView)
{
	VTLOG1("ImageExportTiles:\n");

	vtImageLayer *pIL = GetActiveImageLayer();
	DRECT area;
	pIL->GetExtent(area);
	DPoint2 spacing = pIL->GetSpacing();

	m_tileopts.numlods = 3;
	m_tileopts.bOmitFlatTiles = true;
	m_tileopts.bUseTextureCompression = false;

	TileDlg dlg(m_pParentWindow, -1, _("Tiling Options"));
	dlg.m_fEstX = spacing.x;
	dlg.m_fEstY = spacing.y;
	dlg.SetElevation(false);
	dlg.SetArea(area);
	dlg.SetTilingOptions(m_tileopts);
	dlg.SetView(pView);

	int ret = dlg.ShowModal();
	if (pView)
		pView->HideGridMarks();
	if (ret == wxID_CANCEL)
		return;

	dlg.GetTilingOptions(m_tileopts);

	OpenProgressDialog(_T("Writing tiles"), true);
	bool success = pIL->GetImage()->WriteGridOfTilePyramids(m_tileopts, pView);
	CloseProgressDialog();
	if (success)
		DisplayAndLog("Successfully wrote to '%s'", (const char *) m_tileopts.fname);
	else
		DisplayAndLog("Did not successfully write to '%s'", (const char *) m_tileopts.fname);
}

void Builder::ImageExportPPM()
{
	vtImageLayer *pIL = GetActiveImageLayer();

	vtString fname = pIL->GetExportFilename(FSTRING_PPM);
	if (fname == "")
		return;
	bool success = pIL->GetImage()->WritePPM(fname);
	if (success)
		DisplayAndLog("Successfully wrote file '%s'", (const char *) fname);
	else
		DisplayAndLog("Error writing file.");
}

void Builder::ExportAreaOptimizedElevTileset(BuilderView *pView)
{
	m_tileopts.numlods = 3;

	int count, floating, tins;
	DPoint2 spacing;
	ScanElevationLayers(count, floating, tins, spacing);

	if (count == 0)
	{
		DisplayAndLog("Sorry, you must have some elevation layers\n"
					  "to perform a sampling operation on them.");
		return;
	}

	TileDlg dlg(m_pParentWindow, -1, _("Export Optimized Elevation Tileset"));
	dlg.m_fEstX = spacing.x;
	dlg.m_fEstY = spacing.y;
	dlg.SetElevation(true);
	dlg.SetArea(m_area);
	dlg.SetView(pView);
	m_tileopts.iNoDataFilled = 0;
	dlg.SetTilingOptions(m_tileopts);

	if (dlg.ShowModal() != wxID_OK)
	{
		if (pView)
			pView->HideGridMarks();
		return;
	}
	dlg.GetTilingOptions(m_tileopts);

	// If some of the input grids have floating-point elevation values, ask
	//  the user if they want their resampled output to be floating-point.
	bool bFloat = false;
	if (floating > 0)
	{
		int result = wxMessageBox(_("Sample floating-point elevation values?"),
				_("Question"), wxYES_NO | wxICON_QUESTION, m_pParentWindow);
		if (result == wxYES)
			bFloat = true;
	}

	// Also write derived image tiles?
	int res = wxMessageBox(_("Also derive and export color-mapped image tiles?"), _("Tiled output"), wxYES_NO | wxCANCEL);
	if (res == wxCANCEL)
		return;
	if (res == wxYES)
	{
		// Ask them where to write the image tiles
		m_tileopts.bCreateDerivedImages = true;
		wxString filter = FSTRING_INI;
		wxFileDialog saveFile(NULL, _T(".Ini file"), _T(""), _T(""), filter, wxFD_SAVE);
		bool bResult = (saveFile.ShowModal() == wxID_OK);
		if (!bResult)
			return;
		wxString str = saveFile.GetPath();
		m_tileopts.fname_images = str.mb_str(wxConvUTF8);

		// Ask them how to render the image tiles
		RenderOptionsDlg dlg(m_pParentWindow, -1, _("Rendering options"));
		dlg.SetOptions(m_tileopts.draw);
		if (dlg.ShowModal() != wxID_OK)
			return;
		m_tileopts.draw = dlg.m_opt;
	}
	else
		m_tileopts.bCreateDerivedImages = false;

	bool success = DoSampleElevationToTilePyramids(pView, m_tileopts, bFloat);
	if (success)
		DisplayAndLog("Successfully wrote to '%s'", (const char *) m_tileopts.fname);
	else
		DisplayAndLog("Did not successfully write to '%s'", (const char *) m_tileopts.fname);

	if (m_tileopts.iNoDataFilled != 0)
		DisplayAndLog("Filled %d unknown heixels in output tiles.", m_tileopts.iNoDataFilled);
}

bool Builder::DoSampleElevationToTilePyramids(BuilderView *pView,
											  TilingOptions &opts,
											  bool bFloat, bool bShowGridMarks)
{
	if (m_pParentWindow)
		OpenProgressDialog2(_T("Writing tiles"), true, m_pParentWindow);
	bool success = SampleElevationToTilePyramids(pView, opts, bFloat, bShowGridMarks);
	if (bShowGridMarks && pView)
		pView->HideGridMarks();
	if (m_pParentWindow)
		CloseProgressDialog2();
	return success;
}

void Builder::ExportAreaOptimizedImageTileset(BuilderView *pView)
{
	m_tileopts.numlods = 3;

	DPoint2 spacing(0, 0);
	for (unsigned int i = 0; i < m_Layers.GetSize(); i++)
	{
		vtLayer *l = m_Layers.GetAt(i);
		if (l->GetType() == LT_IMAGE)
		{
			vtImageLayer *im = (vtImageLayer *)l;
			spacing = im->GetSpacing();
		}
	}

	TileDlg dlg(m_pParentWindow, -1, _("Export Optimized Image Tileset"));
	dlg.m_fEstX = spacing.x;
	dlg.m_fEstY = spacing.y;
	dlg.SetElevation(false);
	dlg.SetArea(m_area);
	dlg.SetTilingOptions(m_tileopts);
	dlg.SetView(pView);

	if (dlg.ShowModal() != wxID_OK)
	{
		if (pView)
			pView->HideGridMarks();
		return;
	}
	dlg.GetTilingOptions(m_tileopts);
	m_tileopts.bCreateDerivedImages = false;

	bool success = DoSampleImageryToTilePyramids(pView, m_tileopts);
	if (success)
		DisplayAndLog("Successfully wrote to '%s'", (const char *) m_tileopts.fname);
	else
		DisplayAndLog("Could not successfully write to '%s'", (const char *) m_tileopts.fname);
}

bool Builder::DoSampleImageryToTilePyramids(BuilderView *pView,
											TilingOptions &opts,
											bool bShowGridMarks)
{
	OpenProgressDialog(_T("Writing tiles"), true);
	bool success = SampleImageryToTilePyramids(pView, m_tileopts);
	if (bShowGridMarks && pView)
		pView->HideGridMarks();
	CloseProgressDialog();
	return success;
}

bool Builder::SampleElevationToTilePyramids(BuilderView *pView,
											TilingOptions &opts, bool bFloat,
											bool bShowGridMarks)
{
	VTLOG1("SampleElevationToTilePyramids\n");

	// Avoid trouble with '.' and ',' in Europe
	LocaleWrap normal_numbers(LC_NUMERIC, "C");

	// Check that options are valid
	CheckCompressionMethod(opts);
	bool bJPEG = (opts.bUseTextureCompression && opts.eCompressionType == TC_JPEG);

	// Size of each rectangular tile area
	DPoint2 tile_dim(m_area.Width()/opts.cols, m_area.Height()/opts.rows);

	// Try to create directory to hold the tiles
	vtString dirname = opts.fname;
	RemoveFileExtensions(dirname);
	if (!vtCreateDir(dirname))
		return false;

	// Gather height extents as we produce the tiles
	float minheight = 1E9, maxheight = -1E9;

	ColorMap cmap;
	vtElevLayer::SetupDefaultColors(cmap);	// defaults
	vtString dirname_image = opts.fname_images;
	RemoveFileExtensions(dirname_image);
	if (opts.bCreateDerivedImages)
	{
		if (!vtCreateDir(dirname_image))
			return false;

		vtString cmap_fname = opts.draw.m_strColorMapFile;
		vtString cmap_path = FindFileOnPaths(vtGetDataPath(), "GeoTypical/" + cmap_fname);
		if (cmap_path == "")
			DisplayAndLog("Couldn't find color map.");
		else
		{
			if (!cmap.Load(cmap_path))
				DisplayAndLog("Couldn't load color map.");
		}
	}

	ImageGLCanvas *pCanvas = NULL;
#if USE_OPENGL
	wxFrame *frame = NULL;
	if (opts.bCreateDerivedImages && opts.bUseTextureCompression &&
		opts.eCompressionType == TC_OPENGL)
	{
		frame = new wxFrame;
		frame->Create(m_pParentWindow, -1, _T("Texture Compression OpenGL Context"),
			wxPoint(100,400), wxSize(280, 300), wxCAPTION | wxCLIP_CHILDREN);
		pCanvas = new ImageGLCanvas(frame);
	}
#endif

	// Form an array of pointers to the existing elevation layers
	std::vector<vtElevLayer*> elevs;
	unsigned int elev_layers = ElevLayerArray(elevs);

	// Setup TINs for speedy picking
	unsigned int e;
	for (e = 0; e < elev_layers; e++)
	{
		vtElevLayer *el = elevs[e];
		if (el->m_pTin)
		{
			int tris = el->m_pTin->NumTris();
			// Aim for no more than 50 triangles in a bin
			int bins = (int) sqrt((double) tris / 50);
			if (bins < 10)
				bins = 10;
			UpdateProgressDialog2(1, -1, _T("Binning TIN"));
			el->m_pTin->SetupTriangleBins(bins, progress_callback_minor);
		}
	}

	// make a note of which lods exist
	LODMap lod_existence_map(opts.cols, opts.rows);

	// Pre-build a color lookup table once, rather than for each tile
	std::vector<RGBi> color_table;
	float color_min_elev, color_max_elev;
	if (opts.bCreateDerivedImages)
	{
		ElevLayerArrayRange(elevs, color_min_elev, color_max_elev);
		cmap.GenerateColors(color_table, 4000, color_min_elev, color_max_elev);
	}

	// Time the operation
	clock_t tm1 = clock();

	int i, j;
	int total = opts.rows * opts.cols, done = 0;
	for (j = 0; j < opts.rows; j++)
	{
		for (i = 0; i < opts.cols; i++)
		{
			// We might want to skip certain rows
			if (opts.iMinRow != -1 &&
				(i < opts.iMinCol || i > opts.iMaxCol ||
				 j < opts.iMinRow || j > opts.iMaxRow))
				continue;

			// draw our progress in the main view
			if (bShowGridMarks && pView)
				pView->ShowGridMarks(m_area, opts.cols, opts.rows, i, j);

			DRECT tile_area;
			tile_area.left =	m_area.left + tile_dim.x * i;
			tile_area.right =	m_area.left + tile_dim.x * (i+1);
			tile_area.bottom =	m_area.bottom + tile_dim.y * j;
			tile_area.top =		m_area.bottom + tile_dim.y * (j+1);

			// Look through the elevation layers, determine the highest
			//  resolution available for this tile.
			std::vector<vtElevLayer*> relevant_elevs;
			DPoint2 best_spacing(1E9, 1E9);
			for (e = 0; e < elev_layers; e++)
			{
				DRECT layer_extent;
				elevs[e]->GetExtent(layer_extent);
				if (tile_area.OverlapsRect(layer_extent))
				{
					relevant_elevs.push_back(elevs[e]);

					DPoint2 spacing;
					vtElevationGrid *grid = elevs[e]->m_pGrid;
					if (grid)
						spacing = grid->GetSpacing();
					else
					{
						// In theory a TIN has 'infinite' resolution, but we
						// need to use a fixed value here, so use 1 mm.
						spacing.Set(.001,.001);
					}

					if (spacing.x < best_spacing.x ||
						spacing.y < best_spacing.y)
						best_spacing = spacing;
				}
			}

			// increment progress count whether we omit tile or not
			done++;

			// if there is no data, omit this tile
			if (relevant_elevs.size() == 0)
				continue;

			// If we are paging, don't page out any of the necessary layers
			FlagStickyLayers(relevant_elevs);

			// Estimate what tile resolution is appropriate.
			//  If we can produce a lower resolution, then we can produce fewer lods.
			int total_lods = 1;
			int start_lod = opts.numlods-1;
			int base_tilesize = opts.lod0size >> start_lod;
			float width = tile_area.Width(), height = tile_area.Height();
			while (width / base_tilesize > (best_spacing.x * 1.1) &&	// 10% to avoid roundoff
				   height / base_tilesize > (best_spacing.y * 1.1) &&
				   total_lods < opts.numlods)
			{
			   base_tilesize <<= 1;
			   start_lod--;
			   total_lods++;
			}

			int base_tile_exponent = vt_log2(base_tilesize);
			if( total_lods > base_tile_exponent )
			{
				total_lods = base_tile_exponent;
			}

			int col = i;
			int row = opts.rows-1-j;

			// Now sample the elevation we found to the highest LOD we need
			UpdateProgressDialog2(done*99/total, -1, _("Sampling elevation"));

			vtElevationGrid base_lod(tile_area, base_tilesize+1, base_tilesize+1,
				bFloat, m_proj);

			int iNumInvalid = 0;
			bool bAllInvalid = true;
			bool bAllZero = true;
			DPoint2 p;
			int x, y;
			for (y = base_tilesize; y >= 0; y--)
			{
				p.y = m_area.bottom + (j*tile_dim.y) + ((double)y / base_tilesize * tile_dim.y);

				// Inform user
				progress_callback_minor((base_tilesize-1-y)*99/base_tilesize);

				for (x = 0; x <= base_tilesize; x++)
				{
					p.x = m_area.left + (i*tile_dim.x) + ((double)x / base_tilesize * tile_dim.x);

					float value = ElevLayerArrayValue(relevant_elevs, p);
					base_lod.SetFValue(x, y, value);

					if (value == INVALID_ELEVATION)
						iNumInvalid++;
					else
					{
						bAllInvalid = false;

						// Gather height extents
						if (value < minheight)
							minheight = value;
						if (value > maxheight)
							maxheight = value;

						if (value != 0)
							bAllZero = false;
					}
				}
			}

			// If there is no real data there, omit this tile
			if (bAllInvalid)
				continue;

			// Omit all-zero tiles (flat sea-level) if desired
			if (opts.bOmitFlatTiles && bAllZero)
				continue;

			// Now we know this tile will be included, so note the LODs present
			lod_existence_map.set(col, row, base_tile_exponent, base_tile_exponent-(total_lods-1));

			vtDIB dib;
			if (opts.bCreateDerivedImages && opts.bMaskUnknownAreas)
			{
				dib.Create(base_tilesize, base_tilesize, 32);
				base_lod.ColorDibFromTable(&dib, color_table, color_min_elev, color_max_elev, RGBAi(0,0,0,0));
			}

			if (iNumInvalid > 0)
			{
				// We don't want any gaps at all in the output tiles, because
				//  they will cause huge cliffs.
				UpdateProgressDialog2(done*99/total, -1, _("Filling gaps"));

				bool bGood;
				int method = g_Options.GetValueInt(TAG_GAP_FILL_METHOD);
				if (method == 1)
					bGood = base_lod.FillGaps(NULL, progress_callback_minor);
				else if (method == 2)
					bGood = base_lod.FillGapsSmooth(NULL, progress_callback_minor);
				else if (method == 3)
					bGood = (base_lod.FillGapsByRegionGrowing(2, 5, progress_callback_minor) != -1);
				if (!bGood)
					return false;

				opts.iNoDataFilled += iNumInvalid;
			}

			// Create a matching derived texture tileset
			if (opts.bCreateDerivedImages && !opts.bMaskUnknownAreas)
			{
				dib.Create(base_tilesize, base_tilesize, 24);
				base_lod.ColorDibFromTable(&dib, color_table, color_min_elev, color_max_elev, RGBi(255,0,0));
			}
			if (opts.bCreateDerivedImages)
			{
				if (opts.draw.m_bShadingQuick)
					base_lod.ShadeQuick(&dib, SHADING_BIAS, true);
				else if (opts.draw.m_bShadingDot)
				{
					FPoint3 light_dir = LightDirection(opts.draw.m_iCastAngle,
						opts.draw.m_iCastDirection);

					// Don't cast shadows for tileset; they won't cast
					//  correctly from one tile to the next.
					base_lod.ShadeDibFromElevation(&dib, light_dir, 1.0f,
						opts.draw.m_fAmbient, opts.draw.m_fGamma, true);
				}

				for (int k = 0; k < total_lods; k++)
				{
					vtString fname = MakeFilenameDB(dirname_image, col, row, k);

					int tilesize = base_tilesize >> k;

					vtMiniDatabuf output_buf;

					output_buf.xsize = tilesize;
					output_buf.ysize = tilesize;
					output_buf.zsize = 1;
					output_buf.tsteps = 1;
					output_buf.SetBounds(m_proj, tile_area);

					int depth = dib.GetDepth() / 8;
					int iUncompressedSize = tilesize * tilesize * depth;
					unsigned char *rgb_bytes = (unsigned char *) malloc(iUncompressedSize);

					unsigned char *dst = rgb_bytes;
					if (opts.bMaskUnknownAreas)
					{
						RGBAi rgba;
						for (int ro = 0; ro < base_tilesize; ro += (1<<k))
							for (int co = 0; co < base_tilesize; co += (1<<k))
							{
								dib.GetPixel32(co, ro, rgba);
								*dst++ = rgba.r;
								*dst++ = rgba.g;
								*dst++ = rgba.b;
								*dst++ = rgba.a;
							}
					}
					else
					{
						RGBi rgb;
						for (int ro = 0; ro < base_tilesize; ro += (1<<k))
							for (int co = 0; co < base_tilesize; co += (1<<k))
							{
								dib.GetPixel24(co, ro, rgb);
								*dst++ = rgb.r;
								*dst++ = rgb.g;
								*dst++ = rgb.b;
							}
					}

					// Write and optionally compress the image
					WriteMiniImage(fname, opts, rgb_bytes, output_buf,
						iUncompressedSize, pCanvas);

					// Free the uncompressed image
					free(rgb_bytes);
				}
			}

			for (int k = 0; k < total_lods; k++)
			{
				int lod = start_lod + k;
				int tilesize = base_tilesize >> k;

				vtString fname = MakeFilenameDB(dirname, col, row, k);

				// make a message for the progress dialog
				wxString msg;
				msg.Printf(_("Tile '%hs', size %dx%d"),
					(const char *)fname, tilesize, tilesize);
				bool bCancel = UpdateProgressDialog2(done*99/total, 0, msg);
				if (bCancel)
					return false;

				vtMiniDatabuf buf;
				buf.SetBounds(m_proj, tile_area);
				buf.alloc(tilesize+1, tilesize+1, 1, 1, bFloat ? 2 : 1);
				float *fdata = (float *) buf.data;
				short *sdata = (short *) buf.data;

				for (y = base_tilesize; y >= 0; y -= (1<<k))
				{
					for (x = 0; x <= base_tilesize; x += (1<<k))
					{
						if (bFloat)
						{
							*fdata = base_lod.GetFValue(x, y);
							fdata++;
						}
						else
						{
							*sdata = base_lod.GetValue(x, y);
							sdata++;
						}
					}
				}

#if USE_LIBMINI_DATABUF
				bool saveasPNG=false;
				// libMini can't handle utf8
				vtString fname_local = UTF8ToLocal(fname);
				buf.savedata(fname_local, saveasPNG?2:0); // external format 2=PNG
#else
				buf.savedata(fname);
#endif

				buf.release();
			}
		}
	}

	// Write .ini file
	if (!WriteTilesetHeader(opts.fname, opts.cols, opts.rows, opts.lod0size,
		m_area, m_proj, minheight, maxheight, &lod_existence_map, false))
	{
		vtDestroyDir(dirname);
		return false;
	}

	// Write .ini file for images
	if (opts.bCreateDerivedImages)
		WriteTilesetHeader(opts.fname_images, opts.cols, opts.rows,
			opts.lod0size, m_area, m_proj, INVALID_ELEVATION, INVALID_ELEVATION,
			&lod_existence_map, bJPEG);

#if USE_OPENGL
	if (frame)
	{
		frame->Close();
		delete frame;
	}
#endif

	return true;
}

bool Builder::SampleImageryToTilePyramids(BuilderView *pView, TilingOptions &opts, bool bShowGridMarks)
{
	VTLOG1("SampleImageryToTilePyramids\n");

	// Check that options are valid
	CheckCompressionMethod(opts);
	bool bJPEG = (opts.bUseTextureCompression && opts.eCompressionType == TC_JPEG);

	// Gather array of existing image layers we will sample from
	unsigned int l, layers = m_Layers.GetSize(), num_image = 0;
	vtImageLayer **images = new vtImageLayer *[LayersOfType(LT_IMAGE)];
	for (l = 0; l < layers; l++)
	{
		vtLayer *lp = m_Layers.GetAt(l);
		if (lp->GetType() == LT_IMAGE)
			images[num_image++] = (vtImageLayer *)lp;
	}

	// Avoid trouble with '.' and ',' in Europe
	LocaleWrap normal_numbers(LC_NUMERIC, "C");

	// Size of each rectangular tile area
	DPoint2 tile_dim(m_area.Width()/opts.cols, m_area.Height()/opts.rows);

	// Try to create directory to hold the tiles
	vtString dirname = opts.fname;
	RemoveFileExtensions(dirname);
	if (!vtCreateDir(dirname))
		return false;

	wxFrame *frame = new wxFrame;
	ImageGLCanvas *pCanvas = NULL;
#if USE_OPENGL
	if (opts.bUseTextureCompression && opts.eCompressionType == TC_OPENGL)
	{
		frame->Create(m_pParentWindow, -1, _T("Texture Compression OpenGL Context"),
			wxPoint(100,400), wxSize(280, 300), wxCAPTION | wxCLIP_CHILDREN);
		pCanvas = new ImageGLCanvas(frame);
	}
#endif

	// make a note of which lods exist
	LODMap lod_existence_map(opts.cols, opts.rows);

	// Time the operation
	clock_t tm1 = clock();
	int tiles_written = 0;

	int i, j;
	unsigned int im;
	int total = opts.rows * opts.cols, done = 0;
	for (j = 0; j < opts.rows; j++)
	{
		for (i = 0; i < opts.cols; i++)
		{
			// We might want to skip certain rows
			if (opts.iMinRow != -1 &&
				(i < opts.iMinCol || i > opts.iMaxCol ||
				 j < opts.iMinRow || j > opts.iMaxRow))
				 continue;

			// draw our progress in the main view
			if (bShowGridMarks && pView)
				pView->ShowGridMarks(m_area, opts.cols, opts.rows, i, j);

			DRECT tile_area;
			tile_area.left =	m_area.left + tile_dim.x * i;
			tile_area.right =	m_area.left + tile_dim.x * (i+1);
			tile_area.bottom =	m_area.bottom + tile_dim.y * j;
			tile_area.top =		m_area.bottom + tile_dim.y * (j+1);

			// Look through the image layers to find those which this
			//  tile can sample from.  Determine the highest resolution
			//  available for this tile.
			std::vector<vtImage*> overlapping_images;
			DPoint2 best_spacing(1E9, 1E9);
			int num_source_images = 0;
			for (im = 0; im < num_image; im++)
			{
				DRECT layer_extent;
				images[im]->GetExtent(layer_extent);
				if (tile_area.OverlapsRect(layer_extent))
				{
					num_source_images++;
					vtImage *img = images[im]->GetImage();
					overlapping_images.push_back(img);
					DPoint2 spacing = img->GetSpacing();
					if (spacing.x < best_spacing.x ||
						spacing.y < best_spacing.y)
						best_spacing = spacing;
				}
			}

			// increment progress count whether we omit tile or not
			done++;

			// if there is no data, omit this tile
			if (num_source_images == 0)
				continue;

			// Estimate what tile resolution is appropriate.
			//  If we can produce a lower resolution, then we can produce fewer lods.
			int total_lods = 1;
			int start_lod = opts.numlods-1;
			int base_tilesize = opts.lod0size >> start_lod;
			float width = tile_area.Width(), height = tile_area.Height();
			while (width / base_tilesize > (best_spacing.x * 1.1) &&	// 10% to avoid roundoff
				   height / base_tilesize > (best_spacing.y * 1.1) &&
				   total_lods < opts.numlods)
			{
			   base_tilesize <<= 1;
			   start_lod--;
			   total_lods++;
			}

			int col = i;
			int row = opts.rows-1-j;

			// Now we know this tile will be included, so note the LODs present
			int base_tile_exponent = vt_log2(base_tilesize);
			lod_existence_map.set(col, row, base_tile_exponent, base_tile_exponent-(total_lods-1));

			for (int k = 0; k < total_lods; k++)
			{
				int lod = start_lod + k;
				int tilesize = base_tilesize >> k;

				// The output image area is 1/2 texel larger than the tile area
				DPoint2 texel = tile_dim / (tilesize-1);
				DRECT image_area = tile_area;
				image_area.Grow(texel.x/2, texel.y/2);

				// Sample the images we found to the exact LOD we need
				vtBitmap Target;
				Target.Allocate(tilesize, tilesize);

				// Get ready to multisample
				DPoint2 step = tile_dim / (tilesize-1);
				DLine2 offsets;
				int iNSampling = g_Options.GetValueInt(TAG_SAMPLING_N);
				MakeSampleOffsets(step, iNSampling, offsets);
				double dRes = (step.x+step.y)/2;

				DPoint2 p;
				RGBi pixel, rgb;
				for (int y = tilesize-1; y >= 0; y--)
				{
					p.y = tile_area.bottom + (y * step.y);
					for (int x = 0; x < tilesize; x++)
					{
						p.x = tile_area.left + (x * step.x);

						// find some data for this point
						rgb.Set(0,0,0);
						for (unsigned int im = 0; im < overlapping_images.size(); im++)
							if (overlapping_images[im]->GetMultiSample(p, offsets, pixel, dRes)) rgb = pixel;

						Target.SetPixel24(x, y, rgb);
					}
				}

				vtString fname = MakeFilenameDB(dirname, col, row, k);

				// make a message for the progress dialog
				wxString msg;
				msg.Printf(_("Tile '%hs', size %dx%d"),
					(const char *)fname, tilesize, tilesize);
				bool bCancel = UpdateProgressDialog(done*99/total, msg);
				if (bCancel)
					return false;

				unsigned char *rgb_bytes = (unsigned char *) malloc(tilesize * tilesize * 3);
				int cb = 0;	// count bytes

				// Copy whole image directly from Target to rgb_bytes
				for (int y = tilesize-1; y >= 0; y--)
				{
					for (int x = 0; x < tilesize; x++)
					{
						Target.GetPixel24(x, y, rgb);
						rgb_bytes[cb++] = rgb.r;
						rgb_bytes[cb++] = rgb.g;
						rgb_bytes[cb++] = rgb.b;
					}
				}
				int iUncompressedSize = cb;

				vtMiniDatabuf output_buf;

				output_buf.xsize = tilesize;
				output_buf.ysize = tilesize;

				output_buf.zsize = 1;
				output_buf.tsteps = 1;
				output_buf.SetBounds(m_proj, image_area);

				// Write and optionally compress the image
				WriteMiniImage(fname, opts, rgb_bytes, output_buf,
					iUncompressedSize, pCanvas);

				// Free the uncompressed image
				free(rgb_bytes);

				tiles_written ++;
			}
		}
	}

	// Write .ini file
	WriteTilesetHeader(opts.fname, opts.cols, opts.rows, opts.lod0size,
		m_area, m_proj, INVALID_ELEVATION, INVALID_ELEVATION, &lod_existence_map, bJPEG);

	clock_t tm2 = clock();
	float elapsed = ((float)tm2 - tm1)/CLOCKS_PER_SEC;
	wxString str;
	str.Printf(_("Wrote %d tiles (%d cells) in %.1f seconds (%.2f seconds per cell)"),
		tiles_written, (opts.rows * opts.cols), elapsed, elapsed/(opts.rows * opts.cols));
	VTLOG1(str.mb_str(wxConvUTF8));
	VTLOG1("\n");
	wxMessageBox(str);

	// Statistics
	for (im = 0; im < num_image; im++)
	{
		LineBufferGDAL &buf = images[im]->GetImage()->m_linebuf;
		VTLOG(" Image %d (size %d x %d): %d line reads, %d block reads\n", im,
			buf.m_iXSize, buf.m_iYSize, buf.m_linereads, buf.m_blockreads);
	}

	if (frame)
	{
		frame->Close();
		delete frame;
	}
	return true;
}
