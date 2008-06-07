//
// Builder.cpp: The main Builder class of the VTBuilder
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

#include "vtdata/vtLog.h"
#include "vtdata/DataPath.h"
#include <float.h>	// for FLT_MIN

#include "Builder.h"
#include "Helper.h"
#include "VegGenOptions.h"
#include "vtImage.h"
#include "Options.h"

#include "vtui/Helper.h"

// Layers
#include "ElevLayer.h"
#include "ImageLayer.h"
#include "RawLayer.h"
#include "RoadLayer.h"
#include "StructLayer.h"
#include "UtilityLayer.h"
#include "VegLayer.h"

// Dialogs
#include "OptionsDlg.h"
#include "ResampleDlg.h"
#include "SampleImageDlg.h"
#include "vtui/ProjectionDlg.h"

/** This singleton contains all the global options for the whole application. */
vtTagArray g_Options;

/** Global pointer to builder object, of which there is only one */
Builder *g_bld;

//////////////////////////////////////////////////////////////////
// Builder constructor
//
Builder::Builder()
{
	VTLOG("  Builder constructor: enter\n");

	// init app data
	m_pActiveLayer = NULL;
	m_bAdoptFirstCRS = true;
	m_pParentWindow = NULL;
	m_LSOptions.Defaults();
	m_pInstanceDlg = NULL;
	m_bDrawDisabled = false;

	m_tileopts.cols = 4;
	m_tileopts.rows = 4;
	m_tileopts.numlods = 3;
	m_tileopts.lod0size = 512;
	m_tileopts.bCreateDerivedImages = false;
	m_tileopts.bMaskUnknownAreas = false;
	m_tileopts.bUseTextureCompression = true;
	m_tileopts.eCompressionType = TC_OPENGL;

	g_bld = this;

	// Get datapaths from the vtp.xml config file
	ReadDataPath();
	VTLOG("Using Datapaths:\n");
	int i, n = (int)vtGetDataPath().size();
	if (n == 0)
		VTLOG("   none.\n");
	for (i = 0; i < n; i++)
		VTLOG("   %s\n", (const char *) vtGetDataPath()[i]);

	VTLOG1(" Initializing GDAL.\n");
	CheckForGDALAndWarn();
	g_GDALWrapper.RequestGDALFormats();

	// set up the datum list we will use
	SetupEPSGDatums();

	// Fill list of layer type names
	if (vtLayer::LayerTypeNames.IsEmpty())
	{
		// These must correspond to the order of the LayerType enum!
		vtLayer::LayerTypeNames.Add(_("Raw"));
		vtLayer::LayerTypeNames.Add(_("Elevation"));
		vtLayer::LayerTypeNames.Add(_("Image"));
		vtLayer::LayerTypeNames.Add(_("Road"));
		vtLayer::LayerTypeNames.Add(_("Structure"));
		vtLayer::LayerTypeNames.Add(_("Water"));
		vtLayer::LayerTypeNames.Add(_("Vegetation"));
		vtLayer::LayerTypeNames.Add(_("Utility"));
#if SUPPORT_TRANSIT
		vtLayer::LayerTypeNames.Add(_("Transit"));
#endif
	}

	VTLOG1("  Builder constructor: exit\n");
}

Builder::~Builder()
{
	VTLOG1("Builder destructor\n");
	DeleteContents();
}

void Builder::DeleteContents()
{
	m_Layers.Empty();
	m_pActiveLayer = NULL;
	FreeGlobalMaterials();
	FreeContentFiles();
}


////////////////////////////////////////////////////////////////
// Project Methods

void trim_eol(char *buf)
{
	int len = (int)strlen(buf);
	if (len && buf[len-1] == 10) buf[len-1] = 0;
	len = (int)strlen(buf);
	if (len && buf[len-1] == 13) buf[len-1] = 0;
}

