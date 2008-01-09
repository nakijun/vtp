//
// Hawai'i.cpp
//
// Terrain implementation specific to the Big Island of Hawai'i.
//
// Copyright (c) 2001-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#include "vtlib/vtlib.h"
#include "vtlib/core/Content3d.h"
#include "vtdata/Features.h"
#include "vtdata/vtLog.h"
#include "vtdata/FilePath.h"

#include "Hawaii.h"
#include "Engines.h"	// for PlaneEngine
#include "wx/EnviroGUI.h"

///////////////////////////////
bool g_bLineOfSightTest = false;

IslandTerrain::IslandTerrain()
{
	m_pSA = NULL;
	m_pTelescopes = NULL;

#if 0
	// Points of Interest
	// bottom left (x,y) then top right (x,y)
	AddPointOfInterest(283376, 2181205, 287025, 2182614, "Hilo Airport",
		"http://www.hawaii.gov/dot/hilo.htm");
	AddPointOfInterest(237000, 2188779, 247000, 2202455, "Mauna Kea",
		"/Html/maunakea.htm");
	AddPointOfInterest(223000, 2150000, 233000, 2159000, "Mauna Loa",
		"http://www.yahoo.com");
	AddPointOfInterest(190000, 2153953, 194500, 2157500, "Kealakekua bay",
		"/Html/kealakekua.htm");
	AddPointOfInterest(253500, 2080000, 282500, 2103500, "Lo`ihi",
		"/Html/loihi_seamount.htm");
#endif
}

void IslandTerrain::PaintDib(bool progress_callback(int))
{
	if (g_bLineOfSightTest)
	{
		// test Line Of Sight feature on texture recalculation
		vtHeightFieldGrid3d *pGrid = GetHeightFieldGrid3d();

		FPoint3 campos = vtGetScene()->GetCamera()->GetTrans();

		int width = m_pImage->GetWidth();
		int height = m_pImage->GetHeight();

		FPoint3 tpos;
		int i, j;
		for (i = 0; i < width; i++)
		{
			for (j = 0; j < height; j++)
			{
				pGrid->GetWorldLocation(i, height-1-j, tpos);
				tpos.y += 1;
				if (pGrid->LineOfSight(campos, tpos))
					m_pImage->SetPixel24(i, j, RGBi(255,128,128));
				else
					m_pImage->SetPixel24(i, j, RGBi(128,128,255));
			}
		}
	}
	else
		vtTerrain::PaintDib(progress_callback);
}

class SpinEngine: public vtEngine
{
	void Eval() {
		((vtTransform *) GetTarget())->RotateLocal(FPoint3(0,0,1),0.05);
	}
};

#if VTLIB_OSG && 0
// Test particle effects
class psGeodeTransform : public osg::MatrixTransform
{
public:
   class psGeodeTransformCallback : public osg::NodeCallback
   {
	  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
	  {
		 if ( psGeodeTransform* geodetrans = dynamic_cast<psGeodeTransform*>( node ) )
		 {
			osg::NodePath fullNodePath = nv->getNodePath();
			fullNodePath.pop_back();

			osg::Matrix localCoordMat = osg::computeLocalToWorld( fullNodePath );
			osg::Matrix inverseOfAccum = osg::Matrix::inverse( localCoordMat );

			geodetrans->setMatrix( inverseOfAccum );
		 }
		 traverse(node, nv);
	  }
   };
   psGeodeTransform() {setUpdateCallback( new psGeodeTransformCallback() );}

};
class findGeodeVisitor : public osg::NodeVisitor
{
public:
   findGeodeVisitor() : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
   {
	  foundGeode = NULL;
   }
   virtual void apply(osg::Node &searchNode)
   {
	  if (osg::Geode* g = dynamic_cast<osg::Geode*> (&searchNode) )
		 foundGeode = g;
	  else
		 traverse(searchNode);
   }
   osg::Geode* getGeode() {return foundGeode;}
protected:
   osg::Geode* foundGeode;
};
class particleSystemHelper : public osg::Group
{
public:
   particleSystemHelper(osg::Group* psGroup) : osg::Group(*psGroup)
   {
	  findGeodeVisitor* fg = new findGeodeVisitor();
	  accept(*fg);
	  osg::Geode* psGeode = fg->getGeode();
	  psGeodeXForm = new psGeodeTransform();
	  psGeodeXForm->addChild (psGeode);
	  replaceChild(psGeode,psGeodeXForm);
   }
   void addEffect(osg::Group* psGroup)
   {
	  this->addChild(psGroup);
	  findGeodeVisitor* fg = new findGeodeVisitor();
	  psGroup->accept(*fg);
	  osg::Geode* psGeode = fg->getGeode();
	  psGeodeXForm->addChild(psGeode);
	  psGroup->removeChild( psGroup->getChildIndex(psGeode) );
   }
protected:
   psGeodeTransform* psGeodeXForm;
};
#include <osgDB/ReadFile>
#endif

