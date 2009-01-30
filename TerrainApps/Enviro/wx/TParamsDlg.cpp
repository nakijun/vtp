//
// Name: TParamsDlg.cpp
//
// Copyright (c) 2001-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifdef __GNUG__
	#pragma implementation "TParamsDlg.cpp"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/colordlg.h>
#include <wx/dir.h>

#include "vtlib/vtlib.h"
#include "vtlib/core/Location.h"
#include "vtdata/FilePath.h"		// for FindFileOnPaths
#include "vtdata/vtLog.h"
#include "vtui/ColorMapDlg.h"
#include "vtui/Helper.h"			// for AddFilenamesToChoice

#include "TParamsDlg.h"
#include "TimeDlg.h"
#include "StyleDlg.h"

#include "ScenarioParamsDialog.h"
#define NTILES 4

//---------------------------------------------------------------------------

/**
 * wxListBoxEventHandler is a roudabout way of catching events on our
 * listboxes, to implement the "Delete" key operation on them.
 */
class wxListBoxEventHandler: public wxEvtHandler
{
public:
	wxListBoxEventHandler(TParamsDlg *dlg, wxListBox *pBox)
	{
		m_pDlg = dlg;
		m_pBox = pBox;
	}
	void OnChar(wxKeyEvent& event)
	{
		if (event.GetKeyCode() == WXK_DELETE)
		{
			int sel = m_pBox->GetSelection();
			int count = m_pBox->GetCount();
			if (sel != -1 && sel < count-1)
			{
				m_pDlg->DeleteItem(m_pBox);
				m_pDlg->TransferDataToWindow();
			}
		}
		event.Skip();
	}

private:
	TParamsDlg *m_pDlg;
	wxListBox *m_pBox;
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxListBoxEventHandler, wxEvtHandler)
	EVT_CHAR(wxListBoxEventHandler::OnChar)
END_EVENT_TABLE()


// WDR: class implementations

//----------------------------------------------------------------------------
// TParamsDlg
//----------------------------------------------------------------------------

// WDR: event table for TParamsDlg

BEGIN_EVENT_TABLE(TParamsDlg,AutoDialog)
	EVT_INIT_DIALOG (TParamsDlg::OnInitDialog)

	// Elevation
	EVT_RADIOBUTTON( ID_USE_GRID, TParamsDlg::OnCheckBoxElevType )
	EVT_RADIOBUTTON( ID_USE_TIN, TParamsDlg::OnCheckBoxElevType )
	EVT_RADIOBUTTON( ID_USE_TILESET, TParamsDlg::OnCheckBoxElevType )
	EVT_CHOICE( ID_LODMETHOD, TParamsDlg::OnCheckBox )

	// Texture
	EVT_CHOICE( ID_CHOICE_TILESIZE, TParamsDlg::OnTileSize )
	EVT_CHOICE( ID_TFILE_BASE, TParamsDlg::OnTextureFileBase )

	EVT_RADIOBUTTON( ID_NONE, TParamsDlg::OnTextureNone )
	EVT_RADIOBUTTON( ID_SINGLE, TParamsDlg::OnTextureSingle )
	EVT_RADIOBUTTON( ID_DERIVED, TParamsDlg::OnTextureDerived )
	EVT_RADIOBUTTON( ID_TILED_4BY4, TParamsDlg::OnTextureTiled )
	EVT_RADIOBUTTON( ID_TILESET, TParamsDlg::OnTextureTileset )
	EVT_CHECKBOX( ID_TILE_THREADING, TParamsDlg::OnCheckBox )

	EVT_COMBOBOX( ID_TFILE_SINGLE, TParamsDlg::OnComboTFileSingle )
	EVT_BUTTON( ID_EDIT_COLORS, TParamsDlg::OnEditColors )
	EVT_CHECKBOX( ID_DETAILTEXTURE, TParamsDlg::OnCheckBox )

	// Culture
	EVT_CHECKBOX( ID_PLANTS, TParamsDlg::OnCheckBox )
	EVT_CHECKBOX( ID_ROADS, TParamsDlg::OnCheckBox )
	EVT_CHECKBOX( ID_CHECK_STRUCTURE_SHADOWS, TParamsDlg::OnCheckBox )
	EVT_CHECKBOX( ID_SHADOW_LIMIT, TParamsDlg::OnCheckBox )
	EVT_CHECKBOX( ID_CHECK_STRUCTURE_PAGING, TParamsDlg::OnCheckBox )

	// Ephemeris
	EVT_CHECKBOX( ID_OCEANPLANE, TParamsDlg::OnCheckBox )
	EVT_CHECKBOX( ID_DEPRESSOCEAN, TParamsDlg::OnCheckBox )
	EVT_CHECKBOX( ID_SKY, TParamsDlg::OnCheckBox )
	EVT_CHECKBOX( ID_FOG, TParamsDlg::OnCheckBox )
	EVT_BUTTON( ID_BGCOLOR, TParamsDlg::OnBgColor )
	EVT_BUTTON( ID_SET_INIT_TIME, TParamsDlg::OnSetInitTime )

	// Abstracts
	EVT_BUTTON( ID_STYLE, TParamsDlg::OnStyle )

	// HUD
	EVT_BUTTON( ID_OVERLAY_DOTDOTDOT, TParamsDlg::OnOverlay )

	// Camera
	EVT_TEXT( ID_LOCFILE, TParamsDlg::OnChoiceLocFile )
	EVT_CHOICE( ID_INIT_LOCATION, TParamsDlg::OnChoiceInitLocation )

	// Scenario
	EVT_BUTTON( ID_NEW_SCENARIO, TParamsDlg::OnNewScenario )
	EVT_BUTTON( ID_DELETE_SCENARIO, TParamsDlg::OnDeleteScenario )
	EVT_BUTTON( ID_EDIT_SCENARIO, TParamsDlg::OnEditScenario )
	EVT_BUTTON( ID_MOVEUP_SCENARIO, TParamsDlg::OnMoveUpScenario )
	EVT_BUTTON( ID_MOVEDOWN_SCENARIO, TParamsDlg::OnMoveDownSceanario )
	EVT_LISTBOX( ID_SCENARIO_LIST, TParamsDlg::OnScenarioListEvent )
	EVT_CHOICE( ID_CHOICE_SCENARIO, TParamsDlg::OnChoiceScenario )

	// Clickable listboxes
	EVT_LISTBOX_DCLICK( ID_STRUCTFILES, TParamsDlg::OnListDblClickStructure )
	EVT_LISTBOX_DCLICK( ID_RAWFILES, TParamsDlg::OnListDblClickRaw )
	EVT_LISTBOX_DCLICK( ID_ANIM_PATHS, TParamsDlg::OnListDblClickAnimPaths )
	EVT_LISTBOX_DCLICK( ID_IMAGEFILES, TParamsDlg::OnListDblClickImage )

END_EVENT_TABLE()