bool Builder::LoadProject(const vtString &fname, vtScaledView *pView)
{
	// Avoid trouble with '.' and ',' in Europe
	LocaleWrap normal_numbers(LC_NUMERIC, "C");

	VTLOG1("LoadProject()\n");

	// read project file
	FILE *fp = vtFileOpen(fname, "rb");
	if (!fp)
	{
		DisplayAndLog("Couldn't open project file: '%s'", (const char *) fname);
		return false;
	}

	// even the first layer must match the project's CRS
	m_bAdoptFirstCRS = false;

	char buf[2000];
	while (fgets(buf, 2000, fp) != NULL)
	{
		if (!strncmp(buf, "Projection ", 11))
		{
			// read projection info
			vtProjection proj;
			char *wkt = buf + 11;
			OGRErr err = proj.importFromWkt(&wkt);
			if (err != OGRERR_NONE)
			{
				DisplayAndLog("Had trouble parsing the projection information "
					"from that file.");
				fclose(fp);
				return false;
			}
			SetProjection(proj);
		}
		if (!strncmp(buf, "PlantList ", 10))
		{
			trim_eol(buf);
			LoadSpeciesFile(buf+10);
		}
		if (!strncmp(buf, "BioTypes ", 9))
		{
			trim_eol(buf);
			LoadBiotypesFile(buf+9);
		}
		if (!strncmp(buf, "area ", 5))
		{
			sscanf(buf+5, "%lf %lf %lf %lf\n", &m_area.left, &m_area.top,
				&m_area.right, &m_area.bottom);
			VTLOG(" area: %lf %lf %lf %lf\n", m_area.left, m_area.top,
				m_area.right, m_area.bottom);
		}
		if (!strncmp(buf, "view ", 5))
		{
			DRECT rect;
			sscanf(buf+5, "%lf %lf %lf %lf\n", &rect.left, &rect.top,
				&rect.right, &rect.bottom);
			if (pView)
				pView->ZoomToRect(rect, 0.0f);
		}
		if (!strncmp(buf, "layers", 6))
		{
			int count = 0;
			LayerType ltype;

			sscanf(buf+7, "%d\n", &count);
			for (int i = 0; i < count; i++)
			{
				bool bShow = true, bImport = false;

				char buf2[200], buf3[200];
				fgets(buf, 200, fp);
				int num = sscanf(buf, "type %d, %s %s", &ltype, buf2, buf3);

				if (!strcmp(buf2, "import"))
					bImport = true;
				if (num > 2 && !strcmp(buf3, "hidden"))
					bShow = false;

				// next line is the path
				fgets(buf, 200, fp);

				// trim trailing LF character
				trim_eol(buf);
				vtString path = buf;

				// If there is a relative path, allow it to be relative to the
				//  location of the project file.
				if (path.Left(1) == ".")
				{
					path = ExtractPath(fname, true);
					path += buf;
				}
				wxString layer_fname(path, wxConvUTF8);

				int numlayers = NumLayers();
				if (bImport)
					ImportDataFromArchive(ltype, layer_fname, false);
				else
				{
					vtLayer *lp = vtLayer::CreateNewLayer(ltype);
					if (lp && lp->Load(layer_fname))
						AddLayer(lp);
					else
						delete lp;
				}

				// Hide any layers created, if desired
				int newlayers = NumLayers();
				for (int j = numlayers; j < newlayers; j++)
					GetLayer(j)->SetVisible(bShow);
			}
		}
	}
	fclose(fp);

	// reset to default behavior
	m_bAdoptFirstCRS = true;

	VTLOG1(" LoadProject done.\n");
	return true;
}


////////////////////////////////////////////////////////////////
// Layer Methods

void Builder::AddLayer(vtLayer *lp)
{
	m_Layers.Append(lp);
}

bool Builder::AddLayerWithCheck(vtLayer *pLayer, bool bRefresh)
{
	VTLOG("AddLayerWithCheck(%lx)\n", pLayer);

	vtProjection proj;
	pLayer->GetProjection(proj);

	bool bFirst = (m_Layers.GetSize() == 0);
	if (bFirst && m_bAdoptFirstCRS)
	{
		// if this is our first layer, adopt its projection
		VTLOG1("  Adopting CRS from this layer.\n");
		SetProjection(proj);
	}
	else
	{
		// check for Projection conflict
		VTLOG1("  Checking CRS compatibility.\n");
		if (!(m_proj == proj))
		{
			int ret;
			bool keep = false;
			if (IsGUIApp())
			{
				char *str1, *str2;
				m_proj.exportToProj4(&str1);
				proj.exportToProj4(&str2);

				wxString msg;
				msg.Printf(_("The data already loaded is in:\n   %hs\n but the layer you are attempting to add:\n   %s\n is using:\n   %hs\n Would you like to attempt to convert it now to the existing projection?"),
					str1, pLayer->GetLayerFilename().c_str(), str2);
				OGRFree(str1);
				OGRFree(str2);
				ret = wxMessageBox(msg, _("Convert Coordinate System?"), wxYES_NO | wxCANCEL);
				if (ret == wxNO)
					keep = true;
			}
			else
			{
				// If there is no GUI, we must always assume they want to convert
				ret = wxYES;
			}
			if (ret == wxYES)
			{
				VTLOG1("  Reprojecting..\n");
				OpenProgressDialog(_("Reprojecting"), false, m_pParentWindow);
				bool success = pLayer->TransformCoords(m_proj);
				CloseProgressDialog();

				if (success)
					keep = true;
				else
				{
					if (IsGUIApp())
					{
						ret = wxMessageBox(_("Couldn't convert projection.\n Proceed anyway?"),
							_("Warning"), wxYES_NO);
						if (ret == wxYES)
							keep = true;
					}
				}
			}
			if (!keep)
				return false;
		}
	}
	AddLayer(pLayer);
	SetActiveLayer(pLayer, false);
	return true;
}

void Builder::RemoveLayer(vtLayer *lp)
{
	if (!lp)
		return;

	// check the type of the layer we're deleting
	LayerType lt = lp->GetType();

	// remove and delete the layer
	m_Layers.RemoveAt(m_Layers.Find(lp));

	// if it was the active layer, select another layer of the same type
	if (GetActiveLayer() == lp)
	{
		vtLayer *lp_new = FindLayerOfType(lt);
		SetActiveLayer(lp_new, true);
	}
	DeleteLayer(lp);
}

void Builder::DeleteLayer(vtLayer *lp)
{
	delete lp;
}

void Builder::SetActiveLayer(vtLayer *lp, bool refresh)
{
	m_pActiveLayer = lp;
}

//
// Returns the number of layers present of a given type.
//
int Builder::LayersOfType(LayerType lt)
{
	int count = 0;
	int layers = m_Layers.GetSize();
	for (int l = 0; l < layers; l++)
	{
		if (m_Layers.GetAt(l)->GetType() == lt)
			count++;
	}
	return count;
}

int Builder::NumModifiedLayers()
{
	int count = 0;
	int layers = m_Layers.GetSize();
	for (int l = 0; l < layers; l++)
	{
		vtLayer *lp = m_Layers[l];
		if (lp->GetModified() && lp->CanBeSaved())
			count++;
	}
	return count;
}