void IslandTerrain::CreateCustomCulture()
{
#if VTLIB_OSG && 0
	osg::Group *cessna = (osg::Group*) osgDB::readNodeFile("C:/Dev/OSGParticleDirt/cessnafire.osg");
	if (cessna)
	{
		//particleSystemHelper* psh = new particleSystemHelper(cessna);
		//vtNativeNode *native = new vtNativeNode(psh);

		vtNativeNode *native = new vtNativeNode(cessna);

		AddNode(native);
	}
#endif

	// Enable this to test Line Of Sight feature on texture recalculation
//	g_bLineOfSightTest = true;

	// create a container for the Hawai'i-specific structures
	m_pSA = NewStructureLayer();
	m_pSA->SetFilename("Hawai'i Dynamic Structures");

	do_test_code();

	// import the lighthouses
	vtTransform *lighthouse1 = LoadModel("BuildingModels/mahukonalthse.dsm");
	if (lighthouse1)
	{
		// scale was one unit = 1 m
		// plant it on the ground
		PlantModelAtPoint(lighthouse1, DPoint2(197389, 2230283));

		AddNodeToStructGrid(lighthouse1);
	}

	create_building_manually();

	// TODO: replace these with a .vtst in the .ini
	create_airports();

#if 0
	// Here is an example of how to load structures from a VTST file:
	if (PointIsInTerrain(DPoint2(240749, 2194370))) // if area includes top of Mauna Kea
	{
		vtString path = FindFileOnPaths(s_DataPaths, "BuildingData/MaunaKea.vtst");
		if (path != "")
			CreateStructuresFromXML(path);
	}
#endif

	create_state_park();

	DPoint2 mauna_loa(227611, 2155222);
	if (PointIsInTerrain(mauna_loa)) // if area includes top of Mauna Loa
	{
		vtGeom *thebox = make_red_cube();
		vtGeom *thecone = make_test_cone();
		vtTransform *container = new vtTransform;
		container->SetName2("Test Shape");
		container->AddChild(thebox);
		container->AddChild(thecone);
		AddNode(container);
		PlantModelAtPoint(container, mauna_loa);
	}

	if (m_Params.GetValueBool(STR_VEHICLES))
	{
		float speed = m_Params.GetValueFloat(STR_VEHICLESPEED);
		create_airplanes(speed);
	}

#if 0
	vtNode *blade = vtNode::LoadModel("G:/Data-Distro/Culture/picnictable.ive");
	vtTransform *trans = new vtTransform;
	trans->AddChild(blade);
	SpinEngine *eng = new SpinEngine;
	eng->AddTarget(trans);
	vtGetScene()->AddEngine(eng);
	vtGetScene()->GetRoot()->AddChild(trans);
#endif
}