TParamsDlg::TParamsDlg( wxWindow *parent, wxWindowID id, const wxString &title,
	const wxPoint &position, const wxSize& size, long style ) :
	AutoDialog( parent, id, title, position, size, style | wxRESIZE_BORDER )
{
	VTLOG("TParamsDlg: Constructing.\n");

	TParamsFunc( this, TRUE, TRUE );

	m_bSetting = false;

	// make sure that validation gets down to the child windows
	SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);

	// including the children of the notebook
	wxNotebook *notebook = (wxNotebook*) FindWindow( ID_NOTEBOOK );
	notebook->SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);

	m_pPreLightFactor = GetLightFactor();
	m_pStructFiles = GetStructFiles();
	m_pRawFiles = GetRawFiles();
	m_pAnimFiles = GetAnimPaths();
	m_pImageFiles = GetImageFiles();
	m_pRoadFile = GetRoadfile();
	m_pTreeFile = GetTreefile();
	m_pTextureFileSingle = GetTfileSingle();
	m_pTextureFileTileset = GetTfileTileset();
	m_pDTName = GetDTName();
	m_pLodMethod = GetLodmethod();
	m_pFilename = GetFilename();
	m_pFilenameTin = GetFilenameTin();
	m_pFilenameTileset = GetFilenameTileset();
	m_pLocFile = GetLocfile();
	m_pShadowRez = GetChoiceShadowRez();
	m_pSkyTexture = GetSkytexture();
	m_pLocField = GetLocField();
	m_pNavStyle = GetNavStyle();

	m_pNone = GetNone();
	m_pSingle = GetSingle();
	m_pDerived = GetDerived();
	m_pTiled = GetTiled();
	m_pTileset = GetTileset();
	m_pColorMap = GetColorMap();

	m_pScenarioList = GetScenarioList();
	m_iOverlayX = 0;
	m_iOverlayY = 0;
	m_bOverview = false;
	m_bCompass = false;

	GetTilesize()->Clear();
	GetTilesize()->Append(_T("256"));
	GetTilesize()->Append(_T("512"));
	GetTilesize()->Append(_T("1024"));
	GetTilesize()->Append(_T("2048"));
	GetTilesize()->Append(_T("4096"));
	GetTilesize()->SetSelection(2);
	m_iTilesizeIndex = 2;
	m_iTilesize = 1024;

	// Create Validators To Attach C++ Members To WX Controls

	// overall name
	AddValidator(ID_TNAME, &m_strTerrainName);

	// elevation
	AddValidator(ID_USE_GRID, &m_bGrid);
	AddValidator(ID_USE_TIN, &m_bTin);
	AddValidator(ID_USE_TILESET, &m_bTileset);
	AddValidator(ID_FILENAME, &m_strFilename);
	AddValidator(ID_FILENAME_TIN, &m_strFilenameTin);
	AddValidator(ID_FILENAME_TILES, &m_strFilenameTileset);
	AddNumValidator(ID_VERTEXAG, &m_fVerticalExag, 2);

	// nav
	AddNumValidator(ID_MINHEIGHT, &m_fMinHeight, 2);
	AddValidator(ID_NAV_STYLE, &m_iNavStyle);
	AddNumValidator(ID_NAVSPEED, &m_fNavSpeed, 2);
	AddValidator(ID_LOCFILE, &m_strLocFile);
	AddValidator(ID_INIT_LOCATION, &m_iInitLocation);
	AddNumValidator(ID_HITHER, &m_fHither);
	AddValidator(ID_ACCEL, &m_bAccel);
	AddValidator(ID_ALLOW_ROLL, &m_bAllowRoll);

	// LOD
	AddValidator(ID_LODMETHOD, &m_iLodMethod);
	AddNumValidator(ID_TRI_COUNT, &m_iTriCount);
	AddValidator(ID_TRISTRIPS, &m_bTriStrips);
	AddNumValidator(ID_VTX_COUNT, &m_iVertCount);
	AddNumValidator(ID_TILE_CACHE_SIZE, &m_iTileCacheSize);
	AddValidator(ID_TILE_THREADING, &m_bTileThreading);

	// time
	AddValidator(ID_TIMEMOVES, &m_bTimeOn);
	AddValidator(ID_TEXT_INIT_TIME, &m_strInitTime);
	AddNumValidator(ID_TIMESPEED, &m_fTimeSpeed, 2);

	// texture
	AddValidator(ID_TFILE_SINGLE, &m_strTextureSingle);
	AddValidator(ID_CHOICE_TILESIZE, &m_iTilesizeIndex);
	AddValidator(ID_TFILE_BASE, &m_strTextureBase);
	AddValidator(ID_TFILENAME, &m_strTexture4x4);
	AddValidator(ID_TFILE_TILESET, &m_strTextureTileset);
	AddValidator(ID_TEXTURE_GRADUAL, &m_bTextureGradual);
	AddNumValidator(ID_TEX_LOD, &m_fTextureLODFactor);
	AddValidator(ID_MIPMAP, &m_bMipmap);
	AddValidator(ID_16BIT, &m_b16bit);
	AddValidator(ID_PRELIGHT, &m_bPreLight);
	AddValidator(ID_CAST_SHADOWS, &m_bCastShadows);
	AddNumValidator(ID_LIGHT_FACTOR, &m_fPreLightFactor, 2);
	AddValidator(ID_CHOICE_COLORS, &m_strColorMap);
	AddValidator(ID_RETAIN, &m_bTextureRetain);

	// detail texture
	AddValidator(ID_DETAILTEXTURE, &m_bDetailTexture);
	AddValidator(ID_DT_NAME, &m_strDetailName);
	AddNumValidator(ID_DT_SCALE, &m_fDetailScale);
	AddNumValidator(ID_DT_DISTANCE, &m_fDetailDistance);

	// culture page
	AddValidator(ID_PLANTS, &m_bPlants);
	AddValidator(ID_TREEFILE, &m_strVegFile);
	AddNumValidator(ID_VEGDISTANCE, &m_iVegDistance);

	AddValidator(ID_ROADS, &m_bRoads);
	AddValidator(ID_ROADFILE, &m_strRoadFile);
	AddValidator(ID_HIGHWAYS, &m_bHwy);
	AddValidator(ID_PAVED, &m_bPaved);
	AddValidator(ID_DIRT, &m_bDirt);
	AddNumValidator(ID_ROADHEIGHT, &m_fRoadHeight);
	AddNumValidator(ID_ROADDISTANCE, &m_fRoadDistance);
	AddValidator(ID_TEXROADS, &m_bTexRoads);
	AddValidator(ID_ROADCULTURE, &m_bRoadCulture);

	AddValidator(ID_CONTENT_FILE, &m_strContent);
	AddNumValidator(ID_STRUCT_DISTANCE, &m_iStructDistance);

	// shadows
	AddValidator(ID_CHECK_STRUCTURE_SHADOWS, &m_bStructureShadows);
	AddValidator(ID_CHOICE_SHADOW_REZ, &m_iShadowRez);
	AddNumValidator(ID_DARKNESS, &m_fDarkness, 2);
	AddValidator(ID_SHADOWS_DEFAULT_ON, &m_bShadowsDefaultOn);
	AddValidator(ID_SHADOWS_EVERY_FRAME, &m_bShadowsEveryFrame);
	AddValidator(ID_SHADOW_LIMIT, &m_bLimitShadowArea);
	AddNumValidator(ID_SHADOW_LIMIT_RADIUS, &m_fShadowRadius);

	// paging
	AddValidator(ID_CHECK_STRUCTURE_PAGING, &m_bPagingStructures);
	AddNumValidator(ID_PAGING_MAX_STRUCTURES, &m_iPagingStructureMax);
	AddNumValidator(ID_PAGE_OUT_DISTANCE, &m_fPagingStructureDist, 1);
//	AddValidator(ID_VEHICLES, &m_bVehicles);

	// Ephemeric
	AddValidator(ID_SKY, &m_bSky);
	AddValidator(ID_SKYTEXTURE, &m_strSkyTexture);
	AddValidator(ID_OCEANPLANE, &m_bOceanPlane);
	AddNumValidator(ID_OCEANPLANEOFFSET, &m_fOceanPlaneLevel);
	AddValidator(ID_DEPRESSOCEAN, &m_bDepressOcean);
	AddNumValidator(ID_DEPRESSOCEANOFFSET, &m_fDepressOceanLevel);
	AddValidator(ID_HORIZON, &m_bHorizon);
	AddValidator(ID_FOG, &m_bFog);
	AddNumValidator(ID_FOG_DISTANCE, &m_fFogDistance);

	// HUD
	AddValidator(ID_OVERLAY_FILE, &m_strOverlayFile);
	AddNumValidator(ID_OVERLAY_X, &m_iOverlayX);
	AddNumValidator(ID_OVERLAY_Y, &m_iOverlayY);
	AddValidator(ID_CHECK_OVERVIEW, &m_bOverview);
	AddValidator(ID_CHECK_COMPASS, &m_bCompass);

	// It's somewhat roundabout, but this lets us capture events on the
	// listbox controls without having to subclass.
	m_pStructFiles->PushEventHandler(new wxListBoxEventHandler(this, m_pStructFiles));
	m_pRawFiles->PushEventHandler(new wxListBoxEventHandler(this, m_pRawFiles));
	m_pAnimFiles->PushEventHandler(new wxListBoxEventHandler(this, m_pAnimFiles));
	m_pImageFiles->PushEventHandler(new wxListBoxEventHandler(this, m_pImageFiles));
}

TParamsDlg::~TParamsDlg()
{
	m_pStructFiles->PopEventHandler(true);
	m_pRawFiles->PopEventHandler(true);
	m_pAnimFiles->PopEventHandler(true);
	m_pImageFiles->PopEventHandler(true);
}