vtLayer *Builder::FindLayerOfType(LayerType lt)
{
	int layers = m_Layers.GetSize();
	for (int l = 0; l < layers; l++)
	{
		vtLayer *lp = m_Layers.GetAt(l);
		if (lp->GetType() == lt)
			return lp;
	}
	return NULL;
}

int Builder::LayerNum(vtLayer *lp)
{
	int layers = m_Layers.GetSize();
	for (int i = 0; i < layers; i++)
		if (lp == m_Layers[i])
			return i;
	return -1;
}

void Builder::SwapLayerOrder(int n0, int n1)
{
	vtLayer *lp0 = m_Layers[n0];
	vtLayer *lp1 = m_Layers[n1];
	m_Layers[n0] = lp1;
	m_Layers[n1] = lp0;
}

LayerType Builder::AskLayerType()
{
	wxString choices[LAYER_TYPES];
	for (int i = 0; i < LAYER_TYPES; i++)
		choices[i] = vtLayer::LayerTypeNames[i];

	int n = LAYER_TYPES;
	static int cur_type = 0;	// remember the choice for next time

	wxSingleChoiceDialog dialog(m_pParentWindow, _("These are your choices"),
		_("Please indicate layer type"), n, (const wxString *)choices);

	dialog.SetSelection(cur_type);

	if (dialog.ShowModal() == wxID_OK)
	{
		cur_type = dialog.GetSelection();
		return (LayerType) cur_type;
	}
	else
		return LT_UNKNOWN;
}

void Builder::CheckOptionBounds()
{
	// Check the preferences, make sure they have acceptable values.
	if (g_Options.GetValueInt(TAG_SAMPLING_N) < 1)
		g_Options.SetValueInt(TAG_SAMPLING_N, 1);
	if (g_Options.GetValueInt(TAG_SAMPLING_N) > 32)
		g_Options.SetValueInt(TAG_SAMPLING_N, 32);

	int mp;
	if (!g_Options.GetValueInt(TAG_MAX_MEGAPIXELS, mp))
		mp = 16;
	if (mp < 0) mp = 0;
	if (mp > 300) mp = 300;
	g_Options.SetValueInt(TAG_MAX_MEGAPIXELS, mp);

	int ems;
	if (!g_Options.GetValueInt(TAG_ELEV_MAX_SIZE, ems))
		ems = 4096;
	if (ems < 16) ems = 16;
	if (ems > 8192) ems = 8192;
	g_Options.SetValueInt(TAG_ELEV_MAX_SIZE, ems);

	if (!g_Options.FindTag(TAG_TIFF_COMPRESS))
		g_Options.SetValueBool(TAG_TIFF_COMPRESS, false, true);

	// deal with deprecated tag
	if (g_Options.FindTag(TAG_SLOW_FILL_GAPS))
	{
		bool bSlow = g_Options.GetValueBool(TAG_SLOW_FILL_GAPS);
		g_Options.RemoveTag(TAG_SLOW_FILL_GAPS);
		if (bSlow)
			g_Options.SetValueInt(TAG_GAP_FILL_METHOD, 2, true);
		else
			g_Options.SetValueInt(TAG_GAP_FILL_METHOD, 1, true);
	}

	if (!g_Options.FindTag(TAG_GAP_FILL_METHOD))
		g_Options.SetValueInt(TAG_GAP_FILL_METHOD, 1, true);
}

DRECT Builder::GetExtents()
{
	DRECT rect(1E9,-1E9,-1E9,1E9);

	bool has_bounds = false;

	// Acculumate the extents of all the layers
	DRECT rect2;
	int iLayers = m_Layers.GetSize();

	for (int i = 0; i < iLayers; i++)
	{
		if (m_Layers.GetAt(i)->GetExtent(rect2))
		{
			rect.GrowToContainRect(rect2);
			has_bounds = true;
		}
	}
	if (has_bounds)
		return rect;
	else if (m_proj.IsDymaxion())
	{
		return DRECT(0, 1.5*sqrt(3.0), 5.5, 0);
	}
	else
		return DRECT(-180,90,180,-90);	// geo extents of whole planet
}

//
// Pick a point, in geographic coords, which is roughly in the middle
//  of the data that the user is working with.
//
DPoint2 Builder::EstimateGeoDataCenter()
{
	DRECT rect = GetExtents();
	DPoint2 pos = rect.GetCenter();

	if (!m_proj.IsGeographic())
	{
		vtProjection geo;
		CreateSimilarGeographicProjection(m_proj, geo);
		OCT *trans = CreateConversionIgnoringDatum(&m_proj, &geo);
		if (trans)
			trans->Transform(1, &pos.x, &pos.y);
		delete trans;
	}
	return pos;
}

void Builder::LookForContentFiles()
{
	VTLOG1("Searching data paths for content files (.vtco)\n");
	for (unsigned int i = 0; i < vtGetDataPath().size(); i++)
	{
		vtStringArray fnames;
		AddFilenamesToStringArray(fnames, vtGetDataPath()[i], "*.vtco");

		for (unsigned int j = 0; j < fnames.size(); j++)
		{
			vtString path = vtGetDataPath()[i];
			path += fnames[j];

			bool success = true;
			vtContentManager *mng = new vtContentManager;
			try
			{
				mng->ReadXML(path);
			}
			catch (xh_io_exception &ex)
			{
				// display (or at least log) error message here
				VTLOG("XML error:");
				VTLOG(ex.getFormattedMessage().c_str());
				success = false;
				delete mng;
			}
			if (success)
				m_contents.push_back(mng);
		}
	}
	VTLOG(" found %d files on %d paths\n", m_contents.size(), vtGetDataPath().size());
}

