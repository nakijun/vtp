//
// class Enviro: Main functionality of the Enviro application
//
// Copyright (c) 2001-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#include "vtlib/vtlib.h"
#include "vtlib/core/Fence3d.h"
#include "vtlib/core/Globe.h"
#include "vtlib/core/SkyDome.h"
#include "vtlib/core/Building3d.h"
#include "vtlib/core/PagedLodGrid.h"
#include "vtlib/core/TiledGeom.h"
#include "vtdata/vtLog.h"
#include "vtdata/PolyChecker.h"

#include "Engines.h"
#include "Enviro.h"
#include "Options.h"
#include "Hawaii.h"
#include "Nevada.h"
#include "SpecificTerrain.h"
#include "CarEngine.h"

// Although there is no string translation in the core of Enviro (because it
//  is independent of wx or any GUI library) nonetheless we want the text
//  messages to be found by the gettext utility, so we need to enclose
//  anything to be translated in _()
#define _(x) x

#define ORTHO_HITHER	50	// 50m above highest point on terrain


///////////////////////////////////////////////////////////

Enviro *Enviro::s_pEnviro = NULL;

Enviro::Enviro() : vtTerrainScene()
{
	s_pEnviro = this;

	m_mode = MM_NONE;
	m_state = AS_Initializing;
	m_iInitStep = 0;

	m_bActiveFence = false;
	m_pCurFence = NULL;
	m_FenceParams.Defaults();

	m_bOnTerrain = false;
	m_bEarthShade = false;

	m_pGlobeContainer = NULL;
	m_bGlobeFlat = false;
	m_fFlattening = 1.0f;
	m_fFlattenDir = 0.0f;
	m_bGlobeUnfolded = false;
	m_fFolding = 0.0f;
	m_fFoldDir = 0.0f;
	m_pIcoGlobe = NULL;
	m_pOverlayGlobe = NULL;
	m_pSpaceAxes = NULL;
	m_pEarthLines = NULL;

	m_bTopDown = false;
	m_pTopDownCamera = NULL;
	m_pTerrainPicker = NULL;
	m_pGlobePicker = NULL;
	m_pCursorMGeom = NULL;

	m_pArc = NULL;
	m_pArcMats = NULL;
	m_fArcLength = 0.0;
	m_fDistToolHeight = 5.0f;
	m_bMeasurePath = false;

	m_fMessageTime = 0.0f;
	m_pHUD = NULL;
	m_pHUDMessage = NULL;
	m_pArial = NULL;

	// plants
	m_pPlantList = NULL;
	m_bPlantsLoaded = false;
	m_PlantOpt.m_iMode = 0;
	m_PlantOpt.m_iSpecies = -1;
	m_PlantOpt.m_fHeight = 100.0f;
	m_PlantOpt.m_iVariance = 20;
	m_PlantOpt.m_fSpacing = 2.0f;

	m_bDragging = false;
	m_bDragUpDown = false;
	m_bSelectedStruct = false;
	m_bSelectedPlant = false;
	m_bSelectedVehicle = false;

	// HUD
	m_pHUDMaterials = NULL;
	m_pLegendGeom = NULL;
	m_bCreatedLegend = false;

	m_pCompassSizer = NULL;
	m_pCompassGeom = NULL;
	m_bCreatedCompass = false;
	m_bDragCompass = false;

	m_pWindowBoxMesh = NULL;

	m_pMapOverview = NULL;
	m_bFlyIn = false;
	m_iFlightStage = 0;
}

Enviro::~Enviro()
{
}

void Enviro::Startup()
{
	VTSTARTLOG("debug.txt");
	VTLOG1("\nEnviro\nBuild:");
#if VTDEBUG
	VTLOG1(" Debug");
#else
	VTLOG1(" Release");
#endif
#if UNICODE
	VTLOG1(" Unicode");
#endif
	VTLOG("\n\n");

	// set up the datum list we will use
	SetupEPSGDatums();
}

void Enviro::Shutdown()
{
	VTLOG1("Shutdown.\n");

	delete m_pArial;
	delete m_pPlantList;
	if (m_pArcMats)
		m_pArcMats->Release();
	if (m_pHUDMaterials)
		m_pHUDMaterials->Release();
	if (m_pTopDownCamera)
		m_pTopDownCamera->Release();

	// Clean up the rest of the TerrainScene container
	vtGetScene()->SetRoot(NULL);
	CleanupScene();

	delete m_pIcoGlobe;
	delete m_pOverlayGlobe;
}

void Enviro::StartupArgument(int i, const char *str)
{
	VTLOG("Command line %d: %s\n", i, str);

	if (!strcmp(str, "-fullscreen"))
		g_Options.m_bFullscreen = true;

	else if(!strncmp(str, "-terrain=", 9))
		g_Options.m_strInitTerrain = str+9;

	else if(!strncmp(str, "-location=", 10))
	{
		g_Options.m_strInitLocation = str+10;
		// trim quotes
		g_Options.m_strInitLocation.Remove('\"');
	}

	else if(!strncmp(str, "-neutral", 8))
		g_Options.m_bStartInNeutral = true;
}

void Enviro::LoadAllTerrainDescriptions()
{
	VTLOG("LoadAllTerrainDescriptions...\n");

	for (unsigned int i = 0; i < vtGetDataPath().size(); i++)
		LoadTerrainDescriptions(vtGetDataPath()[i]);

	VTLOG(" Done.\n");
}

void Enviro::LoadTerrainDescriptions(const vtString &path)
{
	int count = 0;
	VTLOG("  On path '%s':\n", (const char *) path);

	vtString directory = path + "Terrains";
	for (dir_iter it((const char *)directory); it != dir_iter(); ++it)
	{
		//VTLOG("\t file: %s\n", it.filename().c_str());
		if (it.is_hidden() || it.is_directory())
			continue;

		std::string name1 = it.filename();
		vtString name = name1.c_str();

		// Only look for ".xml" files which describe a terrain
		vtString ext = GetExtension(name, false);
		if (ext.CompareNoCase(".xml") != 0)
			continue;

		// Some terrain .xml files want to use a different Terrain class
		int dot = name.Find('.');
		vtString before_dot;
		if (dot == -1)
			before_dot = name;
		else
			before_dot = name.Left(dot);

		// This is where you can tell Enviro to contruct your own terrain
		//  class, for a particular config file, rather than the default
		//  vtTerrain.
		vtTerrain *pTerr;
		if (before_dot == "Hawai`i" || before_dot == "Hawai'i" ||
			before_dot == "Hawaii" || before_dot == "Honoka'a" ||
			before_dot == "Kealakekua" || before_dot == "Kamuela")
			pTerr = new IslandTerrain;
		else if (before_dot == "Nevada")
			pTerr = new NevadaTerrain;
		else if (before_dot == "TransitTerrain")
			pTerr = new TransitTerrain;
		else if (before_dot == "Romania")
			pTerr = new Romania;
		else
			pTerr = new vtTerrain;

		if (pTerr->SetParamFile(directory + "/" + name))
		{
			//vtString sn = pTerr->GetParams().GetValueString(STR_NAME);
			//VTLOG("Terrain name: '%s'\n", (const char *) sn);
			AppendTerrain(pTerr);
		}
		else
			VTLOG1("\t Couldn't read.\n");
		count++;
	}
	VTLOG("\t%d terrains.\n", count);
}

void Enviro::LoadGlobalContent()
{
	// Load the global content file, if there is one
	VTLOG("Looking for global content file '%s'\n", (const char *)g_Options.m_strContentFile);
	vtString fname = FindFileOnPaths(vtGetDataPath(), g_Options.m_strContentFile);
	if (fname != "")
	{
		bool success = true;
		vtContentManager3d &con = vtGetContent();
		VTLOG1("  Loading content file.\n");
		try {
			con.ReadXML(fname);
		}
		catch (xh_io_exception &e)
		{
			success = false;
			string str = e.getFormattedMessage();
			VTLOG("  Error: %s\n", str.c_str());
		}
		if (success)
			VTLOG("   Load successful, %d items\n", con.NumItems());
		else
			VTLOG1("   Load not successful.\n");
	}
	else
		VTLOG1("  Couldn't find it.\n");
}

void Enviro::StartControlEngine()
{
	VTLOG1("StartControlEngine\n");

	m_pControlEng = new ControlEngine();
	m_pControlEng->SetName2("Control Engine");
	vtGetScene()->AddEngine(m_pControlEng);
}

void Enviro::DoControl()
{
	if (m_fMessageTime != 0.0f)
	{
		if ((vtGetTime() - m_fMessageStart) > m_fMessageTime)
		{
			SetMessage("");
			m_fMessageTime = 0.0f;
		}
	}
	if (m_state == AS_Initializing)
	{
		m_iInitStep++;

		VTLOG("AS_Initializing initstep=%d\n", m_iInitStep);

		if (m_iInitStep == 1)
		{
			SetupScene1();
			return;
		}
		if (m_iInitStep == 2)
		{
			SetupScene2();
			return;
		}
		if (m_iInitStep == 3)
		{
			SetupScene3();
			return;
		}
		if (g_Options.m_bStartInNeutral)
		{
			SetState(AS_Neutral);
		}
		else if (g_Options.m_bEarthView)
		{
			FlyToSpace();
			return;
		}
		else
		{
			if (!SwitchToTerrain(g_Options.m_strInitTerrain))
			{
				SetMessage(_("Terrain not found"));
				SetState(AS_Error);
			}
			return;
		}
	}
	if (m_state == AS_FlyingIn)
	{
		if (m_iFlightStage == 1)
		{
			FlyInStage1();
			return;
		}
		if (m_iFlightStage == 2)
		{
			FlyInStage2();
			return;
		}
	}
	if (m_state == AS_SwitchToTerrain)
	{
		m_iInitStep++;
		SetupTerrain(m_pTargetTerrain);
		if (m_bFlyIn && m_iInitStep >= 7)
		{
			// Finished constructing, can smoothly fly in now
			ShowProgress(false);
			StartFlyIn();
		}
	}
	if (m_state == AS_MovingOut)
	{
		m_iInitStep++;
		SetupGlobe();
	}
	if (m_state == AS_Orbit)
		DoControlOrbit();
	if (m_state == AS_Terrain)
		DoControlTerrain();
}

void Enviro::DoControlTerrain()
{
	vtTerrain *terr = GetCurrentTerrain();
	if (!terr)
		return;

	// Update structure paging and shadows
	vtLodGrid *plg = terr->GetStructureGrid();
	vtPagedStructureLodGrid *pslg = terr->GetStructureLodGrid();
	if (pslg)
	{
		int remaining = terr->DoStructurePaging();
		if (m_pHUDMessage)
		{
			if (remaining != 0)
			{
				vtString msg;
				msg.Format("Structure queue: %d\n", remaining);
				m_pHUDMessage->SetText(msg);
			}
			else
				m_pHUDMessage->SetText("");
		}
	}
	if (plg)
	{
		// Periodically re-render the shadows
		static FPoint3 previous_shadow_center(0,0,0);

		// Only draw shadows in the area of visible structure LOD
		FSphere sph;
		vtCamera *cam = vtGetScene()->GetCamera();
		sph.center = cam->GetTrans();
		sph.radius = terr->GetLODDistance(TFT_STRUCTURES);

		bool bRedrawShadows = false;

		// If we moved 15% of the LOD distance, re-render
		if ((sph.center - previous_shadow_center).Length() > (sph.radius * 0.15f))
			bRedrawShadows = true;

		// If we've paged in 20 buildings since last render, re-render
		if (pslg && pslg->GetLoadCount() > 20)
			bRedrawShadows = true;

		if (bRedrawShadows)
		{
			// Pass true to force re-render with current area
			vtGetScene()->SetShadowSphere(sph, true);

			previous_shadow_center = sph.center;
			if (pslg)
				pslg->ResetLoadCount();
		}
	}
}

bool Enviro::SwitchToTerrain(const char *name)
{
	VTLOG("SwitchToTerrain (%s)\n", name);
	vtTerrain *pTerr = FindTerrainByName(name);
	if (!pTerr)
		return false;

	if (!IsAcceptable(pTerr))
		return false;

	if (pTerr)
	{
		SwitchToTerrain(pTerr);
		return true;
	}
	else
		return false;
}