void IslandTerrain::create_state_park()
{
	DPoint2 park_location(234900, 2185840);
	if (!PointIsInTerrain(park_location)) // if area includes saddle
		return;

	// Here is an example of how to load a model directly and plant it
	//	on the terrain.  Because it is not part of a vtStructure, the
	//	user won't be able to select and operate on it.
	vtTransform *table = LoadModel("Culture/picnictable.ive");
	if (table)
	{
		// model is at .1 inch per unit
		float scale = .1f * 2.54f / 100;
		scale *= 10;	// Exaggerate its size to make it easier to find
		table->Scale3(scale, scale, scale);
		PlantModelAtPoint(table, park_location);
//		AddNodeToStructGrid(table);
		AddNode(table);
		table->SetName2("Placement for Picnic Table");

#if 0
		// TEMP TEST CODE
		FPoint3 center = table->GetTrans();
		float radius = 20.0f;

		FPoint3 scale1;
		scale1.Set(scale, scale, scale);

		vtAnimPath *path = new vtAnimPath;
		path->SetLoop(true);

#if 0
		float looptime = 10.0f;
		int numSamples = 40;
		float yaw = 0.0f;
		float yaw_delta = 2.0f*osg::PI/((float)numSamples-1.0f);
		float roll = osg::inDegrees(30.0f);

		double time=0.0f;
		double time_delta = looptime/(double)numSamples;
		for(int i=0;i<numSamples;++i)
		{
			FPoint3 position(center + FPoint3(sinf(yaw)*radius, cosf(yaw)*radius,0.0f));
			FQuat rotation(FQuat(FPoint3(0.0,1.0,0.0), roll) * FQuat(FPoint3(0.0,0.0,1.0), -(yaw+osg::inDegrees(90.0f))));

			path->Insert(time,ControlPoint(position,rotation,scale1));

			yaw += yaw_delta;
			time += time_delta;
		}
#else
		FQuat down(FPoint3(1,0,0), PID2f);

		path->Insert(0, ControlPoint(FPoint3(0,  4000,  0), down));
		path->Insert(2, ControlPoint(FPoint3(40000, 4000,  0), down));
		path->Insert(4, ControlPoint(FPoint3(40000, 4000, -40000), down));
		path->Insert(6, ControlPoint(FPoint3(0,  4000, -40000), down));
		path->Insert(8, ControlPoint(FPoint3(0,  4000, 0), down));
		path->ProcessPoints();
		path->SetInterpMode(vtAnimPath::CUBIC_SPLINE);
#endif

		vtAnimPathEngine *engine = new vtAnimPathEngine(path, 1.0);
		engine->SetTarget(table);
		AddEngine(engine);
#endif
	}

	// An example of how to add the content definitions from a content
	//	file (vtco) to the global content manager.
	vtContentManager3d &con = vtGetContent();
	try
	{
		con.ReadXML("../Data/kai.vtco");
	}
	catch (xh_io_exception &e)
	{
		string str = e.getFormattedMessage();
		VTLOG("Error: '%s'\n", str.c_str());
	}

	// Here is an example of how to create a structure instance which
	//	references a content item.  It is planted automatically at the
	//	desired location on the terrain.
	int index = m_pSA->GetSize();
	vtStructInstance *inst = m_pSA->AddNewInstance();
	inst->SetValueString("itemname", "Riesenbuehl", true);
	inst->SetPoint(park_location);
	CreateStructure(m_pSA, index);

#if 0
	// Here is an example of how to directly create a content item and
	//	plant it manually on the terrain.
	vtNode *node = s_Content.CreateNodeFromItemname("Riesenbuehl");
	if (node)
	{
		// plant it
		vtTransform *xform = new vtTransform;
		xform->AddChild(node);
		PlantModelAtPoint(xform, park_location);
		m_pLodGrid->AppendToGrid(xform);
	}
#endif
}

vtGeom *IslandTerrain::make_test_cone()
{
	vtMaterialArray *looks = new vtMaterialArray;
	looks->AddRGBMaterial1(RGBf(1.0f, 0.5f, 0.0f), false);	// orange

	////////////
	int res = 40;
	vtMesh *pMesh = new vtMesh(vtMesh::TRIANGLE_STRIP, VT_Normals, res*res);

	FPoint3 tip(0, 150, 0);
	double cone_radius = PId/4;
	double theta1 = PId * 1.3;
	double theta2 = PId * 1.8;
	double r1 = 70;
	double r2 = 150;

	pMesh->CreateConicalSurface(tip, cone_radius, theta1, theta2, r1, r2, res);

	vtGeom *pGeom = new vtGeom;
	pGeom->SetMaterials(looks);
	looks->Release();	// pass ownership
	pGeom->AddMesh(pMesh, 0);
	pMesh->Release();	// pass ownership

	return pGeom;
}

vtGeom *IslandTerrain::make_red_cube()
{
	vtGeom *thebox = new vtGeom;
	float ws = 100.0f;	// meters

	//code to make it a Shape
	vtMesh *mesh = new vtMesh(vtMesh::TRIANGLE_FAN, VT_Normals, 24);
	FPoint3 size(ws, ws, ws);
	FPoint3 half = size / 2;	// Block() will double the size
	mesh->CreateBlock(half);

	vtMaterialArray *looks = new vtMaterialArray;
	looks->AddRGBMaterial1(RGBf(1.0f, 0.0f, 0.0f), true);
	thebox->SetMaterials(looks);
	looks->Release();	// pass ownership
	thebox->AddMesh(mesh, 0);
	mesh->Release();	// pass ownership

	return thebox;
}