void Builder::FreeContentFiles()
{
	for (unsigned int i = 0; i < m_contents.size(); i++)
		delete m_contents[i];
	m_contents.clear();
}

void Builder::ResolveInstanceItem(vtStructInstance *inst)
{
	vtString name;
	if (!inst->GetValueString("itemname", name))
		return;
	for (unsigned int j = 0; j < m_contents.size(); j++)
	{
		vtItem *item = m_contents[j]->FindItemByName(name);
		if (item)
		{
			inst->SetItem(item);
			break;
		}
	}
}

unsigned int Builder::ElevLayerArray(std::vector<vtElevLayer*> &elevs)
{
	for (int l = 0; l < NumLayers(); l++)
	{
		vtLayer *lp = m_Layers.GetAt(l);
		if (lp->GetType() == LT_ELEVATION && lp->GetVisible())
			elevs.push_back((vtElevLayer *)lp);
	}
	return (unsigned int)elevs.size();
}

/**
 * Fill the gaps (NODATA heixels) in an elevation layer, by interpolating
 * from the surrounding values.
 *
 * \param el The elevation layer to fill the gaps on.
 * \param area Optionally, restrict the operation to a given area.
 */
bool Builder::FillElevGaps(vtElevLayer *el, DRECT *area, int iMethod)
{
	// Create progress dialog for the slow part
	OpenProgressDialog(_("Filling Gaps"), true);

	bool bGood;

	if (iMethod == -1)
		iMethod = g_Options.GetValueInt(TAG_GAP_FILL_METHOD);
	if (iMethod == 1)
		// fast
		bGood = el->m_pGrid->FillGaps(area, progress_callback);
	else if (iMethod == 2)
		// slow and smooth
		bGood = el->m_pGrid->FillGapsSmooth(area, progress_callback);
	else if (iMethod == 3)
	{
		int result = el->m_pGrid->FillGapsByRegionGrowing(2, 5, progress_callback);
		bGood = (result != -1);
	}

	CloseProgressDialog();
	return bGood;
}

/**
 * From a set of elevation layers, pick the valid elevation that occurs latest
 *  in the set.
 */
float ElevLayerArrayValue(std::vector<vtElevLayer*> &elevs, const DPoint2 &p)
{
	float fData, fBestData = INVALID_ELEVATION;
	for (unsigned int g = 0; g < elevs.size(); g++)
	{
		vtElevLayer *elev = elevs[g];

		vtElevationGrid *grid = elev->m_pGrid;
		vtTin2d *tin = elev->m_pTin;
		if (grid)
		{
			fData = grid->GetFilteredValue(p);
			if (fData != INVALID_ELEVATION)
				fBestData = fData;
		}
		else if (tin)
		{
			if (tin->FindAltitudeOnEarth(p, fData))
				fBestData = fData;
		}
	}
	return fBestData;
}

/**
 * From a set of elevation layers, pick the average of all the valid elevations
 *  in the set.
 */
float ElevLayerArrayValueAverage(std::vector<vtElevLayer*> &elevs, const DPoint2 &p)
{
	float fData, fSum = 0.0f;
	int count = 0;

	for (unsigned int g = 0; g < elevs.size(); g++)
	{
		vtElevLayer *elev = elevs[g];

		vtElevationGrid *grid = elev->m_pGrid;
		vtTin2d *tin = elev->m_pTin;
		if (grid)
		{
			fData = grid->GetFilteredValue(p);
			if (fData != INVALID_ELEVATION)
			{
				fSum += fData;
				count++;
			}
		}
		else if (tin)
		{
			if (tin->FindAltitudeOnEarth(p, fData))
			{
				fSum += fData;
				count++;
			}
		}
	}
	if (count)
		return fSum / count;
	else
		return INVALID_ELEVATION;
}

void ElevLayerArrayRange(std::vector<vtElevLayer*> &elevs,
						 float &minval, float &maxval)
{
	float fMin = 1E9;
	float fMax = -1E9;
	for (unsigned int g = 0; g < elevs.size(); g++)
	{
		float LayerMin, LayerMax;
		elevs[g]->GetHeightField()->GetHeightExtents(LayerMin, LayerMax);

		if (LayerMin != INVALID_ELEVATION && LayerMin < fMin)
			fMin = LayerMin;
		if (LayerMax != INVALID_ELEVATION && LayerMax  > fMax)
			fMax = LayerMax;
	}
	minval = fMin;
	maxval = fMax;
}

