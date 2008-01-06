//
// vtTerrainScene - Container class for all of the terrains loaded
//
// Copyright (c) 2001-2007 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#include "vtlib/vtlib.h"
#include "vtdata/vtLog.h"
#include "TerrainScene.h"
#include "Light.h"
#include "SkyDome.h"
#include "Terrain.h"
#include "TimeEngines.h"
#include "LodGrid.h"

///////////////////////////////////////////////////////////////////////

// All terrains share a static data path and content manager
vtTerrainScene *vtTerrainScene::s_pTerrainScene;

///////////////////////////////////////////////////////////////////////
/**
 * A small engine that allows the SkyDome to stay around the Camera.
 */
class vtSkyTrackEngine : public vtEngine
{
public:
	vtSkyTrackEngine();
	virtual void Eval();
	vtCamera *m_pCamera;
};

vtSkyTrackEngine::vtSkyTrackEngine() : vtEngine()
{
	m_pCamera = NULL;
}

void vtSkyTrackEngine::Eval()
{
	// get the location of the camera and the target (skydome)
	vtTransform *pTarget = (vtTransform *) GetTarget();
	if (!pTarget || !m_pCamera)
		return;
	FPoint3 pos1 = m_pCamera->GetTrans();
	FPoint3 pos2 = pTarget->GetTrans();

	// move the target (skydome) to be centers in XZ on the camera
	pos2.x = pos1.x;
	pos2.z = pos1.z;
	pTarget->SetTrans(pos2);
}


///////////////////////////////////////////////////////////////////////
//
// vtTerrainScene class
//

vtTerrainScene::vtTerrainScene()
{
	s_pTerrainScene = this;

	horizon_color.Set(0.70f, 0.85f, 1.0f);
	azimuth_color.Set(0.12f, 0.32f, 0.70f);

	m_pTop = NULL;
	m_pSkyDome = NULL;
	m_pCurrentTerrain = NULL;
	m_pTimeEngine = NULL;
	m_pSkyTrack = NULL;
	m_pSunLight = NULL;
	m_pAtmosphereGroup = NULL;

	m_fCatenaryFactor = 140.0;	// a default value
}

vtTerrainScene::~vtTerrainScene()
{
	VTLOG("TerrainScene destructing\n");
}

void vtTerrainScene::CleanupScene()
{
	VTLOG("vtTerrainScene::CleanupScene\n");

	m_Content.ReleaseContents();
	m_Content.Empty();

	SetCurrentTerrain(NULL);

	// no need to do this explicitly, it is done by releasing the scenegraph
	// if (m_pSkyDome)
	//	m_pSkyDome->Destroy();

	for (unsigned int i = 0; i < NumTerrains(); i++)
	{
		vtTerrain *curr = GetTerrain(i);
		vtGroup *group = curr->GetTopGroup();
		if (m_pTop != NULL && group != NULL)
			m_pTop->RemoveChild(group);
		delete curr;
	}
	m_Terrains.clear();
	m_pCurrentTerrain = NULL;

	for (unsigned int i = 0; i < m_StructObjs.GetSize(); i++)
		delete m_StructObjs[i];

	// get anything left at the top of the scene graph
	if (m_pTop)
		m_pTop->Release();

	// free some statics
	vtStructure3d::ReleaseSharedMaterials();
	vtRoute::ReleaseMaterials();
}