void IslandTerrain::create_airports()
{
	vtTransform *ITO = LoadModel("Culture/ITO.dsm");
	vtTransform *KOA = LoadModel("Culture/KOA.dsm");
	vtTransform *MUE = LoadModel("Culture/MUE.dsm");

	if (ITO)
	{
		PlantModelAtPoint(ITO, DPoint2(283575, 2181163));
		AddNode(ITO);
	}
	if (KOA)
	{
		PlantModelAtPoint(KOA, DPoint2(180290, 2184990));
		AddNode(KOA);
	}
	if (MUE)
	{
		PlantModelAtPoint(MUE, DPoint2(219990, 2213505));
		AddNode(MUE);
	}
}


void IslandTerrain::create_building_manually()
{
	DPoint2 bound[7], c1;
	bound[0].Set(237257, 2219644);
	bound[1] = bound[0] + DPoint2(0.0, -96.64);
	bound[2] = bound[1] + DPoint2(82.5, 0.0);
	bound[3] = bound[1] + DPoint2(178.2, 0.0);
	bound[4] = bound[3] + DPoint2(0.0, 30.48);
	bound[5] = bound[4] + DPoint2(-178.2 + 37.44f, 0.0);
	bound[6] = bound[5] + DPoint2(0.0, 96.64f - 30.48);
	c1 = bound[2] + DPoint2(0.0, 12.2);

	// only if this house would be on the terrain
	if (!GetHeightField()->ContainsEarthPoint(bound[0]))
		return;

	//  8' =  2.4385 m
	// 12' =  3.6576 m
	// 28' =  8.5344 m
	// 36' = 10.9728 m
	// 40' = 12.1920 m
	// 48' = 14.6304 m
	// test dynamic creation of a complicated building
	vtBuilding *bld = m_pSA->AddNewBuilding();

	DPoint2 c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12;
	c2 = c1 + DPoint2(10.9728, 0.0);
	c3 = c1 + DPoint2(14.6304, 0.0);
	c4 = c1 + DPoint2(14.6304, 12.1920);
	c5 = c1 + DPoint2(10.9728, 12.1920);
	c6 = c1 + DPoint2(0.0, 12.1920);
	c7 = c1 + DPoint2(3.6576, 0.0);
	c8 = c1 + DPoint2(3.6576, 3.6576);
	c9 = c1 + DPoint2(0.0, 3.6576);
	c10 = c1 + DPoint2(10.9728, 3.6576);
	c11 = c1 + DPoint2(10.9728, 8.534);
	c12 = c1 + DPoint2(3.6576, 8.534);
	DLine2 dl;
	vtLevel *pLev;
	vtEdge *pEdge;

	// basement/garage level (0)
	dl.Append(c2);
	dl.Append(c3);
	dl.Append(c4);
	dl.Append(c5);
	DPolygon2 footprint;
	footprint.push_back(dl);
	pLev = bld->CreateLevel(footprint);
	pLev->m_fStoryHeight = 2.4385f;
	pLev->m_iStories = 1;

	pEdge = pLev->GetEdge(0);
	pLev->SetEdgeColor(RGBi(128, 128, 128));
	pEdge->m_Features.clear();
	//int feat1 = pEdge->NumFeatures();
	pEdge->AddFeature(WFC_GAP);

	pEdge = pLev->GetEdge(1);
	pEdge->m_Features.clear();
	pEdge->AddFeature(WFC_WALL, -1);
	pEdge->AddFeature(WFC_WALL, -2, 0, 0.5);
	pEdge->AddFeature(WFC_WALL, -4);
	pEdge->AddFeature(WFC_WALL, -2, 0, 0.5);
	pEdge->AddFeature(WFC_WALL, -1);
	pEdge->m_pMaterial = GetGlobalMaterials()->FindName(BMAT_NAME_WOOD);

	pEdge = pLev->GetEdge(2);
	pEdge->m_Features.clear();
	pEdge->AddFeature(WFC_GAP);

	pEdge = pLev->GetEdge(3);
	pEdge->m_Features.clear();
	pEdge->AddFeature(WFC_WALL, -1, 0, 0.5);
	pEdge->AddFeature(WFC_WALL);
	pEdge->AddFeature(WFC_WALL, -1, 0, 0.5);
	pEdge->m_pMaterial = GetGlobalMaterials()->FindName(BMAT_NAME_CEMENT);

	// main floor level (1)
	dl.Empty();
	dl.Append(c7);
	dl.Append(c3);
	dl.Append(c4);
	dl.Append(c6);
	dl.Append(c9);
	dl.Append(c8);
	DPolygon2 footprint2;
	footprint2.push_back(dl);
	pLev = bld->CreateLevel(footprint2);
	pLev->m_fStoryHeight = 2.4385f;
	pLev->m_iStories = 1;
	pLev->SetEdgeMaterial(BMAT_NAME_WOOD);

	pEdge = pLev->GetEdge(0);
	pEdge->m_Features.clear();
	pEdge->AddFeature(WFC_WINDOW, -8, 0.3f, 1.0f);
	pEdge->AddFeature(WFC_WALL, -8);
	pEdge->AddFeature(WFC_WINDOW, -8, 0.5f, 1.0f);
	pEdge->AddFeature(WFC_WALL, -1);
	pEdge->AddFeature(WFC_DOOR, -3);
	pEdge->AddFeature(WFC_WALL, -8);

	pEdge = pLev->GetEdge(1);
	pEdge->m_Features.clear();
	pEdge->AddFeature(WFC_WINDOW, -8, 0.3f, 1.0f);
	pEdge->AddFeature(WFC_WALL, -4);
	pEdge->AddFeature(WFC_WINDOW, -8, 0.3f, 1.0f);
	pEdge->AddFeature(WFC_WALL, -6);
	pEdge->AddFeature(WFC_WINDOW, -2, 0.4f, 0.8f);
	pEdge->AddFeature(WFC_WALL, -4);
	pEdge->AddFeature(WFC_WINDOW, -4, 0.7f, 1.0f);

	pEdge = pLev->GetEdge(2);
	pEdge->m_Features.clear();
	pEdge->AddFeature(WFC_WALL, -6);
	pEdge->AddFeature(WFC_WINDOW, -4, 0.3f, 1.0f);
	pEdge->AddFeature(WFC_WALL, -2);
	pEdge->AddFeature(WFC_WINDOW, -8, 0.3f, 1.0f);
	pEdge->AddFeature(WFC_WALL, -8);
	pEdge->AddFeature(WFC_WINDOW, -4, 0.3f, 1.0f);
	pEdge->AddFeature(WFC_WALL, -4);
	pEdge->AddFeature(WFC_WINDOW, -4, 0.3f, 1.0f);
	pEdge->AddFeature(WFC_WALL, -8);

	pEdge = pLev->GetEdge(3);
	pEdge->m_Features.clear();
	pEdge->AddFeature(WFC_WALL, -8);
	pEdge->AddFeature(WFC_WINDOW, -4, 0.3f, 1.0f);
	pEdge->AddFeature(WFC_WALL, -4);
	pEdge->AddFeature(WFC_WINDOW, -8, 0.3f, 1.0f);
	pEdge->AddFeature(WFC_WALL, -4);

	pEdge = pLev->GetEdge(4);
	pEdge->m_Features.clear();
	pEdge->AddFeature(WFC_WALL, -4);
	pEdge->AddFeature(WFC_WINDOW, -8, 0.3f, 1.0f);

	pEdge = pLev->GetEdge(5);
	pEdge->m_Features.clear();
	pEdge->AddFeature(WFC_DOOR, -4);
	pEdge->AddFeature(WFC_WALL, -8);

	//////////////////////////////
	// first roof level (2)
	dl.Empty();
	dl.Append(c1);
	dl.Append(c3);
	dl.Append(c4);
	dl.Append(c6);
	DPolygon2 footprint3;
	footprint3.push_back(dl);
	pLev = bld->CreateLevel(footprint3);
	pLev->m_iStories = 1;
	pLev->SetEdgeMaterial(BMAT_NAME_PLAIN);
	pLev->SetEdgeColor(RGBi(90, 75, 75));
	bld->SetRoofType(ROOF_HIP, 14, 2);
	pLev->m_fStoryHeight = 0.9144f;	// 3 ft
	pLev->SetEaveLength(1.0f);

	//////////////////////////////
	// second roof level (3)
	dl.Empty();
	dl.Append(c8);
	dl.Append(c10);
	dl.Append(c11);
	dl.Append(c12);
	DPolygon2 footprint4;
	footprint4.push_back(dl);
	pLev = bld->CreateLevel(footprint4);
	pLev->m_iStories = 1;
	pLev->SetEdgeMaterial(BMAT_NAME_PLAIN);
	pLev->SetEdgeColor(RGBi(220, 220, 220));
	bld->SetRoofType(ROOF_GABLE, 33, 3);
	pLev->m_fStoryHeight = 1.6256f;	// 5 1/3 ft

	/////////////////////////////////////////
	CreateStructures(m_pSA);

	// Test XML writing
	//m_pSA->WriteXML("Ben's House.vtst");
}


