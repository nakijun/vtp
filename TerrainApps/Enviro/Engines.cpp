//
// Engines.cpp
//
// Engines used by Enviro
//
// Copyright (c) 2001-2011 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#include "vtlib/vtlib.h"
#include "vtlib/core/Route.h"
#include "vtlib/core/Roads.h"
#include "vtlib/core/Globe.h"
#include "vtdata/vtLog.h"

#include "Engines.h"
#include "Hawaii.h"
#include "Enviro.h"	// for GetCurrentTerrain

//////////////////////////////////////////////////////////////

float utm_points_ito[5][2] = {
	{ 287450, 2182128 },	// come back
	{ 287050, 2182130 },	// begin approach
	{ 286820, 2182140 },	// touchdown point
	{ 286760, 2182194 },	// begin takeoff point
	{ 286400, 2182200 }		// end takeoff point
};

float utm_points_koa[5][2] = {
	{ 181204, 2189667 },	// 0, 600m elev
	{ 180920, 2186879 },	// 1, touchdown
	{ 180801, 2185744 },	// 2, middle of runway
	{ 180662, 2184397 },	// 3, liftoff
	{ 180281, 2180278 }		// 4, 800m elev
};

PlaneEngine::PlaneEngine(float fSpeedExag, AirportCodes code) : vtEngine()
{
	m_fSpeedExag = fSpeedExag;

	// set up some initial points
	float x, y, z;
	DPoint3 utm_points[5];

	for (int i = 0; i < 5; i++)
	{
		if (code == KOA)
		{
			utm_points[i].x = utm_points_koa[i][0];
			utm_points[i].y = utm_points_koa[i][1];
		}
		else
		if (code == ITO)
		{
			utm_points[i].x = utm_points_ito[i][0];
			utm_points[i].y = utm_points_ito[i][1];
		}
		utm_points[i].z = 0.0;
	}

	// 1 mile = 1.60934 km
	// a typical flight speed, 450 mph = 725 kmph
	// 1 kmph = 16.66 meters per minute = .2777777 meters per second
	// 725 kmph = 200 meter per second, slowing to 25 m/s at touchdown
	x = 10000.0f;
	y = 5000.0f;
	z = -130000.0f;

	m_hoop_pos[0].Set(x, y, z);
	m_hoop_speed[0] = 200.0f;
	// done setting initial positions and speed

	// begin approach
	utm_points[0].z = 600.0f;
	g_Conv.ConvertFromEarth(utm_points[0], m_hoop_pos[1]);
	m_hoop_speed[1] = 100.0f;

	// touchdown
	// tarmac is 12-13m above sea level
	// center of plane is 7?m above bottom of landing gear
	double ground_offset1 = 16.8f + 7.0f;
	double ground_offset = 12.5f + 7.0f;

	utm_points[1].z = ground_offset1;
	g_Conv.ConvertFromEarth(utm_points[1], m_hoop_pos[2]);
	m_hoop_speed[2] = 25.0f;

	// speeding up to takeoff point
	utm_points[2].z = ground_offset;
	g_Conv.ConvertFromEarth(utm_points[2], m_hoop_pos[3]);
	m_hoop_speed[3] = 5.0f;

	// takeoff to this point
	utm_points[3].z = ground_offset;
	g_Conv.ConvertFromEarth(utm_points[3], m_hoop_pos[4]);
	m_hoop_speed[4] = 25.0f;

	// point to loop to
	utm_points[4].z = 800.0f;
	g_Conv.ConvertFromEarth(utm_points[4], m_hoop_pos[5]);
	m_hoop_speed[5] = 100.0f;

	// saving last hoop info
	x = 2000.0f;
	y = 5000.0f;
	z = -40000.0f;
	m_hoop_pos[6].Set(x, y, z);
	m_hoop_speed[6] = 200.0f;

	m_hoops = 7;
	m_hoop = 0;

	m_pos = m_hoop_pos[0];

	m_fLastTime = vtGetTime();
}