void Enviro::SwitchToTerrain(vtTerrain *pTerr)
{
	VTLOG("SwitchToTerrain %lx\n", pTerr);

	// Load the species file and check which appearances are available
	LoadSpeciesList();
	FreeArc();

	if (m_state == AS_Orbit)
	{
		// remember camera position
		m_pTrackball->GetState(m_SpaceTrackballState);
	}
	if (m_state == AS_Terrain)
	{
		// remember camera position
		vtTerrain *pT = GetCurrentTerrain();
		vtCamera *pCam = vtGetScene()->GetCamera();
		FMatrix4 mat;
		pCam->GetTransform1(mat);
		pT->SetCamLocation(mat);
		pT->SaveRoute();
	}

	// If it's not a tileset, and we're coming in from space, fly in
	if (m_state == AS_Orbit && g_Options.m_bFlyIn)
		m_bFlyIn = true;

	SetState(AS_SwitchToTerrain);
	m_pTargetTerrain = pTerr;
	m_iInitStep = 0;

	if (g_Options.m_bShowProgress)
	{
		ShowProgress(true);
		SetProgressTerrain(pTerr);
	}

	// Layer view needs to update
	RefreshLayerView();
}

void Enviro::SetupTerrain(vtTerrain *pTerr)
{
	// Avoid trouble with '.' and ',' in Europe
	LocaleWrap normal_numbers(LC_NUMERIC, "C");

	VTLOG("SetupTerrain step %d\n", m_iInitStep);
	if (m_iInitStep == 1)
	{
		vtString str = _("Creating Terrain ");
		str += "'";
		str += pTerr->GetName();
		str += "'";
		SetMessage(str);
		UpdateProgress(m_strMessage, 10, 0);
	}
	else if (m_iInitStep == 2)
	{
		if (pTerr->IsCreated())
		{
			m_iInitStep = 7;	// already made, skip ahead
			return;
		}
		else
			SetMessage(_("Loading Elevation"));
		UpdateProgress(m_strMessage, 20, 0);
	}
	if (m_iInitStep == 3)
	{
		OnCreateTerrain(pTerr);

		pTerr->SetPlantList(m_pPlantList);
		pTerr->CreateStep0();
		if (!pTerr->CreateStep1())
		{
			SetState(AS_Error);
			SetMessage(pTerr->GetLastError());
			return;
		}
		SetMessage(_("Loading/Chopping/Prelighting Textures"));
		UpdateProgress(m_strMessage, 30, 0);
	}
	else if (m_iInitStep == 4)
	{
		if (m_pSkyDome)
		{
			// Tell the skydome where on the planet we are
			DPoint2 geo = pTerr->GetCenterGeoLocation();
			m_pSkyDome->SetGeoLocation(geo);

			// Set time to that of the new terrain
			m_pSkyDome->SetTime(pTerr->GetInitialTime());
		}

		if (!pTerr->CreateStep2(GetSunLight()))
		{
			SetState(AS_Error);
			SetMessage(pTerr->GetLastError());
			return;
		}
		SetMessage(_("Processing Elevation"));
		UpdateProgress(m_strMessage, 40, 0);
	}
	else if (m_iInitStep == 5)
	{
		if (!pTerr->CreateStep3())
		{
			SetState(AS_Error);
			SetMessage(pTerr->GetLastError());
			return;
		}
		SetMessage(_("Building CLOD"));
		UpdateProgress(m_strMessage, 50, 0);
	}
	else if (m_iInitStep == 6)
	{
		if (!pTerr->CreateStep4())
		{
			SetState(AS_Error);
			SetMessage(pTerr->GetLastError());
			return;
		}
		SetMessage(_("Creating Culture"));
		UpdateProgress(m_strMessage, 60, 0);
	}
	else if (m_iInitStep == 7)
	{
		if (!pTerr->CreateStep5())
		{
			SetState(AS_Error);
			SetMessage(pTerr->GetLastError());
			ShowProgress(false);
			return;
		}

		// Initial default location for camera for this terrain: Try center
		//  of heightfield, just above the ground
		vtHeightField3d *pHF = pTerr->GetHeightField();
		FPoint3 middle;
		FMatrix4 mat;

		VTLOG1(" Placing the camera at the center of the terrain:\n");
		VTLOG(" World extents: LRTB %f %f %f %f\n",
			pHF->m_WorldExtents.left,
			pHF->m_WorldExtents.right,
			pHF->m_WorldExtents.top,
			pHF->m_WorldExtents.bottom);
		pHF->GetCenter(middle);
		VTLOG(" Center: %f %f %f\n", middle.x, middle.y, middle.z);
		pHF->FindAltitudeAtPoint(middle, middle.y);
		VTLOG(" Altitude at that point: %f\n", middle.y);
		float minheight = pTerr->GetParams().GetValueFloat(STR_MINHEIGHT);
		middle.y += minheight;
		VTLOG(" plus minimum height (%f) is %f\n", minheight, middle.y);

		mat.Identity();
		mat.SetTrans(middle);
		pTerr->SetCamLocation(mat);

		SetMessage(_("Setting Camera"));
		UpdateProgress(m_strMessage, 70, 0);
	}
	else if (m_iInitStep == 8)
	{
		// If we were in Earth View, hide the globe and disable the trackball
		if (m_pGlobeContainer != NULL)
		{
			m_pGlobeContainer->SetEnabled(false);
			m_pGlobePicker->SetEnabled(false);
		}
		if (m_pTrackball)
			m_pTrackball->SetEnabled(false);

		// Set initial location
		m_pNormalCamera->SetTransform1(pTerr->GetCamLocation());

		SetMessage(_("Switching to Terrain"));
		UpdateProgress(m_strMessage, 80, 0);
	}
	else if (m_iInitStep == 9)
	{
		// make first terrain active
		SetTerrain(pTerr);

		// Set hither and yon
		m_pNormalCamera->SetHither(pTerr->GetParams().GetValueFloat(STR_HITHER));
		m_pNormalCamera->SetYon(500000.0f);

		// ensure that sunlight is active
		GetSunLight()->SetEnabled(true);

		m_pCurRoute=pTerr->GetLastRoute();	// Error checking needed here.

		m_pTerrainPicker->SetEnabled(true);
		SetMode(MM_NAVIGATE);
	}
	else if (m_iInitStep == 10)
	{
		vtString str = _("Welcome to ");
		str += pTerr->GetName();
		SetMessage(str, 5.0f);

		// Layer view needs to update
		RefreshLayerView();

		clock_t clock2 = clock();
		VTLOG(" seconds since app start: %.2f\n", (float)clock2/CLOCKS_PER_SEC);

		ShowProgress(false);

		SetState(AS_Terrain);
	}
}

void Enviro::FormatCoordString(vtString &str, const DPoint3 &coord, LinearUnits units, bool seconds)
{
	DPoint3 pos = coord;
	if (units == LU_DEGREES)
	{
		int deg1 = (int) pos.x;
		pos.x -= (deg1);
		int min1 = (int) (pos.x * 60);
		pos.x -= (min1 / 60.0);
		int sec1 = (int) (pos.x * 3600);
		if (deg1 < 0) deg1 = -deg1;
		if (min1 < 0) min1 = -min1;
		if (sec1 < 0) sec1 = -sec1;
		char ew = coord.x > 0.0f ? 'E' : 'W';

		int deg2 = (int) pos.y;
		pos.y -= (deg2);
		int min2 = (int) (pos.y * 60);
		pos.y -= (min2 / 60.0);
		int sec2 = (int) (pos.y * 3600);
		if (deg2 < 0) deg2 = -deg2;
		if (min2 < 0) min2 = -min2;
		if (sec2 < 0) sec2 = -sec2;
		char ns = coord.y > 0.0f ? 'N' : 'S';

		if (seconds)
			str.Format("%3d:%02d:%02d %c, %3d:%02d:%02d %c", deg1, min1, sec1, ew, deg2, min2, sec2, ns);
		else
			str.Format("%3d:%02d %c, %3d:%02d %c", deg1, min1, ew, deg2, min2, ns);
	}
	else
	{
		str.Format("%7.1d, %7.1d", (int) coord.x, (int) coord.y);
	}
}


//
// Check the terrain under the 3D cursor (for Terrain View).
//
void Enviro::DoCursorOnTerrain()
{
	m_bOnTerrain = false;
	DPoint3 earthpos;
	vtString str;

	if (m_pTerrainPicker != NULL)
		m_bOnTerrain = m_pTerrainPicker->GetCurrentEarthPos(earthpos);
	if (m_bOnTerrain)
	{
		m_EarthPos = earthpos;

		// Attempt to scale the 3d cursor, for ease of use.
		// Rather than keeping it the same size in world space (it would
		// be too small in the distance) or the same size in screen space
		// (would look confusing without the spatial distance cue) we
		// compromise and scale it based on the square root of distance.
		FPoint3 gpos;
		if (m_pTerrainPicker->GetCurrentPoint(gpos))
		{
			FPoint3 campos = vtGetScene()->GetCamera()->GetTrans();
			float distance = (gpos - campos).Length();
			float sc = (float) sqrt(distance) / 1.0f;
			FPoint3 pos = m_pCursorMGeom->GetTrans();
			m_pCursorMGeom->Identity();
			m_pCursorMGeom->Scale3(sc, sc, sc);
			m_pCursorMGeom->SetTrans(pos);
		}

		// Inform GUI, in case it cares.
		EarthPosUpdated();
	}
}


void Enviro::SetupScene1()
{
	VTLOG1("SetupScene1\n");

	// Set some global properties
	m_fCatenaryFactor = g_Options.m_fCatenaryFactor;
	vtMaterial::s_bTextureCompression = g_Options.m_bTextureCompression;
	vtNode::s_bDisableMipmaps = g_Options.m_bDisableModelMipmaps;

	vtScene *pScene = vtGetScene();
	vtCamera *pCamera = pScene->GetCamera();
	if (pCamera) pCamera->SetName2("Standard Camera");

	m_pRoot = BeginTerrainScene();
	pScene->SetRoot(m_pRoot);
}

void Enviro::SetupScene2()
{
	VTLOG1("SetupScene2\n");

	m_pNavEngines = new vtEngine;
	m_pNavEngines->SetName2("Navigation Engines");
	vtGetScene()->GetRootEngine()->AddChild(m_pNavEngines);

	// Make navigation engines
	m_pOrthoFlyer = new vtOrthoFlyer(1.0f);
	m_pOrthoFlyer->SetName2("Orthographic View Flyer");
	m_pOrthoFlyer->SetEnabled(false);
	m_pNavEngines->AddChild(m_pOrthoFlyer);

	m_pQuakeFlyer = new QuakeFlyer(1.0f);
	m_pQuakeFlyer->SetName2("Quake-Style Flyer");
	m_pQuakeFlyer->SetEnabled(false);
	m_pNavEngines->AddChild(m_pQuakeFlyer);

	m_pVFlyer = new VFlyer(1.0f);
	m_pVFlyer->SetName2("Velocity-Gravity Flyer");
	m_pVFlyer->SetEnabled(false);
	m_pNavEngines->AddChild(m_pVFlyer);

	m_pTFlyer = new vtTerrainFlyer(1.0f);
	m_pTFlyer->SetName2("Terrain-following Flyer");
	m_pTFlyer->SetEnabled(false);
	m_pNavEngines->AddChild(m_pTFlyer);

	m_pGFlyer = new GrabFlyer(1.0f);
	m_pGFlyer->SetName2("Grab-Pivot Flyer");
	m_pGFlyer->SetEnabled(false);
	m_pNavEngines->AddChild(m_pGFlyer);

	m_pFlatFlyer = new FlatFlyer();
	m_pFlatFlyer->SetName2("Flat Flyer");
	m_pFlatFlyer->SetEnabled(false);
	m_pNavEngines->AddChild(m_pFlatFlyer);

	m_pPanoFlyer = new vtPanoFlyer(1.0f);
	m_pPanoFlyer->SetName2("Panoramic Flyer");
	m_pPanoFlyer->SetEnabled(false);
	m_pNavEngines->AddChild(m_pPanoFlyer);

	m_nav = NT_Normal;

	// create picker object and picker engine
	vtGeom *pCursor = Create3DCursor(1.0, g_Options.m_fCursorThickness);
	m_pCursorMGeom = new vtMovGeom(pCursor);
	m_pCursorMGeom->SetName2("Cursor");

	GetTop()->AddChild(m_pCursorMGeom);
	m_pTerrainPicker = new TerrainPicker();
	m_pTerrainPicker->SetName2("TerrainPicker");
	vtGetScene()->AddEngine(m_pTerrainPicker);

	m_pTerrainPicker->SetTarget(m_pCursorMGeom);
	m_pTerrainPicker->SetEnabled(false); // turn off at startup

	// Connect to the GrabFlyer
	m_pGFlyer->SetTerrainPicker(m_pTerrainPicker);

	VTLOG("Setting up Cameras\n");
	m_pNormalCamera = vtGetScene()->GetCamera();

	// Create second camera (for Top-Down view)
	if (m_pTopDownCamera == NULL)
	{
		VTLOG("Creating Top-Down Camera\n");
		m_pTopDownCamera = new vtCamera;
		m_pTopDownCamera->SetOrtho(true);
		m_pTopDownCamera->SetName2("Top-Down Camera");
		m_pOrthoFlyer->SetTarget(m_pTopDownCamera);
	}

	m_pQuakeFlyer->SetTarget(m_pNormalCamera);
	m_pVFlyer->SetTarget(m_pNormalCamera);
	m_pTFlyer->SetTarget(m_pNormalCamera);
	m_pGFlyer->SetTarget(m_pNormalCamera);
	m_pFlatFlyer->SetTarget(m_pNormalCamera);
	m_pPanoFlyer->SetTarget(m_pNormalCamera);

	// An engine to keep the camera above the terrain, comes after the other
	//  engines which could move the camera.
	m_pHeightEngine = new vtHeightConstrain(1.0f);
	m_pHeightEngine->SetName2("Height Constrain Engine");
	m_pHeightEngine->SetTarget(m_pNormalCamera);
	vtGetScene()->GetRootEngine()->AddChild(m_pHeightEngine);

	// This HUD group will contain geometry such as the legend
	m_pHUD = new vtHUD;
	m_pHUD->SetName2("HUD");
	m_pRoot->AddChild(m_pHUD);
	m_pHUDMaterials = new vtMaterialArray;

	// The HUD always has a place to put status messages to the user
	m_pArial = new vtFont;
	if (!m_pArial->LoadFont("Arial.ttf"))
	{
		delete m_pArial;
		m_pArial = NULL;
	}

	vtGeom *geom = new vtGeom;
	geom->SetName2("Message");
	m_pHUD->AddChild(geom);
	if (m_pArial)
	{
		m_pHUDMessage = new vtTextMesh(m_pArial, 18);
		m_pHUDMessage->SetText("");
		m_pHUDMessage->SetPosition(FPoint3(3,3,0));
		geom->AddTextMesh(m_pHUDMessage, 0);
		m_pHUDMessage->Release();	// pass ownership
	}
}