//
// Set the values in the dialog from the supplied paramter structure.
// Note that TParams uses UTF8 for all its strings, so we need to use
//  from_utf8() when copying to wxString.
//
void TParamsDlg::SetParams(const TParams &Params)
{
	VTLOG("TParamsDlg::SetParams\n");
	LocaleWrap normal_numbers(LC_NUMERIC, "C");

	// overall name
	m_strTerrainName = wxString(Params.GetValueString(STR_NAME), wxConvUTF8);

	// elevation
	m_bGrid =			Params.GetValueInt(STR_SURFACE_TYPE) == 0;
	m_bTin =			Params.GetValueInt(STR_SURFACE_TYPE) == 1;
	m_bTileset =		Params.GetValueInt(STR_SURFACE_TYPE) == 2;
	if (m_bGrid)
		m_strFilename = wxString(Params.GetValueString(STR_ELEVFILE), wxConvUTF8);
	if (m_bTin)
		m_strFilenameTin = wxString(Params.GetValueString(STR_ELEVFILE), wxConvUTF8);
	if (m_bTileset)
		m_strFilenameTileset = wxString(Params.GetValueString(STR_ELEVFILE), wxConvUTF8);
	m_fVerticalExag =   Params.GetValueFloat(STR_VERTICALEXAG);

	/// navigation
	m_fMinHeight =		Params.GetValueFloat(STR_MINHEIGHT);
	m_iNavStyle =		Params.GetValueInt(STR_NAVSTYLE);
	m_fNavSpeed =		Params.GetValueFloat(STR_NAVSPEED);
	m_strLocFile = wxString(Params.GetValueString(STR_LOCFILE), wxConvUTF8);
	m_strInitLocation = wxString(Params.GetValueString(STR_INITLOCATION), wxConvUTF8);
	m_fHither =			Params.GetValueFloat(STR_HITHER);
	m_bAccel =			Params.GetValueBool(STR_ACCEL);
	m_bAllowRoll =		Params.GetValueBool(STR_ALLOW_ROLL);
	m_AnimPaths =		Params.m_AnimPaths;

	// LOD
	m_iLodMethod =		Params.GetLodMethod();
	m_iTriCount =		Params.GetValueInt(STR_TRICOUNT);
	m_bTriStrips =		Params.GetValueBool(STR_TRISTRIPS);
	m_iVertCount =		Params.GetValueInt(STR_VERTCOUNT);
	m_iTileCacheSize =	Params.GetValueInt(STR_TILE_CACHE_SIZE);
	m_bTileThreading =	Params.GetValueBool(STR_TILE_THREADING);

	// time
	m_bTimeOn =			Params.GetValueBool(STR_TIMEON);
	m_InitTime.SetFromString(Params.GetValueString(STR_INITTIME));
	m_fTimeSpeed =		Params.GetValueFloat(STR_TIMESPEED);

	// texture
	m_iTexture =		Params.GetTextureEnum();

	// single
	if (m_iTexture != TE_TILESET)
		m_strTextureSingle = wxString(Params.GetValueString(STR_TEXTUREFILE), wxConvUTF8);

	// tile4x4
	m_strTextureBase = wxString(Params.GetValueString(STR_TEXTUREBASE), wxConvUTF8);
	m_iTilesize =		Params.GetValueInt(STR_TILESIZE);
	m_iTilesizeIndex = vt_log2(m_iTilesize)-8;
	m_strTexture4x4 = wxString(Params.GetValueString(STR_TEXTURE4BY4), wxConvUTF8);

	// derived
	m_strColorMap = wxString(Params.GetValueString(STR_COLOR_MAP), wxConvUTF8);

	// tileset
	if (m_iTexture == TE_TILESET)
		m_strTextureTileset = wxString(Params.GetValueString(STR_TEXTUREFILE), wxConvUTF8);

	m_bTextureGradual =	Params.GetValueBool(STR_TEXTURE_GRADUAL);
	m_fTextureLODFactor =	Params.GetValueFloat(STR_TEXURE_LOD_FACTOR);
	m_bMipmap =			Params.GetValueBool(STR_MIPMAP);
	m_b16bit =			Params.GetValueBool(STR_REQUEST16BIT);
	m_bPreLight =		Params.GetValueBool(STR_PRELIGHT);
	m_fPreLightFactor = Params.GetValueFloat(STR_PRELIGHTFACTOR);
	m_bCastShadows =	Params.GetValueBool(STR_CAST_SHADOWS);
	m_bTextureRetain =	Params.GetValueBool(STR_TEXTURE_RETAIN);

	// detail texture
	m_bDetailTexture =  Params.GetValueBool(STR_DETAILTEXTURE);
	m_strDetailName = wxString(Params.GetValueString(STR_DTEXTURE_NAME), wxConvUTF8);
	m_fDetailScale = Params.GetValueFloat(STR_DTEXTURE_SCALE);
	m_fDetailDistance = Params.GetValueFloat(STR_DTEXTURE_DISTANCE);

	// culture
	m_bRoads =			Params.GetValueBool(STR_ROADS);
	m_strRoadFile = wxString(Params.GetValueString(STR_ROADFILE), wxConvUTF8);
	m_bHwy =			Params.GetValueBool(STR_HWY);
	m_bPaved =			Params.GetValueBool(STR_PAVED);
	m_bDirt =			Params.GetValueBool(STR_DIRT);
	m_fRoadHeight =		Params.GetValueFloat(STR_ROADHEIGHT);
	m_fRoadDistance =	Params.GetValueFloat(STR_ROADDISTANCE);
	m_bTexRoads =		Params.GetValueBool(STR_TEXROADS);
	m_bRoadCulture =	Params.GetValueBool(STR_ROADCULTURE);

	m_bPlants =			Params.GetValueBool(STR_TREES);
	m_strVegFile = wxString(Params.GetValueString(STR_TREEFILE), wxConvUTF8);
	m_iVegDistance =	Params.GetValueInt(STR_VEGDISTANCE);

	m_bFog =			Params.GetValueBool(STR_FOG);
	m_fFogDistance =	Params.GetValueFloat(STR_FOGDISTANCE);

	// Layers and structure stuff
	m_strContent = wxString(Params.GetValueString(STR_CONTENT_FILE), wxConvUTF8);
	m_Layers = Params.m_Layers;
	m_iStructDistance = Params.GetValueInt(STR_STRUCTDIST);
	// shadows
	m_bStructureShadows =	 Params.GetValueBool(STR_STRUCT_SHADOWS);
	m_iShadowRez =	 vt_log2(Params.GetValueInt(STR_SHADOW_REZ))-8;
	m_fDarkness =			 Params.GetValueFloat(STR_SHADOW_DARKNESS);
	m_bShadowsDefaultOn =	 Params.GetValueBool(STR_SHADOWS_DEFAULT_ON);
	m_bShadowsEveryFrame =	 Params.GetValueBool(STR_SHADOWS_EVERY_FRAME);
	m_bLimitShadowArea =	 Params.GetValueBool(STR_LIMIT_SHADOW_AREA);
	m_fShadowRadius =		 Params.GetValueFloat(STR_SHADOW_RADIUS);
	// paging
	m_bPagingStructures =	 Params.GetValueBool(STR_STRUCTURE_PAGING);
	m_iPagingStructureMax =	 Params.GetValueInt(STR_STRUCTURE_PAGING_MAX);
	m_fPagingStructureDist = Params.GetValueFloat(STR_STRUCTURE_PAGING_DIST);

//	m_bVehicles =	   Params.GetValueBool(STR_VEHICLES);
//  m_fVehicleSize =	Params.GetValueFloat(STR_VEHICLESIZE);
//  m_fVehicleSpeed =   Params.GetValueFloat(STR_VEHICLESPEED);

	m_bSky =			Params.GetValueBool(STR_SKY);
	m_strSkyTexture = wxString(Params.GetValueString(STR_SKYTEXTURE), wxConvUTF8);
	m_bOceanPlane =	 Params.GetValueBool(STR_OCEANPLANE);
	m_fOceanPlaneLevel = Params.GetValueFloat(STR_OCEANPLANELEVEL);
	m_bDepressOcean =   Params.GetValueBool(STR_DEPRESSOCEAN);
	m_fDepressOceanLevel = Params.GetValueFloat(STR_DEPRESSOCEANLEVEL);
	m_bHorizon =		Params.GetValueBool(STR_HORIZON);
	RGBi col =			Params.GetValueRGBi(STR_BGCOLOR);
	m_BgColor.Set(col.r, col.g, col.b);

	m_bRouteEnable =	Params.GetValueBool(STR_ROUTEENABLE);
	const char *routefile = Params.GetValueString(STR_ROUTEFILE);
	if (routefile)
		m_strRouteFile = wxString(routefile, wxConvUTF8);

	vtString fname;
	if (Params.GetOverlay(fname, m_iOverlayX, m_iOverlayY))
		m_strOverlayFile = wxString(fname, wxConvUTF8);
	m_bOverview =	Params.GetValueBool(STR_OVERVIEW);
	m_bCompass =	Params.GetValueBool(STR_COMPASS);

	// Scenarios
	m_strInitScenario = wxString(Params.GetValueString(STR_INIT_SCENARIO), wxConvUTF8);
	m_Scenarios = Params.m_Scenarios;

	// Safety checks
	if (m_iTriCount < 500 || m_iTriCount > 100000)
		m_iTriCount = 10000;
	if (m_fTextureLODFactor < .1f) m_fTextureLODFactor = .1f;
	if (m_fTextureLODFactor > 1) m_fTextureLODFactor = 1;

	VTLOG("   Finished SetParams\n");
}