///////////////////////////////////////

class MyGeom : public vtDynGeom
{
public:
	// these are overrides for virtual methods
	void DoRender();
	void DoCalcBoundBox(FBox3 &box);
	void DoCull(const vtCamera *pCam) {}
};

void MyGeom::DoRender()
{
	// an example: draw a red-green axis in X and Y
	glColor3f(1, 0, 0);
	glBegin(GL_LINES);
	glVertex3f(-1000,0,0);
	glVertex3f( 1000,0,0);
	glEnd();

	glColor3f(0, 1, 0);
	glBegin(GL_LINES);
	glVertex3f(0,-1000,0);
	glVertex3f(0, 1000,0);
	glEnd();
}

void MyGeom::DoCalcBoundBox(FBox3 &box)
{
	// provide the bounding box of your geometry
	box.min.Set(-1000, -1000, 0);
	box.max.Set(1000, 1000, 0);
}

void IslandTerrain::do_test_code()
{
#if 0
	vtElevationGrid grid;
	grid.LoadFromBT("C:/VTP/TerrainApps/Data/Elevation/crater_0513.bt");
	grid.SetupConversion(1.0);
	FPoint3 p;
	grid.GetWorldLocation(200, 200, p);

	float alt;
	grid.FindAltitudeAtPoint(p, alt);
#endif

#if 0
	float x, y;
	DPoint3 p;
	int meter_height;
	char string[80];
	vtFeatures feat;
	feat.SetEntityType(SHPT_POINTZ);
	feat.AddField("Text", FTString);
	feat.GetAtProjection() = m_proj;
	vtString labels_path = FindFileOnPaths(s_DataPaths, "PointData/places.txt");
	FILE *fp = vtFileOpen(labels_path, "r");
	while( !feof(fp) )
	{
		int ret = fscanf(fp, "%f %f %d %s\n", &x, &y, &meter_height, string);
		if (!ret) break;
		p.x = x;
		p.y = y;
		p.z = meter_height;
		int rec = feat.AddPoint(p);
		feat.SetValue(rec, 0, string);
	}
	fclose(fp);
	feat.SaveToSHP("../Data/PointData/hawai`i.shp");
#endif
}