void vtTerrainScene::_CreateSky()
{
	// create the sun
	VTLOG(" Creating Main Light\n");

#if VTLIB_OPENSG==1
	//opensg handles light quite different to osg,
	//this requires to modify the internal vtp sg
	//m_pTop is a light group!
	//the actual light node (m_pTop) is now parent to all nodes to be lit.

	m_pTop->SetName2("Main Light");
	//m_pSunLight holds now only(!) the transform, which acts as the beacon node
	m_pSunLight = new vtTransform;
	m_pSunLight->SetName2("SunLight");

	//store like previously, but this time only the transform
	m_pTop->AddChild(m_pSunLight);

	//point to the sun transformation
	vtLight *pLight(0);
	pLight = dynamic_cast<vtLight*>(m_pTop);
	pLight->SetBeacon(m_pSunLight);

	//sun light defaults
	pLight->SetDiffuse(RGBf(1,1,1));
	pLight->SetAmbient(RGBf(1,1,1));
	pLight->SetSpecular(RGBf(1,1,1));

#else
	vtLight *pLight = new vtLight;
	pLight->SetName2("Main Light");
	m_pSunLight = new vtTransform;
	m_pSunLight->AddChild(pLight);
	m_pSunLight->SetName2("SunLight");
	m_pTop->AddChild(m_pSunLight);
#endif

	VTLOG(" Creating SkyDome\n");
	m_pAtmosphereGroup = new vtGroup;
	m_pAtmosphereGroup->SetName2("Atmosphere Group");
	m_pTop->AddChild(m_pAtmosphereGroup);

	// 'bsc' is the Bright Star Catalog
	vtString bsc = FindFileOnPaths(vtGetDataPath(), "Sky/bsc.data");
	vtString sun = FindFileOnPaths(vtGetDataPath(), "Sky/glow2.png");
	vtString moon = FindFileOnPaths(vtGetDataPath(), "Sky/moon5_256.png");

	VTLOG("  Stars: '%s'\n", (const char *) bsc);
	VTLOG("    Sun: '%s'\n", (const char *) sun);
	VTLOG("   Moon: '%s'\n", (const char *) moon);

	// create a day-night dome
	m_pSkyDome = new vtSkyDome;
	m_pSkyDome->Create(bsc, 3, 1.0f, sun, moon);	// initially unit radius
	m_pSkyDome->SetDayColors(horizon_color, azimuth_color);
	m_pSkyDome->SetName2("The Sky");
	m_pSkyDome->SetSunLight(GetSunLight());
	m_pAtmosphereGroup->AddChild(m_pSkyDome);

	m_pSkyTrack = new vtSkyTrackEngine;
	m_pSkyTrack->SetName2("Sky-Camera-Following");
	m_pSkyTrack->m_pCamera = vtGetScene()->GetCamera();
	m_pSkyTrack->SetTarget(m_pSkyDome);
	vtGetScene()->AddEngine(m_pSkyTrack);
}

/**
 * Find a terrain whose name begins with a given string.
 */
vtTerrain *vtTerrainScene::FindTerrainByName(const char *name)
{
	VTLOG("FindTerrainByName (%s):\n", name);
	for (unsigned int i = 0; i < NumTerrains(); i++)
	{
		vtString tname = m_Terrains[i]->GetName();
		//VTLOG(" %d: %s\n", i, (const char *) tname);
		if (tname == name)
		{
			VTLOG("  found terrain number %d.\n", i);
			return m_Terrains[i];
		}
	}
	VTLOG1("  not found.\n");
	return NULL;
}

void vtTerrainScene::_CreateEngines()
{
	// Set Time in motion
	m_pTimeEngine = new vtTimeEngine;
	m_pTimeEngine->SetTarget((vtTimeTarget *)this);
	m_pTimeEngine->SetName2("Terrain Time");
	m_pTimeEngine->SetEnabled(false);
	vtGetScene()->AddEngine(m_pTimeEngine);
}


/**
 * Call this method once before adding any terrains, to initialize
 * the vtTerrainScene object.
 */
vtGroup *vtTerrainScene::BeginTerrainScene()
{
	VTLOG("BeginTerrainScene:\n");
	_CreateEngines();

#if VTLIB_OPENSG==1
	m_pTop = new vtLight;
#else
	m_pTop = new vtGroup;
#endif

	m_pTop->SetName2("All Terrain");

	// create sky group - this holds all celestial objects
	_CreateSky();

	return m_pTop;
}

/**
 * Adds a terrain to the scene.
 */
void vtTerrainScene::AppendTerrain(vtTerrain *pTerrain)
{
	m_Terrains.push_back(pTerrain);
}


/**
 * BuildTerrain constructs all geometry, textures and objects for a given terrain.
 *
 * \param pTerrain	The terrain to build.
 * \returns			A vtGroup which is the top of the terrain's scene graph.
 */
vtGroup *vtTerrainScene::BuildTerrain(vtTerrain *pTerrain)
{
	pTerrain->CreateStep0();

	if (!pTerrain->CreateStep1())
		return NULL;

	// Set time to that of the new terrain
	m_pSkyDome->SetTime(pTerrain->GetInitialTime());

	// Tell the skydome where on the planet we are
	DPoint2 geo = pTerrain->GetCenterGeoLocation();
	m_pSkyDome->SetGeoLocation(geo);

	if (!pTerrain->CreateStep2(m_pSunLight))
		return NULL;

	if (!pTerrain->CreateStep3())
		return NULL;

	if (!pTerrain->CreateStep4())
		return NULL;

	if (!pTerrain->CreateStep5())
		return NULL;

	return pTerrain->GetTopGroup();
}