void PlaneEngine::Eval()
{
	// determine vectors between last hoop, current position, and next hoop
	FPoint3 pos_next = m_hoop_pos[m_hoop+1];
	FPoint3 diff1 = m_pos - m_hoop_pos[m_hoop];
	FPoint3 diff2 = pos_next - m_pos;
	FPoint3 diff3 = pos_next - m_hoop_pos[m_hoop];

	// and their magnitudes
	float mag1 = diff1.Length();
	float mag2 = diff2.Length();
	float mag3 = diff3.Length();

	// simple linear interpolation of speed
	float speed_diff = (m_hoop_speed[m_hoop+1] - m_hoop_speed[m_hoop]);
	float speed = m_hoop_speed[m_hoop] + ((1.0f - (mag2/mag3)) * speed_diff);
	float factor = (speed / mag2);

	// moderate to real time
	float time = vtGetTime();
	float elapsed = time - m_fLastTime;		// time per frame
	m_fLastTime = time;
	factor *= elapsed;

	// potentially faster than real life
	factor *= m_fSpeedExag;

	// scale difference vector by speed to produce direction vector
	diff2 *= factor;

	// have we reached/passed the next hoop?
	if (mag1/mag3 > 1.0f)
	{
		m_hoop++;
		if (m_hoop == m_hoops-1)
			m_hoop = 0;
		m_pos = m_hoop_pos[m_hoop];
		return;
	}

	// determine position next frame
	m_pos += diff2;

	// turn plan to point toward next frame's position
	// Yaw the object to face the point indicated
	vtTransform *pTarget = dynamic_cast<vtTransform *> (GetTarget());
	if (!pTarget) return;

	pTarget->Identity();
	float angle = atan2f(-diff2.z, diff2.x) - PID2f;
	pTarget->RotateLocal(FPoint3(0,1,0), angle);

	// set the plane to next frame's position
	pTarget->SetTrans(m_pos);
}

void PlaneEngine::SetHoop(int i)
{
	m_hoop = i;
	m_pos = m_hoop_pos[i];
}


//////////////////////////////////////////////////////////////

RoadFollowEngine::RoadFollowEngine(NodeGeom *pStartNode)
{
	m_pLastNode = pStartNode;

	PickRoad(NULL);
}

void RoadFollowEngine::PickRoad(LinkGeom *last_link)
{
	int i = 0;
	NodeGeom *node = m_pLastNode;

	if (node->m_iLinks != 1 && last_link != NULL)
	{
		i = node->m_iLinks;

		// pick next available road
		if (last_link->m_iHwy > 0)
		{
			// special logic: follow the highway
			for (i = 0; i < node->m_iLinks; i++)
			{
				if (node->GetLink(i) != last_link && node->GetLink(i)->m_iHwy > 0)
					break;
			}
			// if no highway, do normal logic
		}
		if (i == node->m_iLinks)
		{
			for (i = 0; i < node->m_iLinks; i++)
			{
				if (node->GetLink(i) == last_link)
				{
					i++;
					i %= node->m_iLinks;
					break;
				}
			}
		}
	}
	m_pCurrentRoad = node->GetLink(i);
	if (m_pCurrentRoad)
	{
		m_pCurrentRoadLength = m_pCurrentRoad->Length();
		forwards = (m_pCurrentRoad->GetNode(0) == m_pLastNode);
	}
	fAmount = 0.0f;
}

void RoadFollowEngine::Eval()
{
	vtTransform *pTarget = (vtTransform *) GetTarget();
	if (!pTarget) return;

	if (!m_pCurrentRoad) return;

	float fInc = 25.0f;
	fAmount += fInc;
	float fRightAmount = forwards ? fAmount : (m_pCurrentRoadLength - fAmount);

	FPoint3 pos1 = m_pCurrentRoad->FindPointAlongRoad(fRightAmount);
	FPoint3 pos(pos1.x, pos1.y, pos1.z);
	pTarget->SetTrans(pos);

	if (fAmount + fInc >= m_pCurrentRoadLength)
	{
		if (forwards)
			m_pLastNode = m_pCurrentRoad->GetNode(1);
		else
			m_pLastNode = m_pCurrentRoad->GetNode(0);
		PickRoad(m_pCurrentRoad);
	}
}