//
// Load the species file and check which appearances are available
//
void Enviro::LoadSpeciesList()
{
	if (m_bPlantsLoaded)
		return;

	VTLOG1("LoadSpeciesList\n");

	// First look for species.xml with terrain name prepended, otherwise fall
	//  back on just "species.xml"
	vtString species_fname = "PlantData/" + g_Options.m_strInitTerrain + "-species.xml";
	vtString species_path = FindFileOnPaths(vtGetDataPath(), species_fname);
	if (species_path == "")
		species_path = FindFileOnPaths(vtGetDataPath(), "PlantData/species.xml");

	if (species_path == "")
	{
		VTLOG1(" not found.\n");
		return;
	}

	vtSpeciesList pl;
	vtString errmsg;
	if (pl.ReadXML(species_path, &errmsg))
	{
		VTLOG(" Using species file: '%s'\n", (const char *) species_path);
		m_pPlantList = new vtSpeciesList3d;
		*m_pPlantList = pl;

		// global options
		vtPlantAppearance3d::s_fPlantScale = g_Options.m_fPlantScale;
		vtPlantAppearance3d::s_bPlantShadows = g_Options.m_bShadows;

		// Don't load all the plant appearances now, just check which are available
//			m_pPlantList->CreatePlantSurfaces();
		int available = m_pPlantList->CheckAvailability();
		VTLOG(" %d plant appearances available.\n", available);

		m_bPlantsLoaded = true;
	}
	else
	{
		VTLOG1("Error loading species: ");
		VTLOG1(errmsg);
		VTLOG1("\n");
	}
}

void Enviro::SetCurrentNavigator(vtTerrainFlyer *pE)
{
	if (m_pCurrentFlyer != NULL)
	{
		const char *name = m_pCurrentFlyer->GetName2();
		VTLOG("Disabling '%s'\n", name);

		m_pCurrentFlyer->SetEnabled(false);
	}
	m_pCurrentFlyer = pE;
	if (m_pCurrentFlyer == NULL)
		VTLOG1("No navigator now enabled.\n");
	else
	{
		const char *name = m_pCurrentFlyer->GetName2();
		VTLOG(" Enabling '%s'\n", name);

		m_pCurrentFlyer->SetEnabled(true);
	}
}

void Enviro::EnableFlyerEngine(bool bEnable)
{
	if (bEnable && !m_bTopDown)
	{
		// enable normal navigation
		if (m_nav == NT_Normal)
			SetCurrentNavigator(m_pTFlyer);
		if (m_nav == NT_Velo)
			SetCurrentNavigator(m_pVFlyer);
		if (m_nav == NT_Grab)
			SetCurrentNavigator(m_pGFlyer);
//		if (m_nav == NT_Quake)
//			SetCurrentNavigator(m_pQuakeFlyer);
		if (m_nav == NT_Pano || m_nav == NT_Dummy)
			SetCurrentNavigator(m_pPanoFlyer);
	}
	else
		SetCurrentNavigator(NULL);

	if (m_bTopDown)
	{
		VTLOG("Enable OrthoFlyer: %d\n", bEnable);
		m_pOrthoFlyer->SetEnabled(bEnable);
	}
}

void Enviro::SetNavType(NavType nav)
{
	if (m_mode == MM_NAVIGATE)
		EnableFlyerEngine(false);
	m_nav = nav;
	if (m_mode == MM_NAVIGATE)
		EnableFlyerEngine(true);
}

void Enviro::SetMaintain(bool bOn)
{
	m_pHeightEngine->SetMaintain(bOn);
	m_pHeightEngine->SetMaintainHeight(0);
}

bool Enviro::GetMaintain()
{
	if (m_pHeightEngine == NULL)
		return false;
	return m_pHeightEngine->GetMaintain();
}


void Enviro::SetTerrain(vtTerrain *pTerrain)
{
	VTLOG("Enviro::SetTerrain '%s'\n",
		pTerrain ? (const char *) pTerrain->GetName() : "none");

	if (m_pCurrentTerrain)
	{
		vtGroup *pOverlay = m_pCurrentTerrain->GetOverlay();
		if (pOverlay)
			m_pHUD->RemoveChild(pOverlay);
	}

	// Inform the container that this new terrain is current
	SetCurrentTerrain(pTerrain);

	// safety check
	if (!pTerrain)
	{
		ShowMapOverview(false);
		return;
	}
	vtHeightField3d *pHF = pTerrain->GetHeightField();
	if (!pHF)
		return;

	// Inform the UI that this new terrain is current
	TParams &param = pTerrain->GetParams();
	SetNavType((enum NavType) param.GetValueInt(STR_NAVSTYLE));

	EnableFlyerEngine(true);

	// Inform the terrain's location saver of the camera
	pTerrain->GetLocSaver()->SetTransform(m_pNormalCamera);

	// inform the navigation engine of the new terrain
	float speed = param.GetValueFloat(STR_NAVSPEED);
	if (m_pCurrentFlyer != NULL)
	{
		m_pCurrentFlyer->SetTarget(m_pNormalCamera);
		m_pCurrentFlyer->SetEnabled(true);
		m_pCurrentFlyer->SetExag(param.GetValueBool(STR_ACCEL));
	}
	m_pTFlyer->SetSpeed(speed);
	m_pVFlyer->SetSpeed(speed);
	m_pPanoFlyer->SetSpeed(speed);
	m_pOrthoFlyer->SetSpeed(speed);

	// TODO: a more elegant way of keeping all nav engines current
	m_pQuakeFlyer->SetHeightField(pHF);
	m_pVFlyer->SetHeightField(pHF);
	m_pTFlyer->SetHeightField(pHF);
	m_pGFlyer->SetHeightField(pHF);
	m_pPanoFlyer->SetHeightField(pHF);
	// also the height constraint engine
	m_pHeightEngine->SetHeightField(pHF);
	m_pHeightEngine->SetMinGroundOffset(param.GetValueFloat(STR_MINHEIGHT));

	bool bAllowRoll = param.GetValueBool(STR_ALLOW_ROLL);
	m_pTFlyer->SetDOF(vtFlyer::DOF_ROLL, bAllowRoll);

	// Set the top-down viewpoint to a point over the center of the new
	//  terrain, with near and far planes derived from the height extents.
	m_pTopDownCamera->Identity();
	FPoint3 middle;
	pHF->GetCenter(middle);		// Gets XZ center

	float fMin, fMax;
	pHF->GetHeightExtents(fMin, fMax);
	fMax *= param.GetValueFloat(STR_VERTICALEXAG);
	fMax += 1;	// beware flat terrain: safety buffer of 1 meter
	middle.y = fMax + ORTHO_HITHER;		// highest value + hither
	m_pTopDownCamera->SetTrans(middle);

	// point it straight down
	m_pTopDownCamera->RotateLocal(TRANS_XAxis, -PID2f);
	m_pTopDownCamera->SetHither(ORTHO_HITHER);
	m_pTopDownCamera->SetYon(fMax - fMin + ORTHO_HITHER + ORTHO_HITHER);

	// pick an arbitrary amount of detail to show initially:
	//  1/4 the terrain width, with proportional speed
	m_pTopDownCamera->SetWidth(middle.x / 2);
	m_pOrthoFlyer->SetSpeed(middle.x / 5);

	if (m_pTerrainPicker != NULL)
		m_pTerrainPicker->SetHeightField(pHF);

	m_fDistToolHeight = param.GetValueFloat(STR_DIST_TOOL_HEIGHT);

	vtGroup *pOverlay = pTerrain->GetOverlay();
	if (pOverlay)
		m_pHUD->AddChild(pOverlay);

	if (m_iFlightStage != 2)
	{
		// Only do this the first time: jump to initial viewpoint
		if (!pTerrain->IsVisited())
		{
			VTLOG1("First visit to this terrain, looking up stored viewpoint.\n");
			if (g_Options.m_strInitLocation != "")
			{
				// may have been given on command line
				pTerrain->GetLocSaver()->RecallFrom(g_Options.m_strInitLocation);
				g_Options.m_strInitLocation = "";
			}
			else
				pTerrain->GetLocSaver()->RecallFrom(pTerrain->GetParams().GetValueString(STR_INITLOCATION));
		}
	}
	pTerrain->Visited(true);

	// Inform the GUI that the terrain has changed
	SetTerrainToGUI(pTerrain);

	// Inform the map overview
	if (m_pMapOverview)
		m_pMapOverview->SetTerrain(pTerrain);

	// If there is an initial scenario, show it
	vtString sname;
	if (param.GetValueString(STR_INIT_SCENARIO, sname))
	{
		for (unsigned int snum = 0; snum < param.m_Scenarios.size(); snum++)
		{
			if (sname == param.m_Scenarios[snum].GetValueString(STR_SCENARIO_NAME))
				pTerrain->ActivateScenario(snum);
		}
	}
}