//
// get the values from the dialog into the supplied paramter structure
//
void TParamsDlg::GetParams(TParams &Params)
{
	VTLOG("TParamsDlg::GetParams\n");
	LocaleWrap normal_numbers(LC_NUMERIC, "C");

	// overall name
	Params.SetValueString(STR_NAME, (const char *) m_strTerrainName.mb_str(wxConvUTF8));

	// elevation
	if (m_bGrid)
	{
		Params.SetValueInt(STR_SURFACE_TYPE, 0);
		Params.SetValueString(STR_ELEVFILE, (const char *) m_strFilename.mb_str(wxConvUTF8));
	}
	if (m_bTin)
	{
		Params.SetValueInt(STR_SURFACE_TYPE, 1);
		Params.SetValueString(STR_ELEVFILE, (const char *) m_strFilenameTin.mb_str(wxConvUTF8));
	}
	if (m_bTileset)
	{
		Params.SetValueInt(STR_SURFACE_TYPE, 2);
		Params.SetValueString(STR_ELEVFILE, (const char *) m_strFilenameTileset.mb_str(wxConvUTF8));
	}
	Params.SetValueFloat(STR_VERTICALEXAG, m_fVerticalExag);

	// navigation
	Params.SetValueFloat(STR_MINHEIGHT, m_fMinHeight);
	Params.SetValueInt(STR_NAVSTYLE, m_iNavStyle);
	Params.SetValueFloat(STR_NAVSPEED, m_fNavSpeed);
	Params.SetValueString(STR_LOCFILE, (const char *) m_strLocFile.mb_str(wxConvUTF8));
	Params.SetValueString(STR_INITLOCATION, (const char *) m_strInitLocation.mb_str(wxConvUTF8));
	Params.SetValueFloat(STR_HITHER, m_fHither);
	Params.SetValueBool(STR_ACCEL, m_bAccel);
	Params.SetValueBool(STR_ALLOW_ROLL, m_bAllowRoll);
	Params.m_AnimPaths = m_AnimPaths;

	// LOD
	Params.SetLodMethod((enum LodMethodEnum) m_iLodMethod);
	Params.SetValueInt(STR_TRICOUNT, m_iTriCount);
	Params.SetValueBool(STR_TRISTRIPS, m_bTriStrips);
	Params.SetValueInt(STR_VERTCOUNT, m_iVertCount);
	Params.SetValueInt(STR_TILE_CACHE_SIZE, m_iTileCacheSize);
	Params.SetValueBool(STR_TILE_THREADING, m_bTileThreading);

	// time
	Params.SetValueBool(STR_TIMEON, m_bTimeOn);
	Params.SetValueString(STR_INITTIME, m_InitTime.GetAsString());
	Params.SetValueFloat(STR_TIMESPEED, m_fTimeSpeed);

	// texture
	Params.SetTextureEnum((enum TextureEnum)m_iTexture);

	// single
	if (m_iTexture != TE_TILESET)
		Params.SetValueString(STR_TEXTUREFILE, (const char *) m_strTextureSingle.mb_str(wxConvUTF8));

	// tile4x4
	Params.SetValueInt(STR_TILESIZE, m_iTilesize);
	Params.SetValueString(STR_TEXTUREBASE, (const char *) m_strTextureBase.mb_str(wxConvUTF8));
	Params.SetValueString(STR_TEXTURE4BY4, (const char *) m_strTexture4x4.mb_str(wxConvUTF8));

	// derived
	Params.SetValueString(STR_COLOR_MAP, (const char *) m_strColorMap.mb_str(wxConvUTF8));

	// tileset
	if (m_iTexture == TE_TILESET)
		Params.SetValueString(STR_TEXTUREFILE, (const char *) m_strTextureTileset.mb_str(wxConvUTF8));

	Params.SetValueBool(STR_TEXTURE_GRADUAL, m_bTextureGradual);
	Params.SetValueFloat(STR_TEXURE_LOD_FACTOR, m_fTextureLODFactor);
	Params.SetValueBool(STR_MIPMAP, m_bMipmap);
	Params.SetValueBool(STR_REQUEST16BIT, m_b16bit);
	Params.SetValueBool(STR_PRELIGHT, m_bPreLight);
	Params.SetValueFloat(STR_PRELIGHTFACTOR, m_fPreLightFactor);
	Params.SetValueBool(STR_CAST_SHADOWS, m_bCastShadows);
	Params.SetValueBool(STR_TEXTURE_RETAIN, m_bTextureRetain);

	// detail texture
	Params.SetValueBool(STR_DETAILTEXTURE, m_bDetailTexture);
	Params.SetValueString(STR_DTEXTURE_NAME, (const char *) m_strDetailName.mb_str(wxConvUTF8));
	Params.SetValueFloat(STR_DTEXTURE_SCALE, m_fDetailScale);
	Params.SetValueFloat(STR_DTEXTURE_DISTANCE, m_fDetailDistance);

	// culture
	Params.SetValueBool(STR_ROADS, m_bRoads);
	Params.SetValueString(STR_ROADFILE, (const char *) m_strRoadFile.mb_str(wxConvUTF8));
	Params.SetValueBool(STR_HWY, m_bHwy);
	Params.SetValueBool(STR_PAVED, m_bPaved);
	Params.SetValueBool(STR_DIRT, m_bDirt);
	Params.SetValueFloat(STR_ROADHEIGHT, m_fRoadHeight);
	Params.SetValueFloat(STR_ROADDISTANCE, m_fRoadDistance);
	Params.SetValueBool(STR_TEXROADS, m_bTexRoads);
	Params.SetValueBool(STR_ROADCULTURE, m_bRoadCulture);

	Params.SetValueBool(STR_TREES, m_bPlants);
	Params.SetValueString(STR_TREEFILE, (const char *) m_strVegFile.mb_str(wxConvUTF8));
	Params.SetValueInt(STR_VEGDISTANCE, m_iVegDistance);

	Params.SetValueBool(STR_FOG, m_bFog);
	Params.SetValueFloat(STR_FOGDISTANCE, m_fFogDistance);
	// (fog color not exposed in UI)

	// Layers and structure stuff
	Params.SetValueString(STR_CONTENT_FILE, (const char *) m_strContent.mb_str(wxConvUTF8));
	Params.m_Layers = m_Layers;

	Params.SetValueInt(STR_STRUCTDIST, m_iStructDistance);
	Params.SetValueBool(STR_STRUCT_SHADOWS, m_bStructureShadows);
	// shadows
	Params.SetValueInt(STR_SHADOW_REZ, 1 << (m_iShadowRez+8));
	Params.SetValueFloat(STR_SHADOW_DARKNESS, m_fDarkness);
	Params.SetValueBool(STR_SHADOWS_DEFAULT_ON, m_bShadowsDefaultOn);
	Params.SetValueBool(STR_SHADOWS_EVERY_FRAME, m_bShadowsEveryFrame);
	Params.SetValueBool(STR_LIMIT_SHADOW_AREA, m_bLimitShadowArea);
	Params.SetValueFloat(STR_SHADOW_RADIUS, m_fShadowRadius);
	// paging
	Params.SetValueBool(STR_STRUCTURE_PAGING, m_bPagingStructures);
	Params.SetValueInt(STR_STRUCTURE_PAGING_MAX, m_iPagingStructureMax);
	Params.SetValueFloat(STR_STRUCTURE_PAGING_DIST, m_fPagingStructureDist);

//	Params.SetValueBool(STR_VEHICLES, m_bVehicles);
//  Params.SetValueFloat(STR_VEHICLESIZE, m_fVehicleSize);
//  Params.SetValueFloat(STR_VEHICLESPEED, m_fVehicleSpeed);

	Params.SetValueBool(STR_SKY, m_bSky);
	Params.SetValueString(STR_SKYTEXTURE, (const char *) m_strSkyTexture.mb_str(wxConvUTF8));

	Params.SetValueBool(STR_OCEANPLANE, m_bOceanPlane);
	Params.SetValueFloat(STR_OCEANPLANELEVEL, m_fOceanPlaneLevel);
	Params.SetValueBool(STR_DEPRESSOCEAN, m_bDepressOcean);
	Params.SetValueFloat(STR_DEPRESSOCEANLEVEL, m_fDepressOceanLevel);
	Params.SetValueBool(STR_HORIZON, m_bHorizon);
	RGBi col(m_BgColor.Red(), m_BgColor.Green(), m_BgColor.Blue());
	Params.SetValueRGBi(STR_BGCOLOR, col);

	Params.SetValueBool(STR_ROUTEENABLE, m_bRouteEnable);
	Params.SetValueString(STR_ROUTEFILE, (const char *) m_strRouteFile.mb_str(wxConvUTF8));

	// HUD
	Params.SetOverlay((const char *) m_strOverlayFile.mb_str(wxConvUTF8), m_iOverlayX, m_iOverlayY);
	Params.SetValueBool(STR_OVERVIEW, m_bOverview);
	Params.SetValueBool(STR_COMPASS, m_bCompass);

	// Scenarios
	Params.SetValueString(STR_INIT_SCENARIO, (const char *) m_strInitScenario.mb_str(wxConvUTF8));
	Params.m_Scenarios = m_Scenarios;
}