//////////////////////////////////////////////////////////////////////

//
// Terrain picking ability
//
TerrainPicker::TerrainPicker() : vtLastMouse()
{
	m_pHeightField = NULL;
	m_bOnTerrain = false;
}

void TerrainPicker::OnMouse(vtMouseEvent &event)
{
	vtLastMouse::OnMouse(event);

	FindGroundPoint();
}

void TerrainPicker::FindGroundPoint()
{
	if (!m_pHeightField) return;

	FPoint3 pos, dir, result;

	vtGetScene()->CameraRay(m_pos, pos, dir);

	// test whether we hit the heightfield
	m_bOnTerrain = m_pHeightField->CastRayToSurface(pos, dir, result);
	if (!m_bOnTerrain)
		return;

	for (unsigned int i = 0; i < NumTargets(); i++)
	{
		vtTransform *pTarget = (vtTransform *) GetTarget(i);
		pTarget->SetTrans(result);
	}

	// save result
	m_GroundPoint = result;

	// Find corresponding earth coordinates
	g_Conv.ConvertToEarth(m_GroundPoint, m_EarthPos);
}

void TerrainPicker::Eval()
{
	// Don't calculate here, since we can get OnMouse events out of synch
	//  with the Paint events that trigger the scene update and engine Eval().
}

bool TerrainPicker::GetCurrentPoint(FPoint3 &p)
{
	if (m_bOnTerrain)
		p = m_GroundPoint;
	return m_bOnTerrain;
}

bool TerrainPicker::GetCurrentEarthPos(DPoint3 &p)
{
	if (m_bOnTerrain)
		p = m_EarthPos;
	return m_bOnTerrain;
}


//
// Globe picking ability
//
GlobePicker::GlobePicker() : vtLastMouse()
{
	m_fRadius = 1.0;
	m_bOnTerrain = false;
	m_pGlobe = NULL;
	m_fTargetScale = 0.01f;
}

void GlobePicker::Eval()
{
	FPoint3 pos, dir;

	vtGetScene()->CameraRay(m_pos, pos, dir);

	// test whether we hit the globe
	FSphere sphere(FPoint3(0.0f, 0.0f, 0.0f), (float)m_fRadius);
	FPoint3 akPoint[2];
	int riQuantity;

	m_bOnTerrain = RaySphereIntersection(pos, dir, sphere, riQuantity, akPoint);
	if (m_bOnTerrain)
	{
		// save result
		m_GroundPoint = akPoint[0];

		// apply global position to target (which is not a child of the globe)
		vtTransform *pTarget = (vtTransform *) GetTarget();
		if (pTarget)
		{
			pTarget->Identity();
			pTarget->SetTrans(m_GroundPoint);
			pTarget->PointTowards(m_GroundPoint * 2.0f);
			pTarget->Scale3(m_fTargetScale, m_fTargetScale, m_fTargetScale);
		}

		if (m_pGlobe)
		{
			// rotate to find position relative to globe's rotation
			vtTransform *xform = m_pGlobe->GetTop();
			FMatrix4 rot;
			xform->GetTransform1(rot);
			FMatrix4 inverse;
			inverse.Invert(rot);
			FPoint3 newpoint;

			// work around SML bug: matrices flagged as identity but
			// will still transform by their components
			if (! inverse.IsIdentity())
			{
				inverse.Transform(m_GroundPoint, newpoint);
				m_GroundPoint = newpoint;
			}
		}

		// Find corresponding geographic coordinates
		xyz_to_geo(m_fRadius, m_GroundPoint, m_EarthPos);
	}
}

bool GlobePicker::GetCurrentPoint(FPoint3 &p)
{
	if (m_bOnTerrain)
		p = m_GroundPoint;
	return m_bOnTerrain;
}