//
// Copy as much state as possible from the active terrain to its parameters
//
void Enviro::StoreTerrainParameters()
{
	vtTerrainScene *ts = vtGetTS();
	vtTimeEngine *te = ts->GetTimeEngine();
	vtTerrain *terr = GetCurrentTerrain();
	TParams &par = terr->GetParams();
	vtCamera *cam = vtGetScene()->GetCamera();
	vtSkyDome *sky = ts->GetSkyDome();

	par.SetValueFloat(STR_VERTICALEXAG, terr->GetVerticalExag());

	par.SetValueInt(STR_NAVSTYLE, GetNavType());
	par.SetValueFloat(STR_NAVSPEED, GetFlightSpeed());
	//par.SetValueString(STR_LOCFILE);
	//par.SetValueString(STR_INITLOCATION);
	par.SetValueFloat(STR_HITHER, cam->GetHither());

	if (par.GetValueInt(STR_SURFACE_TYPE) == 0)	// single grid
	{
		vtDynTerrainGeom *dtg = terr->GetDynTerrain();
		par.SetValueInt(STR_TRICOUNT, dtg->GetPolygonTarget());
	}
	else if (par.GetValueInt(STR_SURFACE_TYPE) == 2)	// tileset
	{
		vtTiledGeom *tg = terr->GetTiledGeom();
		par.SetValueInt(STR_VERTCOUNT, tg->GetVertexTarget());
	}

	par.SetValueBool(STR_TIMEON, te->GetSpeed() != 0.0f);
	par.SetValueString(STR_INITTIME, te->GetTime().GetAsString());
	par.SetValueFloat(STR_TIMESPEED, te->GetSpeed());

	par.SetValueBool(STR_FOG, terr->GetFog());
	//par.SetValueFloat(STR_FOGDISTANCE);	// already set dynamically
	par.SetValueBool(STR_SKY, sky->GetEnabled());
	// par.SetValueString(STR_SKYTEXTURE);

	par.SetValueBool(STR_OCEANPLANE, terr->GetFeatureVisible(TFT_OCEAN));
	//par.SetValueFloat(STR_OCEANPLANELEVEL);	// already set dynamically
	//par.SetValueBool(STR_DEPRESSOCEAN);
	//par.SetValueFloat(STR_DEPRESSOCEANLEVEL);
	par.SetValueBool(STR_HORIZON,  terr->GetFeatureVisible(TFT_HORIZON));
	RGBi col = terr->GetBgColor();
	par.SetValueRGBi(STR_BGCOLOR, col);

	par.SetValueFloat(STR_STRUCTDIST, terr->GetLODDistance(TFT_STRUCTURES));
	par.SetValueFloat(STR_ROADDISTANCE, terr->GetLODDistance(TFT_ROADS));
	par.SetValueInt(STR_VEGDISTANCE, terr->GetLODDistance(TFT_VEGETATION));

	par.SetValueBool(STR_OVERVIEW, GetShowMapOverview());
	par.SetValueBool(STR_COMPASS, GetShowCompass());

	// Layers: copy back from the current set of layers to the set of
	//  layers in the parameters.
	LayerSet &set = terr->GetLayers();
	par.m_Layers.clear();
	for (unsigned int i = 0; i < set.GetSize(); i++)
	{
		vtLayer *lay = set[i];

		vtStructureLayer *slay = dynamic_cast<vtStructureLayer*>(lay);
		vtImageLayer *ilay = dynamic_cast<vtImageLayer*>(lay);
		vtAbstractLayer *alay = dynamic_cast<vtAbstractLayer*>(lay);

		vtTagArray newlay;
		if (slay)
			newlay.SetValueString("Type", TERR_LTYPE_STRUCTURE, true);
		if (ilay)
			newlay.SetValueString("Type", TERR_LTYPE_IMAGE, true);
		if (alay)
		{
			newlay.SetValueString("Type", TERR_LTYPE_ABSTRACT, true);
			vtTagArray &style = alay->GetProperties();
			newlay.CopyTagsFrom(style);
		}

		newlay.SetValueString("Filename", lay->GetLayerName(), true);
		newlay.SetValueBool("Visible", lay->GetVisible());
		par.m_Layers.push_back(newlay);
	}
}

//
// Display a message as a text sprite in the middle of the window.
//
// The fTime argument lets you specify how long the message should
// appear, in seconds.
//
void Enviro::SetMessage(const vtString &msg, float fTime)
{
	VTLOG("  SetMessage: '%s'\n", (const char *) msg);

#if 0
	if (m_pMessageSprite == NULL)
	{
		m_pMessageSprite = new vtSprite;
		m_pMessageSprite->SetName2("MessageSprite");
		m_pRoot->AddChild(m_pMessageSprite);
	}
	if (msg == "")
		m_pMessageSprite->SetEnabled(false);
	else
	{
		m_pMessageSprite->SetEnabled(true);
		m_pMessageSprite->SetText(msg);
		int len = msg.GetLength();
		m_pMessageSprite->SetWindowRect(0.5f - (len * 0.01f), 0.45f,
										0.5f + (len * 0.01f), 0.55f);
	}
#endif
	if (msg != "" && fTime != 0.0f)
	{
		m_fMessageStart = vtGetTime();
		m_fMessageTime = fTime;
	}
	m_strMessage = msg;
}

void Enviro::SetFlightSpeed(float speed)
{
	if (m_bTopDown && m_pOrthoFlyer != NULL)
		m_pOrthoFlyer->SetSpeed(speed);
	else if (m_pCurrentFlyer != NULL)
		m_pCurrentFlyer->SetSpeed(speed);
}

float Enviro::GetFlightSpeed()
{
	if (m_bTopDown && m_pOrthoFlyer != NULL)
		return m_pOrthoFlyer->GetSpeed();
	else if (m_pCurrentFlyer != NULL)
		return m_pCurrentFlyer->GetSpeed();
	else
		return 0.0f;
}

void Enviro::SetFlightAccel(bool bAccel)
{
	if (m_pCurrentFlyer)
		m_pCurrentFlyer->SetExag(bAccel);
}

bool Enviro::GetFlightAccel()
{
	if (m_pCurrentFlyer)
		return m_pCurrentFlyer->GetExag();
	else
		return false;
}

void Enviro::SetMode(MouseMode mode)
{
	VTLOG("SetMode %d\n", mode);

	if (m_pCursorMGeom)
	{
		switch (mode)
		{
		case MM_NAVIGATE:
			m_pCursorMGeom->SetEnabled(false);
			EnableFlyerEngine(true);
			break;
		case MM_SELECT:
			m_pCursorMGeom->SetEnabled(!g_Options.m_bDirectPicking);
			EnableFlyerEngine(false);
			break;
		case MM_SELECTBOX:
			m_pCursorMGeom->SetEnabled(false);
			EnableFlyerEngine(false);
			break;
		case MM_FENCES:
		case MM_BUILDINGS:
		case MM_ROUTES:
		case MM_PLANTS:
		case MM_ADDPOINTS:
		case MM_INSTANCES:
		case MM_VEHICLES:
		case MM_MOVE:
		case MM_MEASURE:
			m_pCursorMGeom->SetEnabled(true);
			EnableFlyerEngine(false);
			break;
		}
	}
	m_bActiveFence = false;
	m_mode = mode;
}

void Enviro::SetTopDown(bool bTopDown)
{
	static bool bWas;

	m_bTopDown = bTopDown;

	if (bTopDown)
	{
		vtGetScene()->SetCamera(m_pTopDownCamera);
		bWas = m_pSkyDome->GetEnabled();
		m_pSkyDome->SetEnabled(false);
	}
	else
	{
		vtGetScene()->SetCamera(m_pNormalCamera);
		m_pSkyDome->SetEnabled(bWas);
		m_pOrthoFlyer->SetEnabled(false);
	}

	// set mode again, to put everything in the right state
	SetMode(m_mode);

	// inform the UI that we have a switched cameras
	CameraChanged();
}

void Enviro::DumpCameraInfo()
{
	vtCamera *cam = m_pNormalCamera;
	FPoint3 pos = cam->GetTrans();
	FPoint3 dir = cam->GetDirection();
	VTLOG("Camera: pos %f %f %f, dir %f %f %f\n",
		pos.x, pos.y, pos.z, dir.x, dir.y, dir.z);
}

void Enviro::SetSpeed(float x)
{
	if (m_state == AS_Orbit && m_pGlobeTime)
		m_pGlobeTime->SetSpeed(x);
	else if (m_state == AS_Terrain)
		vtGetTS()->GetTimeEngine()->SetSpeed(x);
}

float Enviro::GetSpeed()
{
	if (m_state == AS_Orbit && m_pGlobeTime)
		return m_pGlobeTime->GetSpeed();
	else if (m_state == AS_Terrain)
		return vtGetTS()->GetTimeEngine()->GetSpeed();
	return 0;
}

bool Enviro::OnMouse(vtMouseEvent &event)
{
	// check for what is under the 3D cursor
	if (m_state == AS_Orbit)
		DoCursorOnEarth();
	else if (m_state == AS_Terrain &&
		(m_pCursorMGeom->GetEnabled() || g_Options.m_bDirectPicking))
		DoCursorOnTerrain();

	// give the child classes first chance to take this event
	bool bCancel = OnMouseEvent(event);
	if (bCancel)
		return true;

	if (!OnMouseCompass(event))
		return false;

	if (event.type == VT_DOWN)
	{
		if (event.button == VT_LEFT)
		{
			if (m_state == AS_Terrain)
				OnMouseLeftDownTerrain(event);
			else if (m_state == AS_Orbit)
				OnMouseLeftDownOrbit(event);
		}
		else if (event.button == VT_RIGHT)
			OnMouseRightDown(event);
	}
	if (event.type == VT_MOVE)
	{
		OnMouseMove(event);
	}
	if (event.type == VT_UP)
	{
		if (event.button == VT_LEFT)
			OnMouseLeftUp(event);
		if (event.button == VT_RIGHT)
			OnMouseRightUp(event);
	}
	m_MouseLast = event.pos;
	return true;
}

void Enviro::OnMouseLeftDownTerrain(vtMouseEvent &event)
{
	if (m_mode != MM_SELECT && m_mode != MM_SELECTBOX && !m_bOnTerrain)
		return;

	vtTerrain *pTerr = GetCurrentTerrain();

	// Build fences on click
	if (m_mode == MM_FENCES)
	{
		if (!m_bActiveFence)
		{
			start_new_fence();
			m_bActiveFence = true;
		}
		pTerr->AddFencepoint(m_pCurFence, DPoint2(m_EarthPos.x, m_EarthPos.y));
	}
	if (m_mode == MM_BUILDINGS)
		OnMouseLeftDownBuildings();

	if (m_mode == MM_ROUTES)
	{
		if (!m_bActiveRoute)
		{
			start_new_route();
			m_bActiveRoute = true;
		}
		pTerr->add_routepoint_earth(m_pCurRoute,
			DPoint2(m_EarthPos.x, m_EarthPos.y), m_sStructType);
	}
	if (m_mode == MM_PLANTS)
	{
		// try planting a tree there
		if (pTerr->GetProjection().IsGeographic())
			VTLOG("Create a plant at %.8lf,%.8lf:", m_EarthPos.x, m_EarthPos.y);
		else
			VTLOG("Create a plant at %.2lf,%.2lf:", m_EarthPos.x, m_EarthPos.y);
		bool success = PlantATree(DPoint2(m_EarthPos.x, m_EarthPos.y));
		VTLOG(" %s.\n", success ? "yes" : "no");
	}
	if (m_mode == MM_ADDPOINTS)
	{
		DPoint2 atpoint(m_EarthPos.x, m_EarthPos.y);
		vtString str = GetStringFromUser("Created labeled point feature", "Label:");
		if (str != "")
		{
			vtAbstractLayer *alay = GetLabelLayer();
			vtFeatureSetPoint2D *pset = dynamic_cast<vtFeatureSetPoint2D*>(alay->GetFeatureSet());
			if (pset)
			{
				// add a single 2D point
				int rec = pset->AddPoint(atpoint);
				int field = pset->GetFieldIndex("Label");
				pset->SetValueFromString(rec, field, str);

				// create its 3D visual
				alay->CreateStyledFeature(rec);
			}
			UpdateLayerView();
		}
	}
	if (m_mode == MM_INSTANCES)
		PlantInstance();

	if (m_mode == MM_VEHICLES)
		CreateGroundVehicle(m_VehicleOpt);

	if (m_mode == MM_SELECT)
		OnMouseLeftDownTerrainSelect(event);

	if (m_mode == MM_SELECTBOX)
	{
		m_bDragging = true;
		m_MouseDown = event.pos;
	}

	if (m_mode == MM_MOVE)
		OnMouseLeftDownTerrainMove(event);

	if (m_mode == MM_MEASURE)
	{
		m_bDragging = true;
		DPoint2 g1(m_EarthPos.x, m_EarthPos.y);

		if (m_bMeasurePath)
		{
			// Path mode - set initial segment
			int len = m_distance_path.GetSize();
			if (len == 0)
			{
				// begin new path
				m_fArcLength = 0.0;
				m_EarthPosDown = m_EarthPos;
				m_distance_path.Append(g1);
			}
			// default: add point to the path
			m_distance_path.Append(g1);

			SetTerrainMeasure(m_distance_path);
		}
		else
		{
			m_EarthPosDown = m_EarthPos;
			m_fArcLength = 0.0;
			SetTerrainMeasure(g1, g1);
		}
		UpdateDistanceTool();
	}
}

void Enviro::OnMouseLeftDownBuildings()
{
	PolygonSelectionAddPoint();
}