void TParamsDlg::UpdateFilenameBases()
{
	int totalsize = NTILES * (m_iTilesize-1) + 1;

	vtString filter;
	filter.Format("*%d.*", totalsize);
	vtString number;
	number.Format("%d", totalsize);

	GetTFileBase()->Clear();
	for (unsigned int i = 0; i < m_TextureFiles.size(); i++)
	{
		// fill the "single texture filename" control with available bitmap files
		if (m_TextureFiles[i].Matches(filter))
		{
			vtString s = m_TextureFiles[i];
			int offset = s.Find(number);
			if (offset != -1)
				s = s.Left(offset);
			wxString str(s, wxConvUTF8);
			GetTFileBase()->Append(str);
		}
	}
	if (GetTFileBase()->GetCount() == 0)
		GetTFileBase()->Append(_("<none>"));

	int sel = GetTFileBase()->FindString(m_strTextureBase);
	if (sel != -1)
		GetTFileBase()->SetSelection(sel);
	else
	{
		GetTFileBase()->SetSelection(0);
		m_strTextureBase = GetTFileBase()->GetString(0);
	}
}

void TParamsDlg::UpdateTiledTextureFilename()
{
	int totalsize = NTILES * (m_iTilesize-1) + 1;

	vtString filter;
	filter.Format("%s%d.*", (const char *) m_strTextureBase.mb_str(wxConvUTF8), totalsize);

	bool bFound = false;
	m_strTexture4x4 = _("<none>");
	for (unsigned int i = 0; i < m_TextureFiles.size(); i++)
	{
		// fill the "single texture filename" control with available bitmap files
		if (m_TextureFiles[i].Matches(filter))
		{
			m_strTexture4x4 = wxString(m_TextureFiles[i], wxConvUTF8);
			bFound = true;
			break;
		}
	}
	TransferDataToWindow();
}

void TParamsDlg::UpdateEnableState()
{
	GetFilename()->Enable(m_bGrid);
	GetFilenameTin()->Enable(m_bTin);
	GetFilenameTileset()->Enable(m_bTileset);

	FindWindow(ID_LODMETHOD)->Enable(m_bGrid);
	FindWindow(ID_TRI_COUNT)->Enable(m_bGrid && m_iLodMethod != LM_TOPOVISTA);
	FindWindow(ID_TRISTRIPS)->Enable(m_bGrid && m_iLodMethod == LM_MCNALLY);
	FindWindow(ID_VTX_COUNT)->Enable(m_bTileset);
	FindWindow(ID_TILE_CACHE_SIZE)->Enable(false);
	FindWindow(ID_TILE_THREADING)->Enable(m_bTileset);

	FindWindow(ID_NONE)->Enable(!m_bTileset);
	FindWindow(ID_SINGLE)->Enable(!m_bTileset);
	FindWindow(ID_DERIVED)->Enable(!m_bTileset);
	FindWindow(ID_TILED_4BY4)->Enable(!m_bTileset);
	FindWindow(ID_TILESET)->Enable(m_bTileset);

	FindWindow(ID_TFILE_SINGLE)->Enable(m_iTexture == TE_SINGLE);
	FindWindow(ID_CHOICE_COLORS)->Enable(m_iTexture == TE_DERIVED);
	FindWindow(ID_EDIT_COLORS)->Enable(m_iTexture == TE_DERIVED);
	FindWindow(ID_CHOICE_TILESIZE)->Enable(m_iTexture == TE_TILED);
	FindWindow(ID_TFILE_BASE)->Enable(m_iTexture == TE_TILED);
	FindWindow(ID_TFILENAME)->Enable(m_iTexture == TE_TILED);
	FindWindow(ID_TFILE_TILESET)->Enable(m_iTexture == TE_TILESET);
	FindWindow(ID_TEXTURE_GRADUAL)->Enable(m_iTexture == TE_TILESET && m_bTileThreading);
	FindWindow(ID_TEX_LOD)->Enable(m_iTexture == TE_TILESET);

	FindWindow(ID_MIPMAP)->Enable(m_iTexture != TE_NONE && !m_bTileset);
	FindWindow(ID_16BIT)->Enable(m_iTexture != TE_NONE && !m_bTileset);
	FindWindow(ID_PRELIGHT)->Enable(m_iTexture != TE_NONE && m_bGrid);
	FindWindow(ID_LIGHT_FACTOR)->Enable(m_iTexture != TE_NONE && m_bGrid);
	FindWindow(ID_CAST_SHADOWS)->Enable(m_iTexture != TE_NONE && m_bGrid);
	FindWindow(ID_RETAIN)->Enable(m_iTexture != TE_NONE && m_bGrid);

	FindWindow(ID_DETAILTEXTURE)->Enable(m_bGrid && m_iLodMethod == LM_MCNALLY);
	FindWindow(ID_DT_NAME)->Enable(m_bGrid && m_iLodMethod == LM_MCNALLY && m_bDetailTexture);
	FindWindow(ID_DT_SCALE)->Enable(m_bGrid && m_iLodMethod == LM_MCNALLY && m_bDetailTexture);
	FindWindow(ID_DT_DISTANCE)->Enable(m_bGrid && m_iLodMethod == LM_MCNALLY && m_bDetailTexture);

	FindWindow(ID_TREEFILE)->Enable(m_bPlants);
//  FindWindow(ID_VEGDISTANCE)->Enable(m_bPlants); // user might want to adjust

	FindWindow(ID_ROADFILE)->Enable(m_bRoads);
	FindWindow(ID_ROADHEIGHT)->Enable(m_bRoads);
	FindWindow(ID_ROADDISTANCE)->Enable(m_bRoads);
	FindWindow(ID_TEXROADS)->Enable(m_bRoads);
	FindWindow(ID_ROADCULTURE)->Enable(m_bRoads);
	FindWindow(ID_HIGHWAYS)->Enable(m_bRoads);
	FindWindow(ID_PAVED)->Enable(m_bRoads);
	FindWindow(ID_DIRT)->Enable(m_bRoads);

	FindWindow(ID_CHOICE_SHADOW_REZ)->Enable(m_bStructureShadows);
	FindWindow(ID_DARKNESS)->Enable(m_bStructureShadows);
	FindWindow(ID_SHADOWS_DEFAULT_ON)->Enable(m_bStructureShadows);
	FindWindow(ID_SHADOWS_EVERY_FRAME)->Enable(m_bStructureShadows);
	FindWindow(ID_SHADOW_LIMIT)->Enable(m_bStructureShadows);
	FindWindow(ID_SHADOW_LIMIT_RADIUS)->Enable(m_bStructureShadows && m_bLimitShadowArea);
	FindWindow(ID_PAGING_MAX_STRUCTURES)->Enable(m_bPagingStructures);
	FindWindow(ID_PAGE_OUT_DISTANCE)->Enable(m_bPagingStructures);

	GetOceanPlaneOffset()->Enable(m_bOceanPlane);
	GetDepressOceanOffset()->Enable(m_bDepressOcean);
	GetSkytexture()->Enable(m_bSky);
	GetSkytexture()->Enable(m_bSky);
	GetFogDistance()->Enable(m_bFog);
	int iSelected = m_pScenarioList->GetSelection();
	if (iSelected != wxNOT_FOUND)
	{
		GetEditScenario()->Enable(true);
		GetDeleteScenario()->Enable(true);
		if (iSelected != (m_pScenarioList->GetCount() - 1))
			GetMovedownScenario()->Enable(true);
		else
			GetMovedownScenario()->Enable(false);
	   if (iSelected != 0)
			GetMoveupScenario()->Enable(true);
		else
			GetMoveupScenario()->Enable(false);
	}
	else
	{
		GetEditScenario()->Enable(false);
		GetDeleteScenario()->Enable(false);
		GetMoveupScenario()->Enable(false);
		GetMovedownScenario()->Enable(false);
	}
}

void TParamsDlg::RefreshLocationFields()
{
	m_pLocField->Clear();
	m_pLocField->Append(_("(default)"));

	vtString fname = "Locations/";
	fname += m_strLocFile.mb_str(wxConvUTF8);
	vtString path = FindFileOnPaths(vtGetDataPath(), fname);
	if (path == "")
		return;
	vtLocationSaver saver;
	if (!saver.Read(path))
		return;

	int i, num = saver.GetNumLocations();
	for (i = 0; i < num; i++)
	{
		vtLocation *loc = saver.GetLocation(i);
		wxString str(loc->m_strName.c_str());
		m_pLocField->Append(str);
	}
	if (num)
	{
		if (m_iInitLocation < 0)
			m_iInitLocation = 0;
		if (m_iInitLocation > num-1)
			m_iInitLocation = num-1;
	}
}