//
// sample all elevation layers into this one
//
bool Builder::SampleCurrentTerrains(vtElevLayer *pTarget)
{
	VTLOG1(" SampleCurrentTerrains\n");
	// measure time
	clock_t tm1 = clock();

	DRECT area;
	pTarget->GetExtent(area);
	DPoint2 step = pTarget->m_pGrid->GetSpacing();

	int i, j, layers = m_Layers.GetSize();
	float fData=0, fBestData;
	int iColumns, iRows;
	pTarget->m_pGrid->GetDimensions(iColumns, iRows);

	// Create progress dialog for the slow part
	OpenProgressDialog(_("Merging and Resampling Elevation Layers"), true);

	std::vector<vtElevLayer*> elevs;
	ElevLayerArray(elevs);

	// Setup TINs for speedy picking
	for (int l = 0; l < NumLayers(); l++)
	{
		vtLayer *lp = m_Layers.GetAt(l);
		if (lp->GetType() == LT_ELEVATION && lp->GetVisible())
		{
			vtElevLayer *el = (vtElevLayer *)lp;
			if (el->m_pTin)
			{
				int tris = el->m_pTin->NumTris();
				// Aim for no more than 50 triangles in a bin
				int bins = (int) sqrt((double) tris / 50);
				if (bins < 10)
					bins = 10;
				el->m_pTin->SetupTriangleBins(bins, progress_callback);
			}
		}
	}

	// iterate through the vertices of the new terrain
	DPoint2 p;
	wxString str;
	for (i = 0; i < iColumns; i++)
	{
		if ((i % 5) == 0)
		{
			str.Printf(_T("%d / %d"), i, iColumns);
			if (UpdateProgressDialog(i*100/iColumns, str))
			{
				CloseProgressDialog();
				return false;
			}
		}
		p.x = area.left + (i * step.x);
		for (j = 0; j < iRows; j++)
		{
			p.y = area.bottom + (j * step.y);

			// find some data for this point
			fBestData = ElevLayerArrayValue(elevs, p);
			pTarget->m_pGrid->SetFValue(i, j, fBestData);
		}
	}
	CloseProgressDialog();

	clock_t tm2 = clock();
	float time = ((float)tm2 - tm1)/CLOCKS_PER_SEC;
	VTLOG(" SampleCurrentTerrains: %.3f seconds.\n", time);

	return true;
}


//
// sample all image data into this one
//
bool Builder::SampleCurrentImages(vtImageLayer *pTargetLayer)
{
	vtImage *pTarget = pTargetLayer->GetImage();

	DRECT area;
	pTarget->GetExtent(area);
	DPoint2 step = pTarget->GetSpacing();

	int i, j, l, layers = m_Layers.GetSize();
	int iColumns, iRows;
	pTarget->GetDimensions(iColumns, iRows);

	// Create progress dialog for the slow part
	OpenProgressDialog(_("Merging and Resampling Image Layers"), true);

	vtImage **images = new vtImage *[LayersOfType(LT_IMAGE)];
	int g, num_image = 0;
	for (l = 0; l < layers; l++)
	{
		vtLayer *lp = m_Layers.GetAt(l);
		if (lp->GetType() == LT_IMAGE)
			images[num_image++] = ((vtImageLayer *)lp)->GetImage();
	}

	double dRes = step.x;

	// Get ready to multisample
	DLine2 offsets;
	int iNSampling = g_Options.GetValueInt(TAG_SAMPLING_N);
	MakeSampleOffsets(step, iNSampling, offsets);
	dRes /= iNSampling;

	// iterate through the pixels of the new image
	DPoint2 p;
	RGBi rgb, sampled;
	int count;
	for (j = 0; j < iRows; j++)
	{
		if (UpdateProgressDialog(j*100/iRows))
		{
			// Cancel
			CloseProgressDialog();
			return false;
		}

		// Sample at the pixel centers, which are 1/2 pixel in from extents
		p.y = area.bottom + (step.y/2) + (j * step.y);

		for (i = 0; i < iColumns; i++)
		{
			p.x = area.left + (step.x/2) + (i * step.x);

			// find some data for this point
			count = 0;
			for (g = 0; g < num_image; g++)
			{
				// take image that's on top (last in list)
				if (images[g]->GetMultiSample(p, offsets, sampled, dRes))
				{
					rgb = sampled;
					count++;
				}
			}
			if (count)
				pTarget->SetRGB(i, iRows-1-j, rgb.r, rgb.g, rgb.b);
			else
				// write NODATA (black, for now)
				pTarget->SetRGB(i, iRows-1-j, 0, 0, 0);
		}
	}
	CloseProgressDialog();
	delete images;
	return true;
}


float Builder::GetHeightFromTerrain(const DPoint2 &p)
{
	float height = INVALID_ELEVATION;

	int layers = m_Layers.GetSize();
	for (int i = 0; i < layers; i++)
	{
		vtLayer *l = m_Layers.GetAt(i);
		if (l->GetType() != LT_ELEVATION || !l->GetVisible()) continue;
		vtElevLayer *pEL = (vtElevLayer *)l;
		height = pEL->GetElevation(p);
		if (height != INVALID_ELEVATION)
			break;
	}
	return height;
}

bool Builder::GetRGBUnderCursor(const DPoint2 &p, RGBi &rgb)
{
	bool success = false;
	int layers = m_Layers.GetSize();
	RGBi value;
	for (int i = 0; i < layers; i++)
	{
		vtLayer *l = m_Layers.GetAt(i);
		if (l->GetType() != LT_IMAGE || !l->GetVisible()) continue;
		vtImageLayer *pIL = (vtImageLayer *)l;
		if (pIL->GetImage()->GetColorSolid(p, value))
		{
			rgb = value;
			success = true;
		}
	}
	return success;
}

void Builder::SetProjection(const vtProjection &p)
{
	char type[7], value[4000];
	p.GetTextDescription(type, value);
	VTLOG("Setting main projection to: %s, %s\n", type, value);

	m_proj = p;
}


////////////////////////////////////////////////////////////////
// Project operations

bool Builder::LoadSpeciesFile(const char *fname)
{
	if (!GetPlantList()->ReadXML(fname))
	{
		DisplayAndLog("Couldn't read plant list from file '%s'.", fname);
		return false;
	}
	m_strSpeciesFilename = fname;
	return true;
}