void Enviro::OnMouseSelectRayPick(vtMouseEvent &event)
{
	VTLOG("Click, raypick at %d %d, ", event.pos.x, event.pos.y);

	vtTerrain *pTerr = GetCurrentTerrain();
	vtStructureArray3d *pActiveStructures = pTerr->GetStructureLayer();

	if (!(event.flags & VT_CONTROL) && (pActiveStructures != NULL))
	{
		pActiveStructures->VisualDeselectAll();
		m_bSelectedStruct = false;
	}

	vtPlantInstanceArray3d &Plants = pTerr->GetPlantInstances();
	Plants.VisualDeselectAll();
	m_bSelectedPlant = false;

	vtRouteMap &Routes = pTerr->GetRouteMap();
	m_bSelectedUtil = false;

	// Get ray intersection with near and far planes
	FPoint3 Near, Dir;
	vtGetScene()->CameraRay(event.pos, Near, Dir);

	// Dir is unit-length direction vector, so scale it up to the
	//  distance we want to test.
	Dir *= 10000.0f;	// 10km should be enough for visible objects

	vtHitList HitList;
	int iNumHits = vtIntersect(pTerr->GetTopGroup(), Near, Near+Dir, HitList);
	if (iNumHits == 0)
	{
		VTLOG("no hits\n");
		return;
	}
	else
		VTLOG("%d hits\n", iNumHits);

	// Check for structures
	int iOffset;
	vtStructureLayer *slay;
	slay = pTerr->GetLayers().FindStructureFromNode(HitList.front().node, iOffset);
	if (slay)
	{
		VTLOG("  Found structure ");
		vtBuilding3d *pBuilding = slay->GetBuilding(iOffset);
		vtStructInstance3d *pInstance = slay->GetInstance(iOffset);
		vtFence3d *pFence = slay->GetFence(iOffset);

		if (NULL != pBuilding)
		{
			// Found a procedural building
			VTLOG("(building)\n");
			pBuilding->ToggleSelect();
			if (pBuilding->IsSelected())
			{
				pBuilding->ShowBounds(true);
				m_bDragging = true;
			}
			else
				pBuilding->ShowBounds(false);
		}
		else if (NULL != pInstance)
		{
			// Found a structure instance
			VTLOG("(instance)\n");
			pInstance->ToggleSelect();
			if (pInstance->IsSelected())
			{
				pInstance->ShowBounds(true);
				if ((event.flags & VT_SHIFT) != 0)
				{
					m_StartRotation = pInstance->GetRotation();
					m_bRotating = true;
				}
				else
					m_bDragging = true;
			}
			else
				pInstance->ShowBounds(false);
		}
		else if (NULL != pFence)
		{
			// Found a linear structure
			VTLOG("(fence)\n");
			pFence->ToggleSelect();
			if (pFence->IsSelected())
				pFence->ShowBounds(true);
			else
				pFence->ShowBounds(false);
		}
		else
			VTLOG("(unknown)\n");
		if (pActiveStructures != slay)
		{
			// Switching to a different structure set
			pActiveStructures->VisualDeselectAll();
			pTerr->SetStructureLayer(slay);
			ShowLayerView();
			UpdateLayerView();
		}
		// This is inefficient it would be better to maintain a live count if possible
		if (pTerr->GetStructureLayer()->NumSelected())
			m_bSelectedStruct = true;
		else
			m_bSelectedStruct = false;
	}
	// Check for plants
	else if (Plants.FindPlantFromNode(HitList.front().node, iOffset))
	{
		VTLOG("  Found plant\n");
		Plants.VisualSelect(iOffset);
		m_bDragging = true;
		m_bSelectedPlant = true;
	}
	// Check for routes
	else if (Routes.FindRouteFromNode(HitList.front().node, iOffset))
	{
		VTLOG("  Found route\n");
		m_bDragging = true;
		m_bSelectedUtil = true;
		m_pSelRoute = Routes.GetAt(iOffset);
	}
	else
		VTLOG("  Unable to identify node\n");

	if (m_bDragging)
		VTLOG("Now dragging.\n");
	if (m_bRotating)
		VTLOG("Now rotating.\n");
}

void Enviro::OnMouseSelectCursorPick(vtMouseEvent &event)
{
	VTLOG("Click, cursor pick, ");

	// See if camera ray intersects a structure?  NO, it's simpler and
	//  easier for the user to just test whether the ground cursor is
	//  near a structure's origin.
	DPoint2 gpos(m_EarthPos.x, m_EarthPos.y);

	double dist1, dist2, dist3;
	vtTerrain *pTerr = GetCurrentTerrain();
	vtStructureArray3d *structures = pTerr->GetStructureLayer();
	if (!(event.flags & VT_CONTROL) && structures != NULL)
		structures->VisualDeselectAll();

	// SelectionCutoff is in meters, but the picking functions work in
	//  Earth coordinates.  Try to convert it to earth horiz units.
	DPoint2 eoffset;
	g_Conv.ConvertVectorToEarth(g_Options.m_fSelectionCutoff, 0, eoffset);
	double epsilon = eoffset.x;
	VTLOG("epsilon %lf, ", epsilon);

	// We also want to use a small (2m) buffer around linear features, so they
	//  can be picked even if they are inside/on top of a building.
	g_Conv.ConvertVectorToEarth(2.0f, 0, eoffset);
	double linear_buffer = eoffset.x;

	// Check Structures
	int structure;		// index of closest structure
	bool result1 = pTerr->FindClosestStructure(gpos, epsilon, structure, dist1,
		g_Options.m_fMaxPickableInstanceRadius, linear_buffer);
	if (result1)
		VTLOG("structure at dist %lf, ", dist1);
	m_bSelectedStruct = false;

	// Check Plants
	vtPlantInstanceArray3d &plants = pTerr->GetPlantInstances();
	plants.VisualDeselectAll();
	m_bSelectedPlant = false;

	// find index of closest plant
	int plant = plants.FindClosestPoint(gpos, epsilon);
	bool result2 = (plant != -1);
	if (result2)
	{
		dist2 = (gpos - plants.GetPoint(plant)).Length();
		VTLOG("plant at dist %lf, ", dist2);
	}
	else
		dist2 = 1E9;

	// Check Routes
	vtRouteMap &routes = pTerr->GetRouteMap();
	m_bSelectedUtil = false;
	bool result3 = routes.FindClosestUtilNode(gpos, epsilon, m_pSelRoute,
		m_pSelUtilNode, dist3);

	// Check Vehicles
	m_bSelectedVehicle = false;
	float dist4;
	FPoint3 wpos;
	g_Conv.ConvertFromEarth(m_EarthPos, wpos);
	m_Vehicles.VisualDeselectAll();
	int vehicle = m_Vehicles.FindClosestVehicle(wpos, dist4);
	if (dist4 > g_Options.m_fSelectionCutoff)
		vehicle = -1;

	bool click_struct = (result1 && dist1 < dist2 && dist1 < dist3 && dist1 < dist4);
	bool click_plant = (result2 && dist2 < dist1 && dist2 < dist3 && dist2 < dist4);
	bool click_route = (result3 && dist3 < dist1 && dist3 < dist2 && dist3 < dist4);
	bool click_vehicle = (vehicle!=-1 && dist4 < dist1 && dist4 < dist2 && dist4 < dist3);

	if (click_struct)
	{
		VTLOG(" struct is closest.\n");
		vtStructureArray3d *structures_picked = pTerr->GetStructureLayer();
		vtStructure *str = structures_picked->GetAt(structure);
		vtStructure3d *str3d = structures_picked->GetStructure3d(structure);
		if (str->GetType() != ST_INSTANCE && str3d->GetGeom() == NULL)
		{
			VTLOG("  Warning: unconstructed structure.\n");
		}
		else
		{
			str->Select(true);
			str3d->ShowBounds(true);

			vtStructInstance *inst = str->GetInstance();
			vtFence *fen = str->GetFence();

			// Reset dragging state
			m_bRotating = false;
			m_pDraggingFence = NULL;
			m_iDraggingFencePoint = -1;		// no grab

			if (inst != NULL && (event.flags & VT_SHIFT) != 0)
			{
				m_StartRotation = inst->GetRotation();
				m_bRotating = true;
			}
			else if (fen != NULL)
			{
				// perhaps we have clicked on a fence control point
				double dist;
				int idx = fen->GetNearestPointIndex(gpos, dist);
				if (idx != -1 && dist < 2.0f)	// distance cutoff
				{
					m_pDraggingFence = dynamic_cast<vtFence3d*>(str3d);
					m_iDraggingFencePoint = idx;	// grab
					m_bDragging = true;
				}
			}
			else
			{
				m_bDragging = true;
				// Press 'alt' key to drag vertically instead of horizontally
				if ((event.flags & VT_ALT) != 0)
					m_bDragUpDown = true;
			}
			m_bSelectedStruct = true;
		}
		if (structures_picked != structures)
		{
			// active structure set (layer) has changed due to picking
			structures->VisualDeselectAll();
			ShowLayerView();
			UpdateLayerView();
		}
	}
	else if (click_plant)
	{
		VTLOG(" plant is closest.\n");
		plants.VisualSelect(plant);
		m_bDragging = true;
		m_bSelectedPlant = true;
	}
	else if (click_route)
	{
		m_bDragging = true;
		m_bSelectedUtil = true;
	}
	else if (click_vehicle)
	{
		VTLOG(" vehicle is closest.\n");
		m_Vehicles.VisualSelect(vehicle);
		if ((event.flags & VT_SHIFT) != 0)
		{
			m_StartRotation = m_Vehicles.GetSelectedCarEngine()->GetRotation();
			m_bRotating = true;
		}
		else
			m_bDragging = true;
		m_bSelectedVehicle = true;
	}
	else
		VTLOG(" nothing.\n");
}

void Enviro::OnMouseLeftDownTerrainSelect(vtMouseEvent &event)
{
	if (g_Options.m_bDirectPicking)
		OnMouseSelectRayPick(event);
	else
		OnMouseSelectCursorPick(event);

	m_EarthPosDown = m_EarthPosLast = m_EarthPos;
	m_MouseDown = event.pos;
}

void Enviro::OnMouseLeftDownTerrainMove(vtMouseEvent &event)
{
	m_EarthPosDown = m_EarthPosLast = m_EarthPos;
	m_MouseDown = event.pos;

	// In move mode, always start dragging
	m_bDragging = true;

	// Press 'shift' key to drag vertically instead of horizontally
	if ((event.flags & VT_SHIFT) != 0)
		m_bDragUpDown = true;
}

void Enviro::OnMouseLeftUp(vtMouseEvent &event)
{
	m_bDragging = m_bDragUpDown = m_bRotating = false;

	if (m_state == AS_Orbit && m_mode == MM_MEASURE && m_bDragging)
		UpdateEarthArc();

	if (m_mode == MM_SELECTBOX)
		OnMouseLeftUpBox(event);
}

void Enviro::OnMouseLeftUpBox(vtMouseEvent &event)
{
	// Hide the rubber box
	SetWindowBox(IPoint2(0,0), IPoint2(0,0));

	// Do selection in window (HUD) coordinates
	// Frustum's origin is lower left, mouse origin is upper left
	IPoint2 winsize = vtGetScene()->GetWindowSize();

	FRECT select_box(m_MouseDown.x, winsize.y-1-m_MouseDown.y,
					 event.pos.x, winsize.y-1-event.pos.y);
	select_box.Sort();

	// We can use the view matrix to project each 3d object onto the window,
	//  to compare them against our box.
	FMatrix4 viewmat;
	vtGetScene()->ComputeViewMatrix(viewmat);

	vtTerrain *terr = GetCurrentTerrain();
	float fVerticalExag = terr->GetVerticalExag();
	LayerSet &layers = terr->GetLayers();
	for (unsigned int i = 0; i < layers.GetSize(); i++)
	{
		vtAbstractLayer *alay = dynamic_cast<vtAbstractLayer*>(layers[i]);
		if (!alay)
			continue;

		vtFeatureSet *fset = alay->GetFeatureSet();
		vtFeatureSetPoint2D *pset2 = dynamic_cast<vtFeatureSetPoint2D*>(fset);
		vtFeatureSetPoint3D *pset3 = dynamic_cast<vtFeatureSetPoint3D*>(fset);
		if (pset2 || pset3)
		{
			FBox3 bbox;
			for (unsigned int j = 0; j < fset->GetNumEntities(); j++)
			{
				vtFeature *feat = fset->GetFeature(j);
				Visual *viz = alay->GetViz(feat);
				if (!viz)
					continue;

				// Control key extends selection
				if (!(event.flags & VT_CONTROL))
					fset->Select(j, false);

				bool bSelected = false;
				for (unsigned int k = 0; k < viz->m_meshes.size(); k++)
				{
					vtMesh *mesh = viz->m_meshes[k];
					mesh->GetBoundBox(bbox);
					FPoint3 center = bbox.Center();

					// Account for potential vertical exaggeration
					center.y *= fVerticalExag;

					// Project 3d pos to 2d window pos
					FPoint3 frustump = viewmat.PreMult(center);

					// If inside the window box
					if (select_box.ContainsPoint(frustump.x, frustump.y) &&
						frustump.z > 0 &&
						frustump.z < 1)		// and in front of camera
					{
						bSelected = true;
					}
				}
				if (bSelected)
					fset->Select(j, true);
			}
			// Make the selected meshes yellow
			alay->UpdateVisualSelection();
		}
	}
	UpdateLayerView();
}