bool GlobePicker::GetCurrentEarthPos(DPoint3 &p)
{
	if (m_bOnTerrain)
		p = m_EarthPos;
	return m_bOnTerrain;
}

//////////////////////////////////////////////////////////////////////

FlatFlyer::FlatFlyer()
{
	m_bDrag = false;
	m_bZoom = false;
}

void FlatFlyer::OnMouse(vtMouseEvent &event)
{
	int previous = m_buttons;

	vtLastMouse::OnMouse(event);

	//  Left button down
	bool bLeft = (m_buttons & VT_LEFT) != 0;
	bool bRight = (m_buttons & VT_RIGHT) != 0;

	if (!bLeft && !bRight)
	{
		m_bDrag = false;
		m_bZoom = false;
	}

	vtTransform *pTarget = (vtTransform*) GetTarget();
	if (!pTarget)
		return;

	if (bLeft || bRight)
	{
		if (m_buttons != previous)
		{
			// Previously a different mouse button was down, so capture
			//  the starting state.
			m_start_wp = pTarget->GetTrans();
			m_startpos = m_pos;
		}
		if (!bLeft && bRight)
		{
			m_bDrag = true;
			m_bZoom = false;
		}
		if (bLeft && bRight)
		{
			m_bDrag = false;
			m_bZoom = true;
		}
	}
}

void FlatFlyer::Eval()
{
	vtTransform *pTarget = (vtTransform*) GetTarget();
	if (!pTarget)
		return;

	FPoint3 pos = m_start_wp;
	if (m_bDrag)
	{
		// Consider XZ plane of ray from initial eye to ground
		float dx = -(m_pos.x - m_startpos.x) * 0.0015f;
		float dy = (m_pos.y - m_startpos.y) * 0.0015f;

		pos.x += (dx * (pos.z-0.8f));
		pos.y += (dy * (pos.z-0.8f));
		pTarget->SetTrans(pos);
	}
	if (m_bZoom)
	{
		float dz = -(m_pos.y - m_startpos.y) * 0.02f;
		pos.z += dz;

		pTarget->SetTrans(pos);
	}
}

//////////////////////////////////////////////////////////////////////


GrabFlyer::GrabFlyer(float fSpeed) : vtTerrainFlyer(fSpeed)
{
	m_bDrag = false;
	m_bPivot = false;
	m_bZoom = false;
	m_pTP = NULL;
}

void GrabFlyer::OnMouse(vtMouseEvent &event)
{
	if (!m_pTP)
		return;

	int previous = m_buttons;

	vtLastMouse::OnMouse(event);

	//  Left button down
	bool bLeft = (m_buttons & VT_LEFT) != 0;
	bool bRight = (m_buttons & VT_RIGHT) != 0;

	if (!bLeft && !bRight)
	{
		m_bDrag = false;
		m_bPivot = false;
		m_bZoom = false;
	}

	FPoint3 wp;
	bool bOnGround = m_pTP->GetCurrentPoint(wp);
	if (!bOnGround)
		return;

	vtTransform *pTarget = (vtTransform*) GetTarget();
	if (!pTarget)
		return;

	if (bLeft || bRight)
	{
		if (m_buttons != previous)
		{
			// Previously a different mouse button was down, so capture
			//  the starting state.
			m_start_wp = wp;
			m_startpos = m_pos;
			pTarget->GetTransform1(m_start_eye);
		}
		if (bLeft && !bRight)
		{
			m_bPivot = true;
		}
		if (!bLeft && bRight)
		{
			m_bDrag = true;
			// store initial elevation
			FPoint3 pos = pTarget->GetTrans();
			float fAltitude;
			m_pHeightField->FindAltitudeAtPoint(pos, fAltitude);
			m_fHeight = pos.y - fAltitude;
		}
		if (bLeft && bRight)
		{
			m_bZoom = true;
		}
	}
}