void TParamsDlg::UpdateColorMapChoice()
{
	m_pColorMap->Clear();
	vtStringArray &paths = vtGetDataPath();
	for (unsigned int i = 0; i < paths.size(); i++)
	{
		// fill the "colormap" control with available colormap files
		AddFilenamesToChoice(m_pColorMap, paths[i] + "GeoTypical", "*.cmt");
		int sel = m_pColorMap->FindString(m_strColorMap);
		if (sel != -1)
			m_pColorMap->SetSelection(sel);
	}
}

void TParamsDlg::DeleteItem(wxListBox *pBox)
{
	vtString fname = (const char *) pBox->GetStringSelection().mb_str(wxConvUTF8);

	// might be a layer
	int idx = FindLayerByFilename(fname);
	if (idx != -1)
		m_Layers.erase(m_Layers.begin()+idx);

	// or an animpath
	for (unsigned int i = 0; i < m_AnimPaths.size(); i++)
	{
		if (!fname.Compare(m_AnimPaths[i]))
		{
			m_AnimPaths.erase(m_AnimPaths.begin()+i);
			break;
		}
	}
}

int TParamsDlg::FindLayerByFilename(const vtString &fname)
{
	for (unsigned int i = 0; i < m_Layers.size(); i++)
		if (fname == m_Layers[i].GetValueString("Filename"))
			return (int) i;
	return -1;
}


// WDR: handler implementations for TParamsDlg

void TParamsDlg::OnBgColor( wxCommandEvent &event )
{
	wxColourData data;
	data.SetChooseFull(true);
	data.SetColour(m_BgColor);

	wxColourDialog dlg(this, &data);
	if (dlg.ShowModal() == wxID_OK)
	{
		wxColourData data2 = dlg.GetColourData();
		m_BgColor = data2.GetColour();
		UpdateColorControl();
	}
}

void TParamsDlg::OnTextureFileBase( wxCommandEvent &event )
{
	if (m_bSetting || !m_bReady) return;
	TransferDataFromWindow();
	UpdateTiledTextureFilename();
}

void TParamsDlg::OnTileSize( wxCommandEvent &event )
{
	if (m_bSetting || !m_bReady) return;
	TransferDataFromWindow();
	m_iTilesize = 1 << (m_iTilesizeIndex + 8);
	UpdateFilenameBases();
	UpdateTiledTextureFilename();
}

void TParamsDlg::OnInitDialog(wxInitDialogEvent& event)
{
	VTLOG("TParamsDlg::OnInitDialog\n");

	bool bShowProgress = (vtGetDataPath().size() > 1);
	if (bShowProgress)
		OpenProgressDialog(_("Looking for files on data paths"), false, this);

	m_bReady = false;
	m_bSetting = true;

	unsigned int i;
	int sel;

	// Clear drop-down controls before putting values into them
	m_pFilename->Clear();
	m_pFilenameTin->Clear();
	m_pFilenameTileset->Clear();
	m_pTextureFileSingle->Clear();
	m_pTextureFileTileset->Clear();

	vtStringArray &paths = vtGetDataPath();

	for (i = 0; i < paths.size(); i++)
	{
		if (bShowProgress)
			UpdateProgressDialog(i * 100 / paths.size(), wxString(paths[i], wxConvUTF8));

		// Gather all possible texture image filenames
		AddFilenamesToStringArray(m_TextureFiles, paths[i] + "GeoSpecific", "*.bmp");
		AddFilenamesToStringArray(m_TextureFiles, paths[i] + "GeoSpecific", "*.jpg");
		AddFilenamesToStringArray(m_TextureFiles, paths[i] + "GeoSpecific", "*.jpeg");
		AddFilenamesToStringArray(m_TextureFiles, paths[i] + "GeoSpecific", "*.png");
		AddFilenamesToStringArray(m_TextureFiles, paths[i] + "GeoSpecific", "*.tif");

		// fill the "Grid filename" control with available files
		AddFilenamesToComboBox(m_pFilename, paths[i] + "Elevation", "*.bt*");
		sel = m_pFilename->FindString(m_strFilename);
		if (sel != -1)
			m_pFilename->SetSelection(sel);

		// fill the "TIN filename" control with available files
		AddFilenamesToComboBox(m_pFilenameTin, paths[i] + "Elevation", "*.tin");
		AddFilenamesToComboBox(m_pFilenameTin, paths[i] + "Elevation", "*.itf");
		sel = m_pFilenameTin->FindString(m_strFilenameTin);
		if (sel != -1)
			m_pFilenameTin->SetSelection(sel);

		// fill the "Tileset filename" control with available files
		AddFilenamesToComboBox(m_pFilenameTileset, paths[i] + "Elevation", "*.ini");
		sel = m_pFilenameTileset->FindString(m_strFilenameTileset);
		if (sel != -1)
			m_pFilenameTileset->SetSelection(sel);

		// fill the "texture Tileset filename" control with available files
		AddFilenamesToComboBox(m_pTextureFileTileset, paths[i] + "GeoSpecific", "*.ini");
		sel = m_pTextureFileTileset->FindString(m_strTextureTileset);
		if (sel != -1)
			m_pTextureFileTileset->SetSelection(sel);

		// fill the "detail texture" control with available bitmap files
		AddFilenamesToComboBox(m_pDTName, paths[i] + "GeoTypical", "*.bmp");
		AddFilenamesToComboBox(m_pDTName, paths[i] + "GeoTypical", "*.jpg");
		AddFilenamesToComboBox(m_pDTName, paths[i] + "GeoTypical", "*.png");
		sel = m_pDTName->FindString(m_strDetailName);
		if (sel != -1)
			m_pDTName->SetSelection(sel);

		// fill the Location files
		AddFilenamesToComboBox(m_pLocFile, paths[i] + "Locations", "*.loc");
		sel = m_pLocFile->FindString(m_strLocFile);
		if (sel != -1)
			m_pLocFile->SetSelection(sel);

		// fill in Road files
		AddFilenamesToComboBox(m_pRoadFile, paths[i] + "RoadData", "*.rmf");
		sel = m_pRoadFile->FindString(m_strRoadFile);
		if (sel != -1)
			m_pRoadFile->SetSelection(sel);

		// fill in Vegetation files
		AddFilenamesToComboBox(m_pTreeFile, paths[i] + "PlantData", "*.vf");
		AddFilenamesToComboBox(m_pTreeFile, paths[i] + "PlantData", "*.shp");
		sel = m_pTreeFile->FindString(m_strVegFile);
		if (sel != -1)
			m_pTreeFile->SetSelection(sel);

		// fill in Content file
		AddFilenamesToComboBox(GetContentFile(), paths[i], "*.vtco");
		sel = GetContentFile()->FindString(m_strContent);
		if (sel != -1)
			GetContentFile()->SetSelection(sel);

		// fill in Sky files
		AddFilenamesToComboBox(m_pSkyTexture, paths[i] + "Sky", "*.bmp");
		AddFilenamesToComboBox(m_pSkyTexture, paths[i] + "Sky", "*.png");
		AddFilenamesToComboBox(m_pSkyTexture, paths[i] + "Sky", "*.jpg");
		sel = m_pSkyTexture->FindString(m_strSkyTexture);
		if (sel != -1)
			m_pSkyTexture->SetSelection(sel);
	}

	// fill the "single texture filename" control with available image files
	for (i = 0; i < m_TextureFiles.size(); i++)
	{
		wxString str(m_TextureFiles[i], wxConvUTF8);
		m_pTextureFileSingle->Append(str);
	}
	sel = m_pTextureFileSingle->FindString(m_strTextureSingle);
	if (sel != -1)
		m_pTextureFileSingle->SetSelection(sel);

	UpdateColorMapChoice();

	m_pLodMethod->Clear();
	m_pLodMethod->Append(_T("Roettger"));
	m_pLodMethod->Append(_T("TopoVista"));
	m_pLodMethod->Append(_T("McNally"));
	m_pLodMethod->Append(_T("Demeter"));
	m_pLodMethod->Append(_T("Custom"));
	m_pLodMethod->Append(_T("BryanQuad"));
	// add your own LOD method here!

	m_pLodMethod->SetSelection(m_iLodMethod);

	m_pShadowRez->Clear();
	m_pShadowRez->Append(_T("256"));
	m_pShadowRez->Append(_T("512"));
	m_pShadowRez->Append(_T("1024"));
	m_pShadowRez->Append(_T("2048"));
	m_pShadowRez->Append(_T("4096"));

	m_pNavStyle->Clear();
	m_pNavStyle->Append(_("Normal Terrain Flyer"));
	m_pNavStyle->Append(_("Terrain Flyer with Velocity"));
	m_pNavStyle->Append(_("Grab-Pivot"));
//  m_pNavStyle->Append(_("Quake-Style Walk"));
	m_pNavStyle->Append(_("Panoramic Flyer"));

	RefreshLocationFields();

//  DetermineTerrainSizeFromBT();
//  DetermineSizeFromBMP();

//  OnChangeMem();
	UpdateFilenameBases();
	UpdateTiledTextureFilename();

	GetUseGrid()->SetValue(m_bGrid);
	GetUseTin()->SetValue(m_bTin);
	GetUseTileset()->SetValue(m_bTileset);

	UpdateTimeString();

	m_iInitLocation = m_pLocField->FindString(m_strInitLocation);
	if (m_iInitLocation == -1)
		m_iInitLocation = 0;

	UpdateColorControl();

	UpdateScenarioChoices();

	wxWindow::OnInitDialog(event);

	UpdateEnableState();

	if (bShowProgress)
		CloseProgressDialog();

	m_bReady = true;
}