void Enviro::OnMouseRightDown(vtMouseEvent &event)
{
	if (m_mode == MM_BUILDINGS && m_bLineDrawing)
	{
		// Hide the temporary markers which showed the vertices
		PolygonSelectionClose();

		// Close and create new building in the current structure array
		vtTerrain *pTerr = GetCurrentTerrain();
		vtStructureArray3d *structures = pTerr->GetStructureLayer();
		vtBuilding3d *pbuilding = (vtBuilding3d*) structures->AddNewBuilding();

		// Force footprint anticlockwise
		PolyChecker PolyChecker;
		if (PolyChecker.IsClockwisePolygon(m_NewLine))
			m_NewLine.ReverseOrder();
		pbuilding->SetFootprint(0, m_NewLine);

		// Describe the appearance of the new building
		pbuilding->SetStories(2);
		pbuilding->SetRoofType(ROOF_HIP);
		pbuilding->SetColor(BLD_BASIC, RGBi(255,255,255));
		pbuilding->SetColor(BLD_ROOF, RGBi(230,200,170));

		// Construct it and add it to the terrain
		pbuilding->CreateNode(pTerr);
		pTerr->AddNodeToStructGrid(pbuilding->GetContainer());
		RefreshLayerView();
	}
}

void Enviro::OnMouseRightUp(vtMouseEvent &event)
{
	if (m_state == AS_Terrain)
	{
		// close off the fence if we have one
		if (m_mode == MM_FENCES)
			close_fence();
		if (m_mode == MM_ROUTES)
			close_route();
		if (m_mode == MM_SELECT)
		{
			vtTerrain *t = GetCurrentTerrain();
			vtStructureArray3d *sa = t->GetStructureLayer();
			vtPlantInstanceArray3d &plants = t->GetPlantInstances();

			if (sa->NumSelected() != 0 || plants.NumSelected() != 0 ||
				m_Vehicles.GetSelected() != -1)
				ShowPopupMenu(event.pos);
		}
		if (m_mode == MM_SELECTBOX)
		{
			if (NumSelectedAbstractFeatures() > 0)
				ShowPopupMenu(event.pos);
		}
	}
}

void Enviro::OnMouseMove(vtMouseEvent &event)
{
	if (m_state == AS_Terrain)
		OnMouseMoveTerrain(event);

	if (m_state == AS_Orbit && m_mode == MM_MEASURE && m_bDragging)
		UpdateEarthArc();
}

void Enviro::OnMouseMoveTerrain(vtMouseEvent &event)
{
	if ((m_mode == MM_SELECT || m_mode == MM_MOVE) &&
		(m_bDragging || m_bRotating))
	{
		DPoint3 delta = m_EarthPos - m_EarthPosLast;
		DPoint2 ground_delta(delta.x, delta.y);

		//VTLOG("ground_delta %f, %f\n", delta.x, delta.y);

		float fNewRotation = m_StartRotation + (event.pos.x - m_MouseDown.x) / 100.0f;

		vtTerrain *pTerr = GetCurrentTerrain();

		vtStructureArray3d *structures = pTerr->GetStructureLayer();
		if (structures && structures->NumSelected() > 0)
		{
			if (m_bDragging)
			{
				if (m_bDragUpDown)
				{
					// Moving a whole structure (building or instance)
					float fDelta = (m_MouseLast.y - event.pos.y) / 20.0f;
					structures->OffsetSelectedStructuresVertical(fDelta);
				}
				else if (m_pDraggingFence != NULL)
				{
					// Dragging a linear structure point
					DLine2 &pts = m_pDraggingFence->GetFencePoints();
					pts[m_iDraggingFencePoint] += ground_delta;
					m_pDraggingFence->CreateNode(pTerr);
				}
				else
				{
					// Moving a whole structure (building or instance)
					structures->OffsetSelectedStructures(ground_delta);
				}
			}
			else if (m_bRotating)
			{
				for (int sel = structures->GetFirstSelected(); sel != -1; sel = structures->GetNextSelected())
				{
					vtStructInstance *inst = structures->GetAt(sel)->GetInstance();
					vtStructInstance3d *str3d = structures->GetInstance(sel);

					inst->SetRotation(fNewRotation);
					str3d->UpdateTransform(pTerr->GetHeightField());
				}
			}
		}
		if (m_bDragging)
		{
			if (m_bSelectedPlant)
			{
				vtPlantInstanceArray3d &plants = pTerr->GetPlantInstances();
				plants.OffsetSelectedPlants(ground_delta);
			}
			if (m_bSelectedUtil)
			{
				vtRouteMap &routemap = pTerr->GetRouteMap();
				m_pSelUtilNode->Offset(ground_delta);
				m_pSelRoute->Dirty();
				routemap.BuildGeometry(pTerr->GetHeightField());
			}
			if (m_bSelectedVehicle)
			{
				CarEngine *eng = m_Vehicles.GetSelectedCarEngine();
				if (eng)
					eng->SetEarthPos(eng->GetEarthPos() + ground_delta);
			}
		}
		else if (m_bRotating)
		{
			if (m_bSelectedVehicle)
			{
				CarEngine *eng = m_Vehicles.GetSelectedCarEngine();
				if (eng)
					eng->SetRotation(fNewRotation);
			}
		}
		m_EarthPosLast = m_EarthPos;
	}
	if (m_mode == MM_SELECTBOX && m_bDragging)
	{
		SetWindowBox(m_MouseDown, event.pos);
	}
	if (m_mode == MM_MEASURE && m_bDragging && m_bOnTerrain)
	{
//		VTLOG("MouseMove, MEASURE & Drag & OnTerrain: %.1lf, %.1lf\n", m_EarthPos.x, m_EarthPos.y);
		if (m_bMeasurePath)
		{
			DPoint2 g2(m_EarthPos.x, m_EarthPos.y);
			unsigned int len = m_distance_path.GetSize();
			if (len > 1)
				m_distance_path[len-1] = g2;
			SetTerrainMeasure(m_distance_path);
		}
		else
		{
			DPoint2 g1(m_EarthPosDown.x, m_EarthPosDown.y);
			DPoint2 g2(m_EarthPos.x, m_EarthPos.y);
			SetTerrainMeasure(g1, g2);
		}
		UpdateDistanceTool();
	}
}

bool Enviro::OnMouseCompass(vtMouseEvent &event)
{
	if (m_pCompassSizer)
	{
		if (m_bDragCompass)
		{
			if (event.type == VT_UP)
				m_bDragCompass = false;

			if (event.type == VT_MOVE)
			{
				FPoint2 center = m_pCompassSizer->GetWindowCenter();
				FPoint2 pos(event.pos.x, event.pos.y);
				FPoint2 diff = (pos - center);
				float angle = atan2(diff.y, diff.x);
				float delta = angle - m_fDragAngle;
				m_fDragAngle = angle;

				vtCamera *cam = vtGetScene()->GetCamera();
				cam->RotateParent(FPoint3(0,1.0f,0), delta);
			}
		}
		else if (event.type == VT_DOWN)
		{
			FPoint2 center = m_pCompassSizer->GetWindowCenter();
			FPoint2 pos(event.pos.x, event.pos.y);
			FPoint2 diff = (pos - center);
			float dist = diff.Length();
			if (dist > 20 && dist < 64)
			{
				m_bDragCompass = true;
				m_fDragAngle = atan2(diff.y, diff.x);
			}
		}

		if (m_bDragCompass)
			return false;
	}
	return true;
}


void Enviro::SetupArcMesh()
{
	if (!m_pArcMats)
	{
		m_pArcMats = new vtMaterialArray;
		m_pArcMats->AddRGBMaterial1(RGBf(1, 1, 0), false, false); // yellow
		m_pArcMats->AddRGBMaterial1(RGBf(1, 0, 0), false, false); // red
		m_pArcMats->AddRGBMaterial1(RGBf(1, 0.5f, 0), true, true); // orange lit
	}
	// create geometry container, if needed
	if (!m_pArc)
	{
		m_pArc = new vtGeom;
		if (m_state == AS_Orbit)
			m_pIcoGlobe->GetTop()->AddChild(m_pArc);
		else if (m_state == AS_Terrain)
			GetCurrentTerrain()->GetTopGroup()->AddChild(m_pArc);
		m_pArc->SetMaterials(m_pArcMats);
	}

	// re-create mesh if not the first time
	FreeArcMesh();
}

void Enviro::FreeArc()
{
	FreeArcMesh();
	if (m_pArc)
	{
		m_pArc->Release();
		m_pArc = NULL;
	}
}

void Enviro::FreeArcMesh()
{
	if (m_pArc)
	{
		for (int i = m_pArc->GetNumMeshes()-1; i >= 0; i--)
			m_pArc->RemoveMesh(m_pArc->GetMesh(i));
	}
}

void Enviro::SetTerrainMeasure(const DPoint2 &g1, const DPoint2 &g2)
{
	// place the arc for the distance measuring tool on the terrain
	SetupArcMesh();

	DLine2 dline;
	dline.Append(g1);
	dline.Append(g2);

	vtTerrain *pTerr = GetCurrentTerrain();
	vtMeshFactory mf(m_pArc, vtMesh::LINE_STRIP, 0, 30000, 1);
	mf.SetLineWidth(2);
	m_fArcLength = pTerr->AddSurfaceLineToMesh(&mf, dline, m_fDistToolHeight, true);
}

void Enviro::SetTerrainMeasure(const DLine2 &path)
{
	// place the arc for the distance measuring tool on the terrain
	SetupArcMesh();

	vtTerrain *pTerr = GetCurrentTerrain();
	vtMeshFactory mf(m_pArc, vtMesh::LINE_STRIP, 0, 30000, 1);
	mf.SetLineWidth(2);
	m_fArcLength = pTerr->AddSurfaceLineToMesh(&mf, path, m_fDistToolHeight, true);
}

void Enviro::SetDistanceToolMode(bool bPath)
{
	// if switching modes, reset
	bool bNeedReset = (m_bMeasurePath != bPath);

	m_bMeasurePath = bPath;

	if (bNeedReset)
		ResetDistanceTool();
}

void Enviro::ResetDistanceTool()
{
	m_distance_path.Empty();
	m_fArcLength = 0.0;
	if (m_bMeasurePath)
		ShowDistance(DLine2(), FLT_MIN, FLT_MIN);
	else
		ShowDistance(DPoint2(0,0), DPoint2(0,0), FLT_MIN, FLT_MIN);

	// remove visible terrain line
	FreeArcMesh();
}

void Enviro::UpdateDistanceTool()
{
	if (m_bMeasurePath)
		ShowDistance(m_distance_path, m_fArcLength, m_EarthPos.z - m_EarthPosDown.z);
	else
	{
		DPoint2 g1(m_EarthPosDown.x, m_EarthPosDown.y);
		DPoint2 g2(m_EarthPos.x, m_EarthPos.y);
		ShowDistance(g1, g2, m_fArcLength, m_EarthPos.z - m_EarthPosDown.z);
	}
}

// Wind
void Enviro::SetWind(int iDirection, float fSpeed)
{
	// Store these properties on the terrain
	vtTerrain *terr = GetCurrentTerrain();
	TParams &params = terr->GetParams();
	params.SetValueInt("WindDirection", iDirection);
	params.SetValueFloat("WindSpeed", fSpeed);
}