void GrabFlyer::Eval()
{
	if (!m_pTP)
		return;

	vtTransform *pTarget = (vtTransform*) GetTarget();
	if (!pTarget)
		return;

	FMatrix4 rot;
	rot.Identity();

	if (m_bPivot)
	{
		pTarget->SetTransform1(m_start_eye);
		FPoint3 pos = pTarget->GetTrans();
		FPoint3 ray = pos - m_start_wp;
		FPoint3 ray2;

		double radians1 = (m_pos.x - m_startpos.x) * 0.006;
		double radians2 = -(m_pos.y - m_startpos.y) * 0.006;

		FPoint3 up(0, 1, 0);
		FPoint3 side = ray.Cross(up);
		side.Normalize();

		rot.AxisAngle(side, radians2);
		rot.Transform(ray, ray2);

		ray = ray2;

		rot.AxisAngle(up, radians1);
		rot.Transform(ray, ray2);

		FPoint3 out;
		out = m_start_wp + ray2;

		float fAltitude;
		bool bOverTerrain = m_pHeightField->FindAltitudeAtPoint(out, fAltitude);
		if (bOverTerrain)
		{
			if (out.y < fAltitude + 10)
				out.y = fAltitude + 10;
		}

		pTarget->SetTrans(out);

		pTarget->RotateParent(side, radians2);
		pTarget->RotateParent(up, radians1);
	}
	if (m_bDrag)
	{
		pTarget->SetTransform1(m_start_eye);
		FPoint3 pos = pTarget->GetTrans();
		FPoint3 ray = m_start_wp - pos;

		ray.Normalize();
		FPoint3 ray1(ray.x, 0.0f, ray.z);
		FPoint3 ray2(-ray.z, 0.0f, ray.x);

		// Consider XZ plane of ray from initial eye to ground
		float dx = -(m_pos.x - m_startpos.x) * 0.02f * m_fHeight;
		float dz = (m_pos.y - m_startpos.y) * 0.02f * m_fHeight;

		pos += (ray1 * dz);
		pos += (ray2 * dx);

		float fAltitude;
		bool bOverTerrain = m_pHeightField->FindAltitudeAtPoint(pos, fAltitude);
		if (bOverTerrain)
			pos.y = fAltitude + m_fHeight;

		pTarget->SetTrans(pos);
	}
	if (m_bZoom)
	{
		pTarget->SetTransform1(m_start_eye);
		FPoint3 pos = pTarget->GetTrans();
		FPoint3 ray = m_start_wp - pos;

		float dz = -(m_pos.y - m_startpos.y) * 0.02f;
		pos += (ray * dz);

		float fAltitude;
		bool bOverTerrain = m_pHeightField->FindAltitudeAtPoint(pos, fAltitude);
		if (bOverTerrain)
		{
			if (pos.y < fAltitude + 10)
				pos.y = fAltitude + 10;
		}

		pTarget->SetTrans(pos);
	}
}


////////////////////////////////////////////////////////////////////////
//

MapOverviewEngine::MapOverviewEngine()
{
	// boolean to avoid double mouse action clicked and released
	m_bDown = true;
	anglePrec = 0.0;
	MapRatio = 0.0;
	ratioMapTerrain = 0.0;
	MapWidth = 256;
	MapMargin = 10;
	m_pOwnedImage = NULL;

	m_pMapGroup = new vtGroup;
	m_pMapGroup->setName("MapOverview group");

	vtGetScene()->GetHUD()->AddChild(m_pMapGroup);
	CreateMapView();
}

MapOverviewEngine::~MapOverviewEngine()
{
}

void MapOverviewEngine::Eval()
{
	RefreshMapView();
}