bool TParamsDlg::TransferDataToWindow()
{
	m_bSetting = true;

	m_pNone->SetValue(m_iTexture == TE_NONE);
	m_pSingle->SetValue(m_iTexture == TE_SINGLE);
	m_pDerived->SetValue(m_iTexture == TE_DERIVED);
	m_pTiled->SetValue(m_iTexture == TE_TILED);
	m_pTileset->SetValue(m_iTexture == TE_TILESET);

	unsigned int i;
	m_pStructFiles->Clear();
	m_pRawFiles->Clear();
	m_pImageFiles->Clear();
	for (i = 0; i < m_Layers.size(); i++)
	{
		vtString ltype = m_Layers[i].GetValueString("Type");
		vtString fname = m_Layers[i].GetValueString("Filename");
		wxString fname2(fname, wxConvUTF8);

		if (ltype == TERR_LTYPE_STRUCTURE)
			m_pStructFiles->Append(fname2);
		if (ltype == TERR_LTYPE_ABSTRACT)
			m_pRawFiles->Append(fname2);
		if (ltype == TERR_LTYPE_IMAGE)
			m_pImageFiles->Append(fname2);
	}
	m_pStructFiles->Append(_("(double-click to add files)"));
	m_pRawFiles->Append(_("(double-click to add files)"));
	m_pImageFiles->Append(_("(double-click to add files)"));

	m_pAnimFiles->Clear();
	for (i = 0; i < m_AnimPaths.size(); i++)
		m_pAnimFiles->Append(wxString(m_AnimPaths[i], wxConvUTF8));
	m_pAnimFiles->Append(_("(double-click to add files)"));

	m_pScenarioList->Clear();
	for (i = 0; i < m_Scenarios.size(); i++)
	{
		wxString str(m_Scenarios[i].GetValueString(STR_SCENARIO_NAME), wxConvUTF8);
		m_pScenarioList->Append(str);
	}

	bool result = wxDialog::TransferDataToWindow();
	m_bSetting = false;

	return result;
}

bool TParamsDlg::TransferDataFromWindow()
{
	if (m_pNone->GetValue()) m_iTexture = TE_NONE;
	if (m_pSingle->GetValue()) m_iTexture = TE_SINGLE;
	if (m_pDerived->GetValue()) m_iTexture = TE_DERIVED;
	if (m_pTiled->GetValue()) m_iTexture = TE_TILED;
	if (m_pTileset->GetValue()) m_iTexture = TE_TILESET;

	return wxDialog::TransferDataFromWindow();
}

void TParamsDlg::UpdateColorControl()
{
	FillWithColor(GetColorBitmap(), m_BgColor);
}

void TParamsDlg::OnTextureNone( wxCommandEvent &event )
{
	if (m_bSetting)
		return;
	if (event.IsChecked())
	{
		// turn off "Prelighting" if there is no texture
		TransferDataFromWindow();
		m_bPreLight = false;
		TransferDataToWindow();
	}
	UpdateEnableState();
}

void TParamsDlg::OnTextureSingle( wxCommandEvent &event )
{
	if (m_bSetting || !event.IsChecked())
		return;
	TransferDataFromWindow();
	UpdateEnableState();
}

void TParamsDlg::OnTextureDerived( wxCommandEvent &event )
{
	if (m_bSetting)
		return;
	if (event.IsChecked())
	{
		// turn on "Prelighting" if the user wants a derived texture
		TransferDataFromWindow();
		m_bPreLight = true;
		TransferDataToWindow();
	}
	UpdateEnableState();
}

void TParamsDlg::OnTextureTiled( wxCommandEvent &event )
{
	if (m_bSetting || !event.IsChecked())
		return;
	TransferDataFromWindow();
	UpdateEnableState();
}

void TParamsDlg::OnTextureTileset( wxCommandEvent &event )
{
	if (m_bSetting || !event.IsChecked())
		return;
	TransferDataFromWindow();
	UpdateEnableState();
}

void TParamsDlg::OnComboTFileSingle( wxCommandEvent &event )
{
	if (m_bSetting)
		return;
	TransferDataFromWindow();
}

void TParamsDlg::OnEditColors( wxCommandEvent &event )
{
	TransferDataFromWindow();

	if (m_strColorMap.IsEmpty())
	{
		wxMessageBox(_("Please select a filename."));
		return;
	}

	// Look on data paths, to give a complete path to the dialog
	vtString name = "GeoTypical/";
	name += m_strColorMap.mb_str(wxConvUTF8);
	name = FindFileOnPaths(vtGetDataPath(), name);
	if (name == "")
	{
		wxMessageBox(_("Couldn't locate file."));
		return;
	}

	ColorMapDlg dlg(this, -1, _("ColorMap"));
	dlg.SetFile(name);
	dlg.ShowModal();

	// They may have added or removed some color map files on the data path
	UpdateColorMapChoice();
}

void TParamsDlg::OnCheckBoxElevType( wxCommandEvent &event )
{
	TransferDataFromWindow();

	// the tileset elevation and tileset texture only work with each other
	if (event.GetId() == ID_USE_TILESET && event.IsChecked())
		m_iTexture = TE_TILESET;
	if (event.GetId() != ID_USE_TILESET && event.IsChecked() && m_iTexture == TE_TILESET)
		m_iTexture = TE_NONE;

	UpdateEnableState();
	UpdateTiledTextureFilename();
}

void TParamsDlg::OnCheckBox( wxCommandEvent &event )
{
	TransferDataFromWindow();
	UpdateEnableState();
	UpdateTiledTextureFilename();
}

//
// This function is used to find all files in a given directory,
// and if they match a wildcard, add them to a string array.
//
void AddFilenamesToArray(wxArrayString &array, const wxString &dirname,
	const wxString &wildcard)
{
	// We could just call this:
	// wxDir::GetAllFiles(dirname, &array, wildcard, wxDIR_FILES);
	// However, that gets the full path names.  We only want the filenames.

	if (!wxDir::Exists(dirname))
		return;
	wxDir dir(dirname);
	wxString filename;
	bool cont = dir.GetFirst(&filename, wildcard, wxDIR_FILES);
	while ( cont )
	{
		array.Add(filename);
		cont = dir.GetNext(&filename);
	}
}

void TParamsDlg::OnListDblClickStructure( wxCommandEvent &event )
{
	unsigned int i;
	wxArrayString strings;

	for (i = 0; i < vtGetDataPath().size(); i++)
	{
		wxString path(vtGetDataPath()[i], wxConvUTF8);
		path += _T("BuildingData");
		AddFilenamesToArray(strings, path, _T("*.vtst*"));
	}

	wxString result = wxGetSingleChoice(_("One of the following to add:"),
		_("Choose a structure file"), strings, this);

	if (result.Cmp(_T(""))) // user selected something
	{
		TransferDataFromWindow();
		vtTagArray lay;
		lay.SetValueString("Type", TERR_LTYPE_STRUCTURE, true);
		lay.SetValueString("Filename", (const char *) result.mb_str(wxConvUTF8), true);
		m_Layers.push_back(lay);
		TransferDataToWindow();
	}
}

void TParamsDlg::OnListDblClickRaw( wxCommandEvent &event )
{
	unsigned int i;
	wxArrayString strings;

	for (i = 0; i < vtGetDataPath().size(); i++)
	{
		wxString path(vtGetDataPath()[i], *wxConvCurrent);
		path += _T("PointData");
		AddFilenamesToArray(strings, path, _T("*.shp"));
		AddFilenamesToArray(strings, path, _T("*.igc"));
		AddFilenamesToArray(strings, path, _T("*.dxf"));
	}

	wxString result = wxGetSingleChoice(_("One of the following to add:"), _("Choose a structure file"),
		strings, this);

	if (result.Cmp(_T(""))) // user selected something
	{
		vtTagArray lay;
		lay.SetValueString("Type", TERR_LTYPE_ABSTRACT, true);
		lay.SetValueString("Filename", (const char *) result.mb_str(wxConvUTF8), true);
		m_Layers.push_back(lay);
		TransferDataToWindow();
	}
}