bool Builder::LoadBiotypesFile(const char *fname)
{
	if (!m_BioRegion.Read(fname, m_PlantList))
	{
		DisplayAndLog("Couldn't read bioregion list from file '%s'.", fname);
		return false;
	}
	m_strBiotypesFilename = fname;
	return true;
}


//////////////////////////
// Elevation ops

void Builder::ScanElevationLayers(int &count, int &floating, int &tins, DPoint2 &spacing)
{
	count = floating = tins = 0;
	spacing.Set(0,0);
	for (unsigned int i = 0; i < m_Layers.GetSize(); i++)
	{
		vtLayer *l = m_Layers.GetAt(i);
		if (l->GetType() != LT_ELEVATION)
			continue;

		count++;
		vtElevLayer *el = (vtElevLayer *)l;
		if (el->IsGrid())
		{
			vtElevationGrid *grid = el->m_pGrid;
			if (grid->IsFloatMode() || grid->GetScale() != 1.0f)
				floating++;

			spacing = grid->GetSpacing();
		}
		else
		{
			tins++;
			// All TINs have floating-point precision
			floating++;
		}
	}
}

void Builder::MergeResampleElevation(BuilderView *pView)
{
	VTLOG1("MergeResampleElevation\n");

	// If any of the input terrain are floats, then recommend to the user
	// that the output should be float as well.
	bool floatmode = false;

	// sample spacing in meters/heixel or degrees/heixel
	DPoint2 spacing(0, 0);
	int count = 0, floating = 0, tins = 0;
	ScanElevationLayers(count, floating, tins, spacing);
	VTLOG(" Layers: %d, Elevation layers: %d, %d are floating point\n",
		NumLayers(), count, floating);

	if (count == 0)
	{
		DisplayAndLog("Sorry, you must have some elevation grid layers\n"
					  "to perform a sampling operation on them.");
		return;
	}
	if (floating > 0)
		floatmode = true;

	// Always recommend sub-meter precision when sampling TINs
	if (tins > 0)
		floatmode = true;

	if (spacing == DPoint2(0, 0))
	{
		// There were no elevation grids to estimate spacing, so just give
		//  a default value.
		spacing.Set(1,1);
	}

	// Open the Resample dialog
	ResampleDlg dlg(m_pParentWindow, -1, _("Merge and Resample Elevation"));
	dlg.m_fEstX = spacing.x;
	dlg.m_fEstY = spacing.y;
	dlg.m_area = m_area;
	dlg.m_bFloats = floatmode;
	dlg.m_tileopts = m_tileopts;
	dlg.SetView(pView);
	dlg.FormatTilingString();

	int ret = dlg.ShowModal();
	if (pView)
		pView->HideGridMarks();
	if (ret == wxID_CANCEL)
		return;

	// Make new terrain
	vtElevLayer *pOutput = new vtElevLayer(dlg.m_area, dlg.m_iSizeX,
			dlg.m_iSizeY, dlg.m_bFloats, dlg.m_fVUnits, m_proj);

	if (!pOutput->m_pGrid->HasData())
	{
		wxString str;
		str.Printf(_("Failed to initialize %d x %d elevation grid"), dlg.m_iSizeX, dlg.m_iSizeY);
		wxMessageBox(str);
		return;
	}

	// fill in the value for pOutput by merging samples from all other terrain
	if (!SampleCurrentTerrains(pOutput))
	{
		delete pOutput;
		return;
	}
	pOutput->m_pGrid->ComputeHeightExtents();

	if (dlg.m_bFillGaps)
	{
		if (!FillElevGaps(pOutput))
		{
			delete pOutput;
			return;
		}
	}

	if (dlg.m_bNewLayer)
		AddLayerWithCheck(pOutput);
	else if (dlg.m_bToFile)
	{
		OpenProgressDialog(_T("Writing file"), true);

		wxString fname = dlg.m_strToFile;
		bool gzip = (fname.Right(3).CmpNoCase(_T(".gz")) == 0);
		vtString fname_utf8 = (const char *) fname.mb_str(wxConvUTF8);

		bool success = pOutput->m_pGrid->SaveToBT(fname_utf8, progress_callback, gzip);
		delete pOutput;
		CloseProgressDialog();

		if (success)
			DisplayAndLog("Successfully wrote to '%s'", (const char *) fname_utf8);
		else
			DisplayAndLog("Did not successfully write to '%s'", (const char *) fname_utf8);
	}
	else if (dlg.m_bToTiles)
	{
		OpenProgressDialog2(_T("Writing tiles"), true);
		bool success = pOutput->WriteGridOfElevTilePyramids(dlg.m_tileopts, pView);
		if (pView)
			pView->HideGridMarks();
		delete pOutput;
		CloseProgressDialog2();
		if (success)
			DisplayAndLog("Successfully wrote to '%s'", (const char *) dlg.m_tileopts.fname);
		else
			DisplayAndLog("Did not successfully write to '%s'", (const char *) dlg.m_tileopts.fname);
	}
}


//////////////////////////////////////////////////////////
// Image ops