void vtTerrainScene::RemoveTerrain(vtTerrain *pTerrain)
{
	for (unsigned int i = 0; i < NumTerrains(); i++)
	{
		if (pTerrain != GetTerrain(i))
			continue;

		// remove from scene graph
		vtGroup *group = pTerrain->GetTopGroup();
		m_pTop->RemoveChild(group);

		m_Terrains.erase(m_Terrains.begin()+i);
		return;
	}
}

/**
 * Set the current Terrain for the scene.  There can only be one terrain
 * active a at time.  If you have more than one terrain, you can use this
 * method to switch between them.
 */
void vtTerrainScene::SetCurrentTerrain(vtTerrain *pTerrain)
{
	if (m_pCurrentTerrain != NULL)
	{
		// turn off the scene graph of the previous terrain
		m_pCurrentTerrain->Enable(false);

		// turn off the engines specific to the previous terrain
		m_pCurrentTerrain->ActivateEngines(false);
	}
	m_pCurrentTerrain = pTerrain;

	// if setting to no terrain, then nothing more to do
	if (!pTerrain)
	{
		if (m_pSkyDome)
		{
			m_pSkyDome->SetTexture(NULL);
			m_pSkyDome->SetEnabled(false);
		}
		if (m_pTimeEngine)
			m_pTimeEngine->SetEnabled(false);
		return;
	}

	// switch: add the terrain's node to the scene graph
	vtGroup *pTerrainGroup = m_pCurrentTerrain->GetTopGroup();
	if (!m_pTop->ContainsChild(pTerrainGroup))
		m_pTop->AddChild(pTerrainGroup);

	// make the new terrain visible
	m_pCurrentTerrain->Enable(true);

	TParams &param = m_pCurrentTerrain->GetParams();

	// switch to the projection of this terrain
	m_pCurrentTerrain->SetGlobalProjection();

	// Set background color to match the ocean
	vtGetScene()->SetBgColor(m_pCurrentTerrain->GetBgColor());

	// Turn on the engines specific to the new terrain
	m_pCurrentTerrain->ActivateEngines(true);

	// Setup the time engine for the new terrain
	vtTime localtime = pTerrain->GetInitialTime();

	// handle the atmosphere
	UpdateSkydomeForTerrain(pTerrain);

	m_pCurrentTerrain->SetFog(param.GetValueBool(STR_FOG));

	// Update the time engine, which also calls us back to update the skydome
	m_pTimeEngine->SetTime(localtime);
	if (param.GetValueBool(STR_TIMEON))
		m_pTimeEngine->SetSpeed(param.GetValueFloat(STR_TIMESPEED));
	else
		m_pTimeEngine->SetSpeed(0.0f);
	m_pTimeEngine->SetEnabled(true);

	if (param.GetValueBool(STR_STRUCT_SHADOWS))
	{
		int iRez = param.GetValueInt(STR_SHADOW_REZ);
		// Experimental OSG-specific code!
#if VTLIB_OSG
		// Set up cull callback on the dynamic geometry transform node
		vtLodGrid *pStructures = m_pCurrentTerrain->GetStructureGrid();
		vtNode *pShadowed = m_pCurrentTerrain->GetTopGroup()->GetChild(0);
		if (NULL != pShadowed)
		{
			bool bPaging = (pTerrain->GetStructureLodGrid() != NULL);

			LocaleWrap normal_numbers(LC_NUMERIC, "C");
			float fDarkness;
			if (!param.GetValueFloat(STR_SHADOW_DARKNESS, fDarkness))
				fDarkness = 0.8f;
			int iTextureUnit = m_pCurrentTerrain->GetShadowTextureUnit();

			FSphere shadow_area;
			if (bPaging)
				shadow_area.Set(FPoint3(0,0,0),-1);
			else
				pStructures->GetBoundSphere(shadow_area, true);

			vtGetScene()->SetShadowedNode(m_pSunLight, pStructures, pShadowed,
				iRez, fDarkness, iTextureUnit, shadow_area);

			// Update the time engine once again, forcing it to move the sun
			//  to set the correct sunlight direction
			m_pTimeEngine->SetTime(localtime);
		}
#endif
	}
}