void TParamsDlg::OnListDblClickAnimPaths( wxCommandEvent &event )
{
	unsigned int i;
	wxArrayString strings;

	for (i = 0; i < vtGetDataPath().size(); i++)
	{
		wxString path(vtGetDataPath()[i], *wxConvCurrent);
		path += _T("Locations");
		AddFilenamesToArray(strings, path, _T("*.vtap"));
	}

	wxString result = wxGetSingleChoice(_("One of the following to add:"), _("Choose an animpath file"),
		strings, this);

	if (result.Cmp(_T(""))) // user selected something
	{
		m_AnimPaths.push_back(vtString(result.mb_str(wxConvUTF8)));
		TransferDataToWindow();
	}
}

void TParamsDlg::OnListDblClickImage( wxCommandEvent &event )
{
	unsigned int i;
	wxArrayString strings;

	for (i = 0; i < vtGetDataPath().size(); i++)
	{
		wxString path(vtGetDataPath()[i], wxConvUTF8);
		path += _T("GeoSpecific");
		AddFilenamesToArray(strings, path, _T("*.bmp"));
		AddFilenamesToArray(strings, path, _T("*.jpg"));
		AddFilenamesToArray(strings, path, _T("*.jpeg"));
		AddFilenamesToArray(strings, path, _T("*.png"));
		AddFilenamesToArray(strings, path, _T("*.tif"));
	}

	wxString result = wxGetSingleChoice(_("One of the following to add:"),
		_("Choose an image file"), strings, this);

	if (result.Cmp(_T(""))) // user selected something
	{
		TransferDataFromWindow();
		vtTagArray lay;
		lay.SetValueString("Type", TERR_LTYPE_IMAGE, true);
		lay.SetValueString("Filename", (const char *) result.mb_str(wxConvUTF8), true);
		m_Layers.push_back(lay);
		TransferDataToWindow();
	}
}

void TParamsDlg::OnChoiceLocFile( wxCommandEvent &event )
{
	if (m_bSetting || !m_bReady) return;

	wxString prev = m_strLocFile;
	TransferDataFromWindow();
	if (m_strLocFile != prev)
	{
		RefreshLocationFields();
		m_bSetting = true;
		TransferDataToWindow();
		m_bSetting = false;
	}
}

void TParamsDlg::OnChoiceInitLocation( wxCommandEvent &event )
{
	TransferDataFromWindow();
	m_strInitLocation = m_pLocField->GetString(m_iInitLocation);
}

void TParamsDlg::OnSetInitTime( wxCommandEvent &event )
{
	TransferDataFromWindow();

	TimeDlg dlg(this, -1, _("Set Initial Time"));
	dlg.AddOkCancel();
	dlg.SetTime(m_InitTime);
	if (dlg.ShowModal() == wxID_OK)
	{
		dlg.GetTime(m_InitTime);
		UpdateTimeString();
		m_bSetting = true;
		TransferDataToWindow();
		m_bSetting = false;
	}
}

void TParamsDlg::OnStyle( wxCommandEvent &event )
{
	vtString str = (const char *) GetRawFiles()->GetStringSelection().mb_str(wxConvUTF8);
	int idx = FindLayerByFilename(str);
	if (idx == -1)
		return;

	StyleDlg dlg(this, -1, _("Feature Style"), wxDefaultPosition,
		wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
	dlg.SetOptions(m_Layers[idx]);
	if (dlg.ShowModal() == wxID_OK)
	{
		dlg.GetOptions(m_Layers[idx]);
	}
}

void TParamsDlg::OnOverlay( wxCommandEvent &event )
{
	TransferDataFromWindow();

	wxFileDialog loadFile(NULL, _("Overlay Image File"), _T(""), _T(""),
		_("Image Files (*.png,*.jpg,*.bmp)|*.png;*.jpg;*.bmp"), wxFD_OPEN);
	if (m_strOverlayFile != _T(""))
		loadFile.SetPath(m_strOverlayFile);
	bool bResult = (loadFile.ShowModal() == wxID_OK);
	if (!bResult)
		return;
	m_strOverlayFile = loadFile.GetPath();

	m_bSetting = true;
	TransferDataToWindow();
	m_bSetting = false;
}

void TParamsDlg::UpdateTimeString()
{
	m_strInitTime = wxString(asctime(&m_InitTime.GetTM()), wxConvLibc);

	// asctime has a weird habit of putting a LF at the end
	m_strInitTime.Trim();
}

void TParamsDlg::UpdateScenarioChoices()
{
	GetScenarios()->Clear();
	for (unsigned int i = 0; i < m_Scenarios.size(); i++)
	{
		vtString vs = m_Scenarios[i].GetValueString(STR_SCENARIO_NAME);
		GetScenarios()->Append(wxString(vs, wxConvUTF8));
	}
	GetScenarios()->SetStringSelection(m_strInitScenario);
}

void TParamsDlg::OnNewScenario( wxCommandEvent &event )
{
	wxString ScenarioName = wxGetTextFromUser(_("Enter Scenario Name"),
		_("New Scenario"));

	if (!ScenarioName.IsEmpty())
	{
		ScenarioParams Scenario;

		Scenario.SetValueString(STR_SCENARIO_NAME,
			(const char *) ScenarioName.mb_str(wxConvUTF8), true);
		m_Scenarios.push_back(Scenario);
		m_pScenarioList->SetSelection(m_pScenarioList->Append(ScenarioName));
		UpdateScenarioChoices();
		UpdateEnableState();
	}
}

void TParamsDlg::OnDeleteScenario( wxCommandEvent &event )
{
	int iSelected = m_pScenarioList->GetSelection();

	if (iSelected != wxNOT_FOUND)
	{
		m_pScenarioList->Delete(iSelected);
		m_Scenarios.erase(m_Scenarios.begin() + iSelected);
		UpdateScenarioChoices();
		UpdateEnableState();
	}
}

void TParamsDlg::OnEditScenario( wxCommandEvent &event )
{
	CScenarioParamsDialog ScenarioParamsDialog(this, -1, _("Scenario Parameters"));
	int iSelected = m_pScenarioList->GetSelection();

	if (iSelected != wxNOT_FOUND)
	{
		vtStringArray lnames;
		for (unsigned int i = 0; i < m_Layers.size(); i++)
			lnames.push_back(m_Layers[i].GetValueString("Filename"));

		ScenarioParamsDialog.SetAvailableLayers(lnames);
		ScenarioParamsDialog.SetParams(m_Scenarios[iSelected]);

		if (wxID_OK == ScenarioParamsDialog.ShowModal())
		{
			if (ScenarioParamsDialog.IsModified())
			{
				wxString str(m_Scenarios[iSelected].GetValueString(STR_SCENARIO_NAME), wxConvUTF8);
				m_Scenarios[iSelected] = ScenarioParamsDialog.GetParams();
				m_pScenarioList->SetString(iSelected, str);
			}
		}
	}
}

void TParamsDlg::OnMoveUpScenario( wxCommandEvent &event )
{
	int iSelected = m_pScenarioList->GetSelection();

	if ((iSelected != wxNOT_FOUND) && (iSelected != 0))
	{
		ScenarioParams TempParams = m_Scenarios[iSelected];
		wxString TempString = m_pScenarioList->GetString(iSelected);
		m_pScenarioList->Delete(iSelected);
// Bug in wxWindows
//		m_pScenarioList->SetSelection(m_pScenarioList->Insert(TempString, iSelected - 1));
		m_pScenarioList->Insert(TempString, iSelected - 1);
		m_pScenarioList->SetSelection(iSelected - 1);
		m_Scenarios.erase(m_Scenarios.begin() + iSelected);
		m_Scenarios.insert(m_Scenarios.begin() + iSelected - 1,TempParams);
		UpdateEnableState();
	}
}

void TParamsDlg::OnMoveDownSceanario( wxCommandEvent &event )
{
	int iSelected = m_pScenarioList->GetSelection();

	if ((iSelected != wxNOT_FOUND) && (iSelected != (m_pScenarioList->GetCount() - 1)))
	{
		ScenarioParams TempParams = m_Scenarios[iSelected];
		wxString TempString = m_pScenarioList->GetString(iSelected);
		m_pScenarioList->Delete(iSelected);
// Bug in wxWindows
//		m_pScenarioList->SetSelection(m_pScenarioList->Insert(TempString, iSelected + 1));
		m_pScenarioList->Insert(TempString, iSelected + 1);
		m_pScenarioList->SetSelection(iSelected + 1);
		m_Scenarios.erase(m_Scenarios.begin() + iSelected);
		m_Scenarios.insert(m_Scenarios.begin() + iSelected + 1,TempParams);
		UpdateEnableState();
	}
}

void TParamsDlg::OnScenarioListEvent( wxCommandEvent &event )
{
	UpdateEnableState();
}

void TParamsDlg::OnChoiceScenario( wxCommandEvent &event )
{
	m_strInitScenario = GetScenarios()->GetStringSelection();
}