void Builder::MergeResampleImages(BuilderView *pView)
{
	// sample spacing in meters/heixel or degrees/heixel
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
	if (spacing == DPoint2(0, 0))
	{
		DisplayAndLog(_("Sorry, you must have some image layers to\n perform a sampling operation on them."));
		return;
	}

	// Open the Resample dialog
	SampleImageDlg dlg(m_pParentWindow, -1, _("Merge and Resample Imagery"));
	dlg.m_fEstX = spacing.x;
	dlg.m_fEstY = spacing.y;
	dlg.m_area = m_area;
	dlg.SetView(pView);
	dlg.m_tileopts = m_tileopts;
	dlg.FormatTilingString();

	int ret = dlg.ShowModal();
	if (pView)
		pView->HideGridMarks();
	if (ret == wxID_CANCEL)
		return;

	// Make new image
	vtImageLayer *pOutputLayer = new vtImageLayer(dlg.m_area, dlg.m_iSizeX,
			dlg.m_iSizeY, m_proj);
	vtImage *pOutput = pOutputLayer->GetImage();

	if (!pOutput->GetBitmap())
	{
		DisplayAndLog(_("Sorry, could not allocate an image of that size."));
		delete pOutputLayer;
		return;
	}

	// fill in the value for pBig by merging samples from all other terrain
	bool success = SampleCurrentImages(pOutputLayer);
	if (!success)
	{
		delete pOutputLayer;
		return;
	}

	if (dlg.m_bNewLayer)
		AddLayerWithCheck(pOutputLayer);
	else if (dlg.m_bToFile)
	{
		OpenProgressDialog(_T("Writing file"), true);
		vtString fname = (const char *) dlg.m_strToFile.mb_str(wxConvUTF8);
		success = pOutput->SaveToFile(fname);
		delete pOutput;
		CloseProgressDialog();
		if (success)
			DisplayAndLog("Successfully wrote to '%s'", (const char *) fname);
		else
			DisplayAndLog(("Did not successfully write to '%s'."), (const char *) fname);
	}
	else if (dlg.m_bToTiles)
	{
		OpenProgressDialog(_T("Writing tiles"), true);
		bool success = pOutput->WriteGridOfTilePyramids(dlg.m_tileopts, pView);
		if (pView)
			pView->HideGridMarks();
		delete pOutput;
		CloseProgressDialog();
		if (success)
			DisplayAndLog("Successfully wrote to '%s'", (const char *) dlg.m_tileopts.fname);
		else
			DisplayAndLog("Did not successfully write to '%s'", (const char *) dlg.m_tileopts.fname);
	}
}


//////////////////////////
// Vegetation ops

/**
 * Generate vegetation in a given area, and writes it to a VF file.
 * All options are given in the VegGenOptions object passed in.
 *
 */
void Builder::GenerateVegetation(const char *vf_file, DRECT area,
	VegGenOptions &opt)
{
	OpenProgressDialog(_("Generating Vegetation"), true);

	clock_t time1 = clock();

	vtBioType SingleBiotype;
	if (opt.m_iSingleSpecies != -1)
	{
		// simply use a single species
		vtPlantSpecies *ps = m_PlantList.GetSpecies(opt.m_iSingleSpecies);
		SingleBiotype.AddPlant(ps, opt.m_fFixedDensity);
		opt.m_iSingleBiotype = m_BioRegion.AddType(&SingleBiotype);
	}

	// Create some optimization indices to speed it up
	if (opt.m_pBiotypeLayer)
		opt.m_pBiotypeLayer->CreateIndex(10);
	if (opt.m_pDensityLayer)
		opt.m_pDensityLayer->CreateIndex(10);

	GenerateVegetationPhase2(vf_file, area, opt);

	// Clean up the optimization indices
	if (opt.m_pBiotypeLayer)
		opt.m_pBiotypeLayer->FreeIndex();
	if (opt.m_pDensityLayer)
		opt.m_pDensityLayer->FreeIndex();

	// clean up temporary biotype
	if (opt.m_iSingleSpecies != -1)
	{
		m_BioRegion.m_Types.RemoveAt(opt.m_iSingleBiotype);
	}

	clock_t time2 = clock();
	float time = ((float)time2 - time1)/CLOCKS_PER_SEC;
	VTLOG("GenerateVegetation: %.3f seconds.\n", time);
}