void MapOverviewEngine::OnMouse(vtMouseEvent &event)
{
	if (!m_pMapGroup->GetEnabled())
		return;

	IPoint2 position = IPoint2(event.pos.x,vtGetScene()->GetWindowSize().y - event.pos.y);
	if( event.button == VT_MIDDLE
		&& m_bDown
		&& position.x < (MapWidth + MapMargin)
		&& position.y < ((float)MapWidth / MapRatio + (float)MapMargin)
		&& position.x > MapMargin
		&& position.y > MapMargin)
	{
		vtCamera *cam = vtGetScene()->GetCamera();
		FPoint3 PreviousPosition = cam->GetTrans();

		FPoint3 NewPos((position.x - MapMargin) / ratioMapTerrain, 0,
			-(position.y - MapMargin) / ratioMapTerrain);
		cam->SetTrans(NewPos);

		// Set camera direction towards previous point
		cam->SetDirection(FPoint3((PreviousPosition.x - NewPos.x), 0,
									PreviousPosition.z - NewPos.z));
	}
	if(event.button == VT_MIDDLE)
		m_bDown = !m_bDown;
}

void MapOverviewEngine::CreateMapView()
{
	// Create the image-sprite
	m_pMapView = new vtImageSprite;

	CreateArrow();

	// Set terrain-related aspects
	SetTerrain(GetCurrentTerrain());
	m_pMapGroup->addChild(m_pMapView->GetGeode());
}

void MapOverviewEngine::SetTerrain(vtTerrain *pTerr)
{
	vtImage *image;

	// We only support overviews for 'single' or '4x4 tiled' textures
	int depth;
	TextureEnum eTex = pTerr->GetParams().GetTextureEnum();
	if (eTex == TE_SINGLE || eTex == TE_DERIVED)
	{
		image = pTerr->GetTextureImage();
		if (!image)
			return;
		depth = image->GetDepth();
	}
	else if (eTex == TE_TILED)
	{
		vtOverlappedTiledImage	*olap = pTerr->GetOverlappedImage();
		int xsize = olap->GetWidth();
		int ysize = olap->GetHeight();

		depth = olap->GetDepth();
		image = new vtImage;
		image->Create(256, 256, depth);

		RGBi rgb;
		RGBAi rgba;
		for (int i = 0; i < 256; i++)
		{
			for (int j = 0; j < 256; j++)
			{
				if (depth == 24)
				{
					olap->GetPixel24(i * xsize / 256, j * ysize / 256, rgb);
					image->SetPixel24(i, j, rgb);
				}
				else
				{
					olap->GetPixel32(i * xsize / 256, j * ysize / 256, rgba);
					image->SetPixel32(i, j, rgba);
				}
			}
		}
		m_pOwnedImage = image;
	}
	else
		return;		// not supported

	if (m_pMapView->GetGeode())
		m_pMapView->SetImage(image);	// already created
	else
	{
		m_pMapView->Create(image, depth == 32);
		m_pMapView->GetGeode()->setName("Map Overview Image Sprite");
	}

	FPoint2 terrainSize(pTerr->GetHeightField()->m_WorldExtents.Width(),
						pTerr->GetHeightField()->m_WorldExtents.Height());
	MapRatio = fabs(terrainSize.x / terrainSize.y);

	ratioMapTerrain = (float)MapWidth / (float)terrainSize.x;
	m_pMapView->SetPosition((float) MapMargin,
							(float) MapMargin - terrainSize.y * ratioMapTerrain,
							(float) MapMargin + MapWidth,
							(float) MapMargin);
	RefreshMapView();
}