void IslandTerrain::create_airplanes(float fSpeed)
{
	// make some planes
	for (int i = 0; i < 6; i++)
		create_airplane(i, fSpeed);
}

void IslandTerrain::create_airplane(int i, float fSpeed)
{
	RGBf red(1.0f, 1.0f, 0.0f);
	Vehicle *copy = g_App.m_VehicleManager.CreateVehicle("747", red);
	if (!copy)
		return;

	AddNode(copy);

	// make it faster than real life
	float fSpeedExag = fSpeed;

	// attach engine
	AirportCodes code;
	code = KOA;

	PlaneEngine *pEng = new PlaneEngine(fSpeedExag, code);
	pEng->SetName2("Airplane Engine");
	pEng->SetTarget(copy);
	pEng->SetHoop(i);
	AddEngine(pEng);

#if 0
	if (bDoSound)
	{
		//sound stuff
		vtSound3D* plane = new vtSound3D(m_strDataPath + "Vehicles/Airport-trim1.wav");
		plane->Initialize();
		plane->SetName2("Plane Sound");
		plane->SetModel(1,1,200,200);	//set limit of how far away sound can be heard
		plane->SetTarget(copy);			//set target
		plane->SetMute(true);			//mute the sound until later
		plane->Play(0, 0.0f);			//play the sound (muted)
		AddEngine(plane, pScene);
	}
#endif
}