//
// Use the current cursor position as a point to add to a polygon being
//  visually selected.
//
void Enviro::PolygonSelectionAddPoint()
{
	SetupArcMesh();
	DPoint2 g1(m_EarthPos.x, m_EarthPos.y);

	// Create a marker pole for this corner of the new building
	int matidx = 2;
	float fHeight = 10.0f;
	float fRadius = 0.2f;
	vtGeom *geom = CreateCylinderGeom(m_pArcMats, matidx, VT_Normals, fHeight,
		fRadius, 10, true, false, false, 1);
	vtTransform *trans = new vtTransform;
	trans->AddChild(geom);
	m_Markers.push_back(trans);
	vtTerrain *pTerr = GetCurrentTerrain();
	pTerr->PlantModelAtPoint(trans, g1);
	pTerr->AddNode(trans);

	if (m_bLineDrawing)
	{
		// continue existing line
		m_NewLine.Append(g1);

		vtTerrain *pTerr = GetCurrentTerrain();
		vtMeshFactory mf(m_pArc, vtMesh::LINE_STRIP, 0, 30000, 1);
		pTerr->AddSurfaceLineToMesh(&mf, m_NewLine, m_fDistToolHeight, true);
	}
	else
	{
		// start new line
		m_bLineDrawing = true;
		m_NewLine.Empty();
		m_NewLine.Append(g1);
	}
}

void Enviro::PolygonSelectionClose()
{
	m_bLineDrawing = false;

	// Hide the temporary markers which showed the vertices
	SetupArcMesh();
	for (unsigned int i = 0; i < m_Markers.size(); i++)
	{
		GetCurrentTerrain()->RemoveNode(m_Markers[i]);
		m_Markers[i]->Release();
	}
	m_Markers.clear();
}


////////////////////////////////////////////////////////////////
// Fences

void Enviro::start_new_fence()
{
	vtFence3d *fence = new vtFence3d;
	fence->SetParams(m_FenceParams);
	if (GetCurrentTerrain()->AddFence(fence))
	{
		m_pCurFence = fence;

		// update count shown in layer view
		RefreshLayerView();
	}
	else
		delete fence;
}

void Enviro::finish_fence()
{
	m_bActiveFence = false;
}

void Enviro::close_fence()
{
	if (m_bActiveFence && m_pCurFence)
	{
		DLine2 &pts = m_pCurFence->GetFencePoints();
		if (pts.GetSize() > 2)
		{
			DPoint2 FirstFencePoint = pts.GetAt(0);
			m_pCurFence->AddPoint(FirstFencePoint);
			GetCurrentTerrain()->RedrawFence(m_pCurFence);
		}
	}
	m_bActiveFence = false;
}

void Enviro::SetFenceOptions(const vtLinearParams &param, bool bProfileChanged)
{
	m_FenceParams = param;

	vtTerrain *pTerr = GetCurrentTerrain();
	if (!pTerr)
		return;

	if (m_bActiveFence)
	{
		m_pCurFence->SetParams(param);
		if (bProfileChanged)
			m_pCurFence->ProfileChanged();
		m_pCurFence->CreateNode(pTerr);	// re-create
	}

	vtStructureArray3d *structures = pTerr->GetStructureLayer();
	for (unsigned int i = 0; i < structures->GetSize(); i++)
	{
		vtStructure *str = structures->GetAt(i);
		if (!str->IsSelected() || str->GetType() != ST_LINEAR)
			continue;
		vtFence3d *pFence = structures->GetFence(i);
		pFence->SetParams(param);
		if (bProfileChanged)
			pFence->ProfileChanged();
		pFence->CreateNode(pTerr);	// re-create
	}
}


////////////////////////////////////////////////////////////////
// Route

void Enviro::start_new_route()
{
	vtRoute *route = new vtRoute(GetCurrentTerrain());
	GetCurrentTerrain()->AddRoute(route);
	m_pCurRoute = route;
}

void Enviro::finish_route()
{
	m_bActiveRoute = false;
}

void Enviro::close_route()
{
	if (m_bActiveRoute && m_pCurRoute)
	{
		GetCurrentTerrain()->SaveRoute();
	}
	m_bActiveRoute = false;
}

void Enviro::SetRouteOptions(const vtString &sStructType)
{
	m_sStructType = sStructType;
}


////////////////////////////////////////////////
// Plants

void Enviro::SetPlantOptions(const PlantingOptions &opt)
{
	m_PlantOpt = opt;
	if (m_mode == MM_SELECT)
	{
		vtPlantInstanceArray3d &pia = GetCurrentTerrain()->GetPlantInstances();
		for (unsigned int i = 0; i < pia.GetNumEntities(); i++)
		{
			if (pia.IsSelected(i))
			{
				pia.SetPlant(i, opt.m_fHeight, opt.m_iSpecies);
				pia.CreatePlantNode(i);
			}
		}
	}
}

void Enviro::SetVehicleOptions(const VehicleOptions &opt)
{
	m_VehicleOpt = opt;
}

/**
 * Plant a tree at the given location (in earth coordinates)
 */
bool Enviro::PlantATree(const DPoint2 &epos)
{
	if (!m_pPlantList)
		return false;

	vtTerrain *pTerr = GetCurrentTerrain();
	if (!pTerr)
		return false;

	// check distance from other plants
	vtPlantInstanceArray &pia = pTerr->GetPlantInstances();
	int size = pia.GetNumEntities();
	double len, closest = 1E8;
	DPoint2 diff;

	bool bPlant = true;
	if (m_PlantOpt.m_fSpacing > 0.0f)
	{
		// Spacing is in meters, but the picking functions work in
		//  Earth coordinates.  Try to convert it to earth horiz units.
		DPoint2 eoffset;
		g_Conv.ConvertVectorToEarth(m_PlantOpt.m_fSpacing, 0, eoffset);
		double epsilon = eoffset.x;

		for (int i = 0; i < size; i++)
		{
			diff = epos - pia.GetPoint(i);
			len = diff.Length();

			if (len < closest) closest = len;
		}
		if (closest < epsilon)
			bPlant = false;
		VTLOG(" closest plant %.2fm,%s planting..", closest, bPlant ? "" : " not");
	}
	if (!bPlant)
		return false;

	float height = m_PlantOpt.m_fHeight;
	float variance = m_PlantOpt.m_iVariance / 100.0f;
	height *= (1.0f + random(variance*2) - variance);
	if (!pTerr->AddPlant(epos, m_PlantOpt.m_iSpecies, height))
		return false;

	// If there is a GUI, let it update to show one more plant
	UpdateLayerView();
	return true;
}


//// Instances

void Enviro::PlantInstance()
{
#if 0	// test code
	#include "CreateWedge.cpp"
	return;
#endif

	VTLOG("Plant Instance: ");
	vtTagArray *tags = GetInstanceFromGUI();
	if (!tags)
		return;

	// create a new Instance object
	vtTerrain *pTerr = GetCurrentTerrain();
	vtStructureArray3d *structs = pTerr->GetStructureLayer();
	vtStructInstance3d *inst = (vtStructInstance3d *) structs->NewInstance();
	inst->CopyTagsFrom(*tags);
	inst->SetPoint(DPoint2(m_EarthPos.x, m_EarthPos.y));
	VTLOG("  at %.7g, %.7g: ", m_EarthPos.x, m_EarthPos.y);

	// Allow the rest of the framework to 'ornament' this instance by
	//  extending it as desired.
	ExtendStructure(inst);

	int index = structs->Append(inst);
	bool success = pTerr->CreateStructure(structs, index);
	if (success)
	{
		VTLOG(" succeeded.\n");
		RefreshLayerView();
	}
	else
	{
		// creation failed
		VTLOG(" failed.\n");
		ShowMessage("Could not create instance.");
		inst->Select(true);
		structs->DeleteSelected();
		return;
	}
}

void Enviro::DescribeCoordinatesTerrain(vtString &str)
{
	DPoint3 epos;
	vtString str1;

	str = "";

#if 0
	// give location of camera and cursor
	str = "Camera: ";
	// get camera pos
	vtScene *scene = vtGetScene();
	vtCamera *camera = scene->GetCamera();
	FPoint3 campos = camera->GetTrans();

	// Find corresponding earth coordinates
	g_Conv.ConvertToEarth(campos, epos);

	FormatCoordString(str1, epos, g_Conv.GetUnits());
	str += str1;
	str1.Format(" elev %.1f", epos.z);
	str += str1;
	str += ", ";
#endif

	// ground cursor
	str += _("Cursor: ");
	bool bOn = m_pTerrainPicker->GetCurrentEarthPos(epos);
	if (bOn)
	{
		vtString str1;
		FormatCoordString(str1, epos, g_Conv.GetUnits(), true);
		str += str1;
	}
	else
		str += _("Not on ground");
}

void Enviro::DescribeCLOD(vtString &str)
{
	str = "";

	if (m_state != AS_Terrain) return;
	vtTerrain *t = GetCurrentTerrain();
	if (!t) return;
	vtDynTerrainGeom *dtg = t->GetDynTerrain();
	if (!dtg) return;

	// McNally and Roettger CLOD algos use a triangle/vertex count target.
	//  The older implementations use a floating point factor relating to
	//  error/detail.
	//
	LodMethodEnum method = t->GetParams().GetLodMethod();
	if (method == LM_MCNALLY || method == LM_ROETTGER)
	{
		str.Format("CLOD: target %d, drawn %d ", dtg->GetPolygonTarget(),
			dtg->GetNumDrawnTriangles());
	}
}

vtString Enviro::GetStatusString(int which)
{
	vtScene *scene = vtGetScene();

	vtString str;
	if (which == 0)
	{
		// Fps: get framerate
		float fps = scene->GetFrameRate();

		// only show 3 significant digits
		if (fps < 10)
			str.Format("fps %1.2f", fps);
		else if (fps < 80)
			str.Format("fps %2.1f", fps);
		else
			str.Format("fps %3.0f", fps);

		return str;
	}
	if (which == 1)
	{
		if (m_state == AS_Orbit)
			DescribeCoordinatesEarth(str);
		else if (m_state == AS_Terrain)
			DescribeCoordinatesTerrain(str);
	}
	if (which == 2)
	{
		DPoint3 epos;

		if (m_state == AS_Orbit)
		{
			m_pGlobePicker->GetCurrentEarthPos(epos);
			vtTerrain *pTerr = FindTerrainOnEarth(DPoint2(epos.x, epos.y));
			if (pTerr)
				str = pTerr->GetName();
		}
		else if (m_state == AS_Terrain)
		{
			bool bOn = m_pTerrainPicker->GetCurrentEarthPos(epos);
			if (bOn)
			{
				float exag;
				{
					// Avoid trouble with '.' and ',' in Europe
					LocaleWrap normal_numbers(LC_NUMERIC, "C");
					// Report true elevation, without vertical exaggeration
					exag = GetCurrentTerrain()->GetVerticalExag();
				}
				epos.z /= exag;
				str = _("Elev: ");
				vtString str2;
				str2.Format("%.1f", epos.z);
				str += str2;
			}
			else
				str += _("Not on ground");
		}
	}
	return str;
}

// Handle the map overview option
void Enviro::ShowMapOverview(bool bShow)
{
	if (bShow && !m_pMapOverview)
		CreateMapOverview();
	if (m_pMapOverview)
		m_pMapOverview->ShowMapOverview(bShow);
}

bool Enviro::GetShowMapOverview()
{
	if (m_pMapOverview)
		return m_pMapOverview->GetShowMapOverview();
	return false;
}

void Enviro::CreateMapOverview()
{
	// setup the mapoverview engine
	if (!m_pMapOverview)
	{
		m_pMapOverview = new MapOverviewEngine;
		m_pMapOverview->SetName2("Map overview engine");
		vtGetScene()->AddEngine(m_pMapOverview);
	}
}

void Enviro::TextureHasChanged()
{
	// Texture has changed, so update the overview
	if (m_pMapOverview && m_pMapOverview->GetShowMapOverview())
	{
		m_pMapOverview->SetTerrain(GetCurrentTerrain());
	}
}

void Enviro::ShowElevationLegend(bool bShow)
{
	if (bShow && !m_bCreatedLegend)
		CreateElevationLegend();
	if (m_pLegendGeom)
		m_pLegendGeom->SetEnabled(bShow);
}

bool Enviro::GetShowElevationLegend()
{
	if (m_pLegendGeom)
		return m_pLegendGeom->GetEnabled();
	return false;
}

void Enviro::ShowCompass(bool bShow)
{
	if (bShow && !m_bCreatedCompass)
		CreateCompass();
	if (m_pCompassGeom)
		m_pCompassGeom->SetEnabled(bShow);
}