void MapOverviewEngine::CreateArrow()
{
	// Create the "arrow"
	m_pArrow = new vtTransform;
	vtGeode * arrowGeom = new vtGeode;

	vtMaterialArray *pMats = new vtMaterialArray;
	pMats->AddRGBMaterial1(RGBf(1, 0, 0), false, false); // red
	pMats->AddRGBMaterial1(RGBf(0, 0, 0), false, false); // black
	arrowGeom->SetMaterials(pMats);

	int ind[7];
	vtMesh *mesh = new vtMesh(PrimitiveSet::LINES, 0, 7);

	ind[0] = mesh->AddVertex( 0.0, 0.0, 0.0);
	ind[1] = mesh->AddVertex( 0.0, 5.0, 0.0);
	ind[2] = mesh->AddVertex( 0.0,-5.0, 0.0);
	ind[3] = mesh->AddVertex(-4.5,-4.0, 0.0);
	ind[6] = mesh->AddVertex(-4.5, 4.0, 0.0);
	ind[4] = mesh->AddVertex( 5.0, 7.5, 0.0);
	ind[5] = mesh->AddVertex( 5.0,-7.5, 0.0);

	mesh->AddLine(ind[1],ind[2]);
	mesh->AddLine(ind[1],ind[6]);
	mesh->AddLine(ind[2],ind[3]);
	mesh->AddLine(ind[3],ind[6]);
	mesh->AddLine(ind[1],ind[4]);
	mesh->AddLine(ind[2],ind[5]);

	m_pArrow->addChild(arrowGeom);

	// the second argument is the indice of the RGB color added into the material array
	arrowGeom->AddMesh(mesh, 1);

	mesh = new vtMesh(PrimitiveSet::QUADS, 0, 4);

	ind[0] = mesh->AddVertex(-4.3, 3.8, 0.0);
	ind[1] = mesh->AddVertex(-4.3,-3.8, 0.0);
	ind[2] = mesh->AddVertex( 0.0, 4.8, 0.0);
	ind[3] = mesh->AddVertex( 0.0,-4.8, 0.0);

	mesh->AddQuad(ind[2],ind[3],ind[1],ind[0]);

	arrowGeom->AddMesh(mesh,0);

	m_pMapGroup->AddChild(m_pArrow);
}

void MapOverviewEngine::RefreshMapView()
{
	// 3dimension vector is useless ... but needed in SetTrans()
	FPoint3 camPos = vtGetScene()->GetCamera()->GetTrans();

	FPoint2 camDir;
	camDir.x =  vtGetScene()->GetCamera()->GetDirection().x;
	camDir.y =  vtGetScene()->GetCamera()->GetDirection().z;

	//arrow position
	FPoint3 ArrowPos;
	ArrowPos.x = ( camPos.x) * ratioMapTerrain + MapMargin;
	ArrowPos.y = (-camPos.z) * ratioMapTerrain + MapMargin;
	ArrowPos.z = 0.0f;
	m_pArrow->SetTrans(ArrowPos);

	float angle = acosf(camDir.Dot(FPoint2(1,0)));

	if (fabs(anglePrec - angle) > 0.0001)
	{
		camDir.y > 0 ? angle = fabs(angle) : angle = -fabs(angle);
		//arrow orientation
		m_pArrow->RotateLocal(FPoint3(0,0,1),-(angle - anglePrec));
		anglePrec = angle;
	}
}

///////////////////////////////////////////////////////////////////////
// vtSpriteSizer
//   An engine to put sprites in the right place.
//
vtSpriteSizer::vtSpriteSizer(vtImageSprite *pSprite, float l, float t, float r, float b)
{
	m_pSprite = pSprite;
	m_fRotation = 0.0f;
	m_rect.SetRect(l, t, r, b);
	IPoint2 size = vtGetScene()->GetWindowSize();
	OnWindowSize(size.x, size.y);
}

void vtSpriteSizer::OnWindowSize(int width, int height)
{
	m_window_rect = m_rect;
	if (m_window_rect.left < 0) m_window_rect.left += width;
	if (m_window_rect.top < 0) m_window_rect.top += height;
	if (m_window_rect.right < 0) m_window_rect.right += width;
	if (m_window_rect.bottom < 0) m_window_rect.bottom += height;

	m_pSprite->SetPosition(m_window_rect.left, m_window_rect.top, m_window_rect.right, m_window_rect.bottom, m_fRotation);
}

FPoint2 vtSpriteSizer::GetWindowCenter()
{
	FPoint2 center = m_window_rect.Center();
	IPoint2 size = vtGetScene()->GetWindowSize();
	center.y = size.y - center.y;
	return center;
}