void vtTerrainScene::UpdateSkydomeForTerrain(vtTerrain *pTerrain)
{
	// safety check
	if (!m_pSkyDome)
		return;

	TParams &param = pTerrain->GetParams();

	// move the sky to fit the new current terrain
	// use 5x larger than terrain's maximum dimension
	vtHeightField3d *hf = pTerrain->GetHeightField();
	FRECT world_ext = hf->m_WorldExtents;
	float radius;
	float width = world_ext.Width();
	float depth = world_ext.Height();
	float minheight, maxheight;
	hf->GetHeightExtents(minheight, maxheight);

	radius = width;
	if (radius < depth)
		radius = depth;
	if (radius < maxheight)
		radius = maxheight;

	radius *= 5;
	float max_radius = 450000;
	if (radius > max_radius)
		radius = max_radius;
	m_pSkyDome->Identity();
	m_pSkyDome->Scale3(radius, radius, radius);

	// Tell the skydome where on the planet we are
	DPoint2 geo = pTerrain->GetCenterGeoLocation();
	m_pSkyDome->SetGeoLocation(geo);

	// Does this terrain want to show the skydome?
	bool bDoSky = param.GetValueBool(STR_SKY);

	m_pSkyDome->SetEnabled(bDoSky);
	if (bDoSky)
	{
		vtString fname = param.GetValueString(STR_SKYTEXTURE);
		if (fname == "")
		{
			m_pSkyDome->SetTexture(NULL);
		}
		else
		{
			vtString filename = "Sky/";
			filename += fname;
			vtString skytex = FindFileOnPaths(vtGetDataPath(), filename);
			if (skytex != "")
				m_pSkyDome->SetTexture(skytex);
		}
	}
}

void vtTerrainScene::SetTime(const vtTime &time)
{
	if (m_pSkyDome)
	{
		// TODO? Convert to local time?
		m_pSkyDome->SetTime(time);
		// TODO? Update the fog color to match the color of the horizon.

#if VTLIB_OSG
		// Experimental OSG-specific code!
		vtGetScene()->UpdateShadowLightDirection(m_pSunLight);
#endif
	}
}

vtUtilStruct *vtTerrainScene::LoadUtilStructure(const vtString &name)
{
	VTLOG("LoadUtilStructure '%s'\n", (const char *)name);

	// Check to see if it's already loaded
	unsigned int i;
	for (i = 0; i < m_StructObjs.GetSize(); i++)
	{
		if (m_StructObjs[i]->m_sStructName == name)
		{
			VTLOG("  already loaded.\n");
			return m_StructObjs[i];
		}
	}

	// If not, look for it in the global content manager
	vtItem *item = m_Content.FindItemByName(name);
	VTLOG(item != NULL ? " Item Found.\n" : "Item not found.\n");
	vtNode *node = m_Content.CreateNodeFromItemname(name);
	VTLOG(node != NULL ? " Node Loaded.\n" : "Node not Loaded.\n");

	if (node == NULL)
		return NULL;

	// create new util structure
	vtUtilStruct *stnew = new vtUtilStruct;
	stnew->m_pTower = node;
	stnew->m_pTower->SetName2(name);
	stnew->m_sStructName = name;

	// Avoid trouble with '.' and ',' in Europe
	LocaleWrap normal_numbers(LC_NUMERIC, "C");

	// get wire info
	for (i = 0; i < item->NumTags(); i++)
	{
		vtTag *tag = item->GetTag(i);
		if (tag->name == "wire_info")
		{
			// parse wire locations
			FPoint3 p1, p2;
			sscanf(tag->value, "%f, %f, %f, %f, %f, %f",
				&p1.x, &p1.y, &p1.z, &p2.x, &p2.y, &p2.z);
			stnew->m_fpWireAtt1.Append(p1);
			stnew->m_fpWireAtt2.Append(p2);
			stnew->m_iNumWires ++;
		}
	}

	m_StructObjs.Append(stnew);
	return stnew;
}


//
// Global helper function
//
vtTerrainScene *vtGetTS()
{
	return vtTerrainScene::s_pTerrainScene;
}

vtContentManager3d &vtGetContent()
{
	return vtTerrainScene::s_pTerrainScene->m_Content;
}