bool Enviro::GetShowCompass()
{
	if (m_pCompassGeom)
		return m_pCompassGeom->GetEnabled();
	return false;
}

void Enviro::UpdateCompass()
{
	vtCamera *cam = vtGetScene()->GetCamera();
	if (!cam)
		return;
	FPoint3 dir = cam->GetDirection();
	float theta = atan2(dir.z, dir.x) + PID2f;
	if (m_pCompassSizer)
	{
		m_pCompassSizer->SetRotation(theta);
		IPoint2 size = vtGetScene()->GetWindowSize();
		m_pCompassSizer->OnWindowSize(size.x, size.y);
	}
}

void Enviro::CreateElevationLegend()
{
	// Must have a color-mapped texture on the terrain to show a legend
	ColorMap *cmap = GetCurrentTerrain()->GetTextureColors();
	if (!cmap)
		return;

	// Define the size and shape of the legend: input values
	const int ticks = 8;
	const IPoint2 border(10, 18);
	const IPoint2 base(10, 10);
	const IPoint2 size(140, 230);
	const int fontsize = 16;

	// Derived values
	const IPoint2 in_base = base + border;
	const IPoint2 in_size(size.x - (border.x*2), size.y - (border.y*2));
	const int vert_space = in_size.y / (ticks-1);
	const int cbar_left = in_base.x + (in_size.x * 6 / 10);
	const int cbar_right = in_base.x + in_size.x;

	int i, idx;
	int white = m_pHUDMaterials->AddRGBMaterial1(RGBf(1, 1, 1), false, false); // white
	int grey = m_pHUDMaterials->AddRGBMaterial1(RGBf(.2, .2, .2), false, false); // dark grey

	m_pLegendGeom = new vtGeom;
	m_pLegendGeom->SetName2("Legend");
	m_pLegendGeom->SetMaterials(m_pHUDMaterials);

	// Solid rectangle behind it
	vtMesh *mesh4 = new vtMesh(vtMesh::QUADS, 0, 4);
	mesh4->AddRectangleXY(base.x, base.y, size.x, size.y, -1.0f);
	m_pLegendGeom->AddMesh(mesh4, grey);
	mesh4->Release();

	float fMin, fMax;
	GetCurrentTerrain()->GetHeightField()->GetHeightExtents(fMin, fMax);

	// Big band of color
	std::vector<RGBi> table;
	cmap->GenerateColors(table, in_size.y, fMin, fMax);
	vtMesh *mesh1 = new vtMesh(vtMesh::TRIANGLE_STRIP, VT_Colors, (in_size.y + 1)*2);
	for (i = 0; i < in_size.y + 1; i++)
	{
		FPoint3 p1(cbar_left,  in_base.y + i, 0.0f);
		FPoint3 p2(cbar_right, in_base.y + i, 0.0f);
		idx = mesh1->AddLine(p1, p2);
		mesh1->SetVtxColor(idx, (RGBf) table[i]);
		mesh1->SetVtxColor(idx+1, (RGBf) table[i]);
	}
	mesh1->AddStrip2((in_size.y + 1)*2, 0);
	m_pLegendGeom->AddMesh(mesh1, white);
	mesh1->Release();

	// Small white tick marks
	vtMesh *mesh2 = new vtMesh(vtMesh::LINES, 0, ticks*2);
	for (i = 0; i < ticks; i++)
	{
		FPoint3 p1(cbar_left-border.x*2, in_base.y + i*vert_space, 0.0f);
		FPoint3 p2(cbar_left,			 in_base.y + i*vert_space, 0.0f);
		mesh2->AddLine(p1, p2);
	}
	m_pLegendGeom->AddMesh(mesh2, white);
	mesh2->Release();

	// Text labels
	for (i = 0; i < ticks; i++)
	{
		vtTextMesh *mesh3 = new vtTextMesh(m_pArial, fontsize, false);
		vtString str;
		str.Format("%4.1f", fMin + (fMax - fMin) / (ticks-1) * i);
		mesh3->SetText(str);
		FPoint3 p1(in_base.x, in_base.y + i*vert_space - (fontsize*1/3), 0.0f);
		mesh3->SetPosition(p1);

		m_pLegendGeom->AddTextMesh(mesh3, white);
		mesh3->Release();
	}

	m_pHUD->AddChild(m_pLegendGeom);
	m_bCreatedLegend = true;
}

void Enviro::CreateCompass()
{
	// Define the size and shape of the compass
	const IPoint2 base(310, 10);
	const IPoint2 size(128, 128);

	VTLOG1("Loading compass..\n");
	vtString path = FindFileOnPaths(vtGetDataPath(), "compass.png");
	if (path == "")
	{
		VTLOG1(" not found.\n");
		return;
	}

	vtImageSprite *CompassSprite = new vtImageSprite;
	bool success = CompassSprite->Create(path, true);	// blending = true
	if (!success)
		return;

	m_pCompassSizer = new vtSpriteSizer(CompassSprite, -133, -5, -5, -133);
	m_pCompassSizer->SetName2("Sizer for Compass");
	vtGetScene()->AddEngine(m_pCompassSizer);

	m_pCompassGeom = (vtGeom *) CompassSprite->GetNode();
	m_pCompassGeom->SetName2("Compass");
	m_pHUD->AddChild(m_pCompassGeom);
	m_bCreatedCompass = true;
}

void Enviro::SetWindowBox(const IPoint2 &p1, const IPoint2 &p2)
{
	if (!m_pWindowBoxMesh)
	{
		// create a yellow wireframe polygon we can stretch later
		int yellow = m_pHUDMaterials->AddRGBMaterial1(RGBf(1,1,0), false, false, true);
		vtGeom *geom = new vtGeom;
		geom->SetName2("Selection Box");
		geom->SetMaterials(m_pHUDMaterials);
		m_pWindowBoxMesh = new vtMesh(vtMesh::POLYGON, 0, 4);
		m_pWindowBoxMesh->AddVertex(0,0,0);
		m_pWindowBoxMesh->AddVertex(1,0,0);
		m_pWindowBoxMesh->AddVertex(1,1,0);
		m_pWindowBoxMesh->AddVertex(0,1,0);
		m_pWindowBoxMesh->AddStrip2(4, 0);
		geom->AddMesh(m_pWindowBoxMesh, yellow);
		m_pWindowBoxMesh->Release();	// pass ownership
		m_pHUD->AddChild(geom);
	}
	// Invert the coordinates Y, because mouse origin is upper left, and
	//  HUD origin is lower left
	IPoint2 winsize = vtGetScene()->GetWindowSize();

	m_pWindowBoxMesh->SetVtxPos(0, FPoint3(p1.x,winsize.y-1-p1.y,0));
	m_pWindowBoxMesh->SetVtxPos(1, FPoint3(p2.x,winsize.y-1-p1.y,0));
	m_pWindowBoxMesh->SetVtxPos(2, FPoint3(p2.x,winsize.y-1-p2.y,0));
	m_pWindowBoxMesh->SetVtxPos(3, FPoint3(p1.x,winsize.y-1-p2.y,0));
	m_pWindowBoxMesh->ReOptimize();
}


////////////////////////////////////////////////////////////////////////
// Vehicles

#include "CarEngine.h"

void Enviro::CreateGroundVehicle(const VehicleOptions &opt)
{
	// Create test vehicle
	vtTerrain *pTerr = GetCurrentTerrain();
	if (!pTerr)
		return;

	DPoint3 epos;
	bool bOn = m_pTerrainPicker->GetCurrentEarthPos(epos);
	if (!bOn)
		return;

	vtTransform *car = m_VehicleManager.CreateVehicle(opt.m_Itemname, opt.m_Color);
	if (!car)
		return;
	pTerr->AddNode(car);

	pTerr->PlantModelAtPoint(car, DPoint2(epos.x, epos.y));

	float speed = 0.0f;		// kmph
	float wheel_radius = 0.25f;
	CarEngine *pE1 = new CarEngine(pTerr->GetHeightField(), speed, wheel_radius, car->GetTrans());
	pE1->SetName2("drive");
	pE1->SetTarget(car);
	if (pE1->FindWheelTransforms())
	{
		pTerr->AddEngine(pE1);
		m_Vehicles.AddEngine(pE1);
	}
}

void Enviro::CreateSomeTestVehicles(vtTerrain *pTerrain, unsigned int iNum, float fSpeed)
{
	vtRoadMap3d *pRoadMap = pTerrain->GetRoadMap();

	// How many four-wheel land vehicles are there in the content catalog?
	vtStringArray vnames;
	vtContentManager3d &con = vtGetContent();
	for (unsigned int i = 0; i < con.NumItems(); i++)
	{
		vtItem *item = con.GetItem(i);
		const char *type = item->GetValueString("type");
		int wheels = item->GetValueInt("num_wheels");
		if (type && vtString(type) == "ground vehicle" && wheels == 4)
		{
			vnames.push_back(item->m_name);
		}
	}
	unsigned int numv = vnames.size();

	// add some test vehicles
	NodeGeom *road_node = NULL;
	for (unsigned int i = 0; i < iNum; i++)
	{
		if (road_node == NULL)
			road_node = pRoadMap->GetFirstNode();

		RGBf color;
		switch (i % 8)
		{
		case 0: color.Set(1.0f, 1.0f, 1.0f); break;	// white
		case 1: color.Set(1.0f, 1.0f, 0.0f); break; // yellow
		case 2: color.Set(0.3f, 0.6f, 1.0f); break; // medium blue
		case 3: color.Set(1.0f, 0.3f, 0.3f); break; // red
		case 4: color.Set(0.5f, 1.0f, 0.5f); break; // medium green
		case 5: color.Set(0.2f, 0.2f, 0.2f); break; // black/dk.grey
		case 6: color.Set(0.1f, 0.6f, 0.1f); break; // dk.green
		case 7: color.Set(1.0f, 0.8f, 0.6f); break; // tan
		}

		// Create some of each land vehicle type
		int vnum = i % numv;
		vtTransform *car = m_VehicleManager.CreateVehicle(vnames[vnum], color);
		if (car)
		{
			pTerrain->AddNode(car);
			pTerrain->PlantModelAtPoint(car, road_node->m_p);

			float fSpeed = 60.0f;	// kmph

			CarEngine *pE1;
			if (road_node == NULL)
			{
				pE1 = new CarEngine(pTerrain->GetHeightField(), fSpeed, .25f,
					car->GetTrans());
			}
			else
			{
				pE1 = new CarEngine(pTerrain->GetHeightField(), fSpeed, .25f,
					road_node, 1);
			}
			pE1->SetName2("drive");
			pE1->SetTarget(car);
			if (pE1->FindWheelTransforms())
				pTerrain->AddEngine(pE1);
			else
				delete pE1;
			m_Vehicles.AddEngine(pE1);
		}
		road_node = (NodeGeom*) road_node->m_pNext;
	}
}


////////////////////////////////////////////////////////////////////////
// Abstract Layers

// Find an appropriate point layer for labels.
vtAbstractLayer *Enviro::GetLabelLayer()
{
	vtTerrain *pTerr = GetCurrentTerrain();
	if (!pTerr)
		return false;
	LayerSet &layers = pTerr->GetLayers();
	for (unsigned int i = 0; i < layers.GetSize(); i++)
	{
		vtAbstractLayer *alay = dynamic_cast<vtAbstractLayer*>(layers[i]);
		if (!alay)
			continue;
		if (alay->GetFeatureSet()->GetGeomType() == wkbPoint &&
			alay->GetFeatureSet()->GetField("Label") != NULL)
			return alay;
	}
	return NULL;
}

int Enviro::NumSelectedAbstractFeatures()
{
	vtTerrain *pTerr = GetCurrentTerrain();
	if (!pTerr)
		return 0;
	LayerSet &layers = pTerr->GetLayers();
	int count = 0;
	for (unsigned int i = 0; i < layers.GetSize(); i++)
	{
		vtAbstractLayer *alay = dynamic_cast<vtAbstractLayer*>(layers[i]);
		if (alay)
			count += alay->GetFeatureSet()->NumSelected();
	}
	return count;
}


////////////////////////////////////////////////////////////////////////

void ControlEngine::Eval()
{
	Enviro::s_pEnviro->DoControl();
}

////////////////////////////////////////////////////////////////////////

vtTerrain *GetCurrentTerrain()
{
	return Enviro::s_pEnviro->GetCurrentTerrain();
}