void Builder::GenerateVegetationPhase2(const char *vf_file, DRECT area,
	VegGenOptions &opt)
{
	// Avoid trouble with '.' and ',' in Europe
	LocaleWrap normal_numbers(LC_NUMERIC, "C");

	unsigned int i, j, k;
	DPoint2 p, p2;

	unsigned int x_trees = (unsigned int)(area.Width() / opt.m_fSampling);
	unsigned int y_trees = (unsigned int)(area.Height() / opt.m_fSampling);

	int bio_type=0;
	float density_scale;

	vtPlantInstanceArray pia;
	vtPlantDensity *pd;
	vtBioType *bio;
	vtPlantSpecies *ps;

	// inherit projection from the main frame
	vtProjection proj;
	GetProjection(proj);
	pia.SetProjection(proj);

	m_BioRegion.ResetAmounts();
	pia.SetPlantList(&m_PlantList);

	// Iterate over the whole area, creating plant instances
	for (i = 0; i < x_trees; i ++)
	{
		wxString str;
		str.Printf(_("column %d/%d, plants: %d"), i, x_trees, pia.GetNumEntities());
		if (UpdateProgressDialog(i * 100 / x_trees, str))
		{
			// user cancel
			CloseProgressDialog();
			return;
		}

		p.x = area.left + (i * opt.m_fSampling);
		for (j = 0; j < y_trees; j ++)
		{
			p.y = area.bottom + (j * opt.m_fSampling);

			// randomize the position slightly
			p2.x = p.x + random_offset(opt.m_fSampling * 0.5f);
			p2.y = p.y + random_offset(opt.m_fSampling * 0.5f);

			// Density
			if (opt.m_pDensityLayer)
			{
				density_scale = opt.m_pDensityLayer->FindDensity(p2);
				if (density_scale == 0.0f)
					continue;
			}
			else
				density_scale = 1.0f;

			// Species
			if (opt.m_iSingleSpecies != -1)
			{
				// use our single species biotype
				bio_type = opt.m_iSingleBiotype;
			}
			else
			{
				if (opt.m_iSingleBiotype != -1)
				{
					bio_type = opt.m_iSingleBiotype;
				}
				else if (opt.m_pBiotypeLayer != NULL)
				{
					bio_type = opt.m_pBiotypeLayer->FindBiotype(p2);
					if (bio_type == -1)
						continue;
				}
			}
			// look at veg_type to decide which BioType to use
			bio = m_BioRegion.m_Types[bio_type];

			float square_meters = opt.m_fSampling * opt.m_fSampling;
			float factor = density_scale * square_meters * opt.m_fScarcity;

			// the amount of each species present accumulates until it
			//  exceeds 1, at which time we produce a plant instance
			for (k = 0; k < bio->m_Densities.GetSize(); k++)
			{
				pd = bio->m_Densities[k];

				pd->m_amount += (pd->m_plant_per_m2 * factor);
			}

			ps = NULL;
			for (k = 0; k < bio->m_Densities.GetSize(); k++)
			{
				pd = bio->m_Densities[k];
				if (pd->m_amount > 1.0f)	// time to plant
				{
					pd->m_amount -= 1.0f;
					pd->m_iNumPlanted++;
					ps = pd->m_pSpecies;
					break;
				}
			}
			if (ps == NULL)
				continue;

			// Now determine size
			float size;
			if (opt.m_fFixedSize != -1.0f)
			{
				size = opt.m_fFixedSize;
			}
			else
			{
				float range = opt.m_fRandomTo - opt.m_fRandomFrom;
				size = (opt.m_fRandomFrom + random(range)) * ps->GetMaxHeight();
			}

			// Finally, add the plant
			pia.AddPlant(p2, size, ps);
		}
	}
	pia.WriteVF(vf_file);
	CloseProgressDialog();

	// display a useful message informing the user what was planted
	int unplanted = 0;
	wxString msg, str;
	msg = _("Vegetation distribution results:\n");
	for (i = 0; i < m_BioRegion.m_Types.GetSize(); i++)
	{
		bio = m_BioRegion.m_Types[i];

		int total_this_type = 0;
		for (k = 0; k < bio->m_Densities.GetSize(); k++)
		{
			pd = bio->m_Densities[k];
			total_this_type += pd->m_iNumPlanted;
			unplanted += (int) (pd->m_amount);
		}
		str.Printf(_("  BioType %d"), i);
		msg += str;

		if (total_this_type != 0)
		{
			msg += _T("\n");
			for (k = 0; k < bio->m_Densities.GetSize(); k++)
			{
				pd = bio->m_Densities[k];
				str.Printf(_("    Plant %d: %hs: %d generated.\n"), k,
					(const char *) pd->m_pSpecies->GetCommonName().m_strName, pd->m_iNumPlanted);
				msg += str;
			}
		}
		else
			msg += _(": None.\n");
	}
	DisplayAndLog(msg.mb_str(wxConvUTF8));

	if (unplanted > 0)
	{
		msg.Printf(_T("%d plants were generated that could not be placed.\n"), unplanted);
		msg += _T("Try to decrease your spacing or scarcity, so that\n");
		msg += _T("there are enough places to plant.");
		wxMessageBox(msg, _("Warning"));
	}
}


//////////////////////////////

using namespace std;

void Builder::ReadDataPath()
{
	wxString cwd1 = wxGetCwd();
	vtString cwd = (const char *) cwd1.mb_str(wxConvUTF8);
	VTLOG("  Current directory: '%s'\n", (const char *) cwd);

	VTLOG("Looking for data path info.\n");

	// Look these up, we might need them
	wxString Dir1 = wxStandardPaths::Get().GetUserConfigDir();
	wxString Dir2 = wxStandardPaths::Get().GetConfigDir();

	vtString AppDataUser = (const char *) Dir1.mb_str(wxConvUTF8);
	vtString AppDataCommon = (const char *) Dir2.mb_str(wxConvUTF8);

	// Read the vt datapaths
	bool bLoadedDataPaths = vtLoadDataPath(AppDataUser, AppDataCommon);

	if (!bLoadedDataPaths)
	{
		VTLOG1("  Not found, using default of '../Data/'.\n");
		vtGetDataPath().push_back(vtString("../Data/"));
	}
}


bool Builder::ConfirmValidCRS(vtProjection *pProj)
{
	if (!pProj->GetRoot())
	{
		// No projection.
		int res;
		if (g_Options.GetValueBool(TAG_USE_CURRENT_CRS))
			res = wxNO;		// Don't ask user, just use current CRS
		else
		{
			wxString msg = _("File lacks a projection.\n Would you like to specify one?\n Yes - specify projection\n No - use current projection\n");
			res = wxMessageBox(msg, _("Coordinate Reference System"), wxYES_NO | wxCANCEL);
		}
		if (res == wxYES)
		{
			ProjectionDlg dlg(NULL, -1, _("Please indicate projection"));
			dlg.SetProjection(m_proj);

			if (dlg.ShowModal() == wxID_CANCEL)
				return false;
			dlg.GetProjection(*pProj);
		}
		else if (res == wxNO)
			*pProj = m_proj;
		else if (res == wxCANCEL)
			return false;
	}
	return true;
}

