//
// RoadMapEdit.cpp
//
// Copyright (c) 2001-2006 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "vtdata/shapelib/shapefil.h"
#include "vtdata/vtString.h"
#include "vtdata/vtLog.h"
#include "xmlhelper/easyxml.hpp"

#include "RoadMapEdit.h"
#include "assert.h"

#include "ogrsf_frmts.h"

#define BUFFER_SIZE		8000
#define MAX_SEGMENT_LENGTH	80.0						// in meters

void RoadMapEdit::ApplyDLGAttributes(int road_type, int &lanes,
									 SurfaceType &stype, int &priority)
{
	switch (road_type)
	{
	case -201:
	case -202:
		stype = SURFT_RAILROAD;
		lanes = 1;
		priority = 1;
		break;
	case 201:	//	Primary route, class 1, symbol undivided
	case 202:	//	Primary route, class 1, symbol divided by centerline
	case 203:	//	Primary route, class 1, divided, lanes separated
	case 204:	//	Primary route, class 1, one way, other than divided highway
		stype = SURFT_PAVED;
		lanes = 4;
		priority = 1;
		break;
	case 205:	//	Secondary route, class 2, symbol undivided
	case 206:	//	Secondary route, class 2, symbol divided by centerline
	case 207:	//	Secondary route, class 2, symbol divided, lanes separated
	case 208:	//	Secondary route, class 2, one way, other then divided highway
		stype = SURFT_PAVED;
		lanes = 2;
		priority = 2;
		break;
	case 209:	//	Road or street, class 3
	case 217:	//	Road or street, class 3, symbol divided by centerline
	case 218:	//	Road or street, class 3, divided lanes separated
	case 221:	//	Road in street, class 3, one way
	case 222:	//  Road in transition
	case 223:	//  Road in service facility, rest area or viewpoint.
	case 5:		//  cul-de-sac
	case 405:	//  non-standard section of road...???
		stype = SURFT_PAVED;
		lanes = 2;
		priority = 3;
		break;
	case 210:	//	Road or street, class 4
	case 219:	//  Road or street, class 4, 1-way
		stype = SURFT_DIRT;
		lanes = 2;
		priority = 5;
		break;
	case 402:	//	Ramp in interchange
		stype = SURFT_PAVED;
		lanes = 1;
		priority = 4;
		break;
	case 211:	//	Trail, class 5, other than four-wheel drive vehicle
		stype = SURFT_TRAIL;
		lanes = 1;
		priority = 10;
		break;
	case 212:	//	Trail, class 5, four-wheel-drive vehicle
		stype = SURFT_2TRACK;
		lanes = 1;
		priority = 6;
		break;
	case 213:	//	Footbridge
		break;
	}
}

bool RoadMapEdit::attribute_filter_roads(DLGLine *pLine, int &lanes,
										 SurfaceType &stype, int &priority)
{
	// check to see if there is an attribute for road type
	int road_type = 0;
	for (int j = 0; j < pLine->m_iAttribs; j++)
	{
		if (pLine->m_attr[j].m_iMajorAttr == 170 &&
			((pLine->m_attr[j].m_iMinorAttr >= 201 && pLine->m_attr[j].m_iMinorAttr <= 213) ||
			(pLine->m_attr[j].m_iMinorAttr >= 217 && pLine->m_attr[j].m_iMinorAttr <= 222) ||
			(pLine->m_attr[j].m_iMinorAttr >= 401 && pLine->m_attr[j].m_iMinorAttr <= 405))
			)
		{
			road_type = pLine->m_attr[j].m_iMinorAttr;
			break;
		}
		if (pLine->m_attr[j].m_iMajorAttr == 180 &&
			(pLine->m_attr[j].m_iMinorAttr == 201 ||
			 pLine->m_attr[j].m_iMinorAttr == 202)
			) {
			road_type = -pLine->m_attr[j].m_iMinorAttr;
		}
	}
	stype = SURFT_NONE;
	priority = 0;

	ApplyDLGAttributes(road_type, lanes, stype, priority);
	return (stype != SURFT_NONE);
}


void RoadMapEdit::AddElementsFromDLG(vtDLGFile *pDlg)
{
	int i, j, lanes;
	SurfaceType stype;
	DPoint2 buffer[BUFFER_SIZE];

	// set projection
	m_proj = pDlg->GetProjection();

	// expand extents to include the new DLG
	if (!m_bValidExtents)
	{
		// unitialized
		m_extents.right = pDlg->m_NE_utm.x;
		m_extents.top = pDlg->m_NE_utm.y;
		m_extents.left = pDlg->m_SW_utm.x;
		m_extents.bottom = pDlg->m_SW_utm.y;
		m_bValidExtents = true;
	}
	else
	{
		// expand extents
		m_extents.GrowToContainPoint(pDlg->m_SW_utm);
		m_extents.GrowToContainPoint(pDlg->m_NE_utm);
	}

	//an array to allow for fast lookup
	NodeEditPtr *pNodeLookup = new NodeEditPtr[pDlg->m_iNodes+1];
	int id = 1;

	NodeEdit *pN;
	for (i = 0; i < pDlg->m_iNodes; i++)
	{
		DLGNode &dnode = pDlg->m_nodes[i];

		// create new node
		pN = new NodeEdit;
		pN->m_id = id++;
		pN->m_p = dnode.m_p;

		AddNode(pN);

		//add to array
		pNodeLookup[pN->m_id] = pN;
	}
	LinkEdit *pL;
	for (i = 0; i < pDlg->m_iLines; i++)
	{
		DLGLine &dline = pDlg->m_lines[i];

		int priority;
		bool result = false;
		result = attribute_filter_roads(&dline, lanes, stype, priority);
		if (!result)
			continue;

		// create new link
		pL = new LinkEdit;
		pL->m_Surface = stype;
		pL->m_iLanes = lanes;
		pL->m_iPriority = priority;

		// copy data from DLG line
		pL->SetNode(0, pNodeLookup[dline.m_iNode1]);
		pL->SetNode(1, pNodeLookup[dline.m_iNode2]);

		int actual_coords = 0;
		for (j = 0; j < dline.m_iCoords; j++)
		{
			// safety check
			assert(actual_coords < BUFFER_SIZE);
			if (j > 0)
			{
				//
				// check how long this segment is
				//
				DPoint2 delta = dline.m_p[j] - dline.m_p[j-1];
				double length = delta.Length();

				//
				// if it's too long, chop it up into a series of smaller segments,
				// by adding more points
				//
				if (length > MAX_SEGMENT_LENGTH)
				{
					int splits = (int) (length / MAX_SEGMENT_LENGTH);
					double step = 1.0 / (splits+1);
					for (double amount = step; amount <= 0.999; amount += step)
					{
						buffer[actual_coords] = (dline.m_p[j-1] + (delta * amount));
						actual_coords++;
					}
				}
			}
			buffer[actual_coords] = dline.m_p[j];
			actual_coords++;
		}
		pL->SetSize(actual_coords);
		for (j = 0; j < actual_coords; j++)
			pL->SetAt(j, buffer[j]);

		//set bounding box for the link
		pL->ComputeExtent();

		pL->m_iHwy = dline.HighwayNumber();

		// add to list
		AddLink(pL);

		// inform the Nodes to which it belongs
		pL->GetNode(0)->AddLink(pL, true);
		pL->GetNode(1)->AddLink(pL, false);
		pL->m_fLength = pL->Length();
	}

	//delete the lookup array.
	delete [] pNodeLookup;

	GuessIntersectionTypes();
}

//
// Guess and add some intersection behaviors
//
void RoadMapEdit::GuessIntersectionTypes()
{
	int i;
	NodeEdit *pN;

	pN = GetFirstNode();
	LinkEdit* curRoad;
	while (pN)
	{
		if (pN->m_iLinks <= 2)
		{
			pN->SetVisual(VIT_NONE);
			if (pN->m_iLinks > 0)
				assert(pN->SetIntersectType(0,IT_NONE));
			if (pN->m_iLinks == 2)
				assert(pN->SetIntersectType(1,IT_NONE));
		}
		else
		{
			int topPriority = ((LinkEdit*)(pN->GetLink(0)))->m_iPriority;
			int topCount = 0;
			int lowPriority = topPriority;

			//analyze the links intersecting at the node
			for (i = 0; i < pN->m_iLinks; i++)
			{
				curRoad = (LinkEdit*)(pN->GetLink(i));
				if (curRoad->m_iPriority == topPriority) {
					topCount++;
				} else if (curRoad->m_iPriority < topPriority) {
					topCount = 1;
					topPriority = curRoad->m_iPriority;
				} else if (curRoad->m_iPriority > lowPriority) {
					lowPriority = curRoad->m_iPriority;
				}
			}

			IntersectionType bType;
			//all links have same priority
			if (topCount == pN->m_iLinks)
			{
				if (topPriority <= 2) {
					//big roads.  use lights
					pN->SetVisual(VIT_ALLLIGHTS);
					bType = IT_LIGHT;
				} else if (topPriority >= 5) {
					//dirt roads.  uncontrolled
					pN->SetVisual(VIT_NONE);
					bType = IT_NONE;
				} else {
					//smaller roads, use stop signs
					bType = IT_STOPSIGN;
					pN->SetVisual(VIT_ALLSTOPS);
				}
				for (i = 0; i < pN->m_iLinks; i++)
				{
					pN->SetIntersectType(i, bType);
				}
			}
			else
			{
				//we have a mix of priorities
				if (lowPriority <=2)
				{
					//big roads, use lights
					pN->SetVisual(VIT_ALLLIGHTS);
					for (i = 0; i < pN->m_iLinks; i++)
					{
						pN->SetIntersectType(i, IT_LIGHT);
					}
				}
				else
				{
					//top priority links have right of way
					pN->SetVisual(VIT_STOPSIGN);
					for (i = 0; i < pN->m_iLinks; i++)
					{
						curRoad = (LinkEdit*)(pN->GetLink(i));
						if (curRoad->m_iPriority == topPriority) {
							//higher priority link.
							pN->SetIntersectType(i, IT_NONE);
						} else {
							//low priority.
							pN->SetIntersectType(i, IT_STOPSIGN);
						}
					}
				}
			}
		}

		for (i = 0; i< pN->m_iLinks; i++) {
			pN->SetLightStatus(i, LT_INVALID);
		}
		pN->AdjustForLights();

		pN = pN->GetNext();
	}
}

bool RoadMapEdit::ApplyCFCC(LinkEdit *pL, const char *str)
{
	bool bReject = false;

	if (str[0] != 'A')
		return false;
	int code1 = str[1] - '0';
	int code2 = str[2] - '0';
	switch (code1)
	{
	case 1:
		// Primary Highway With Limited Access
		pL->m_iLanes = 4;
		pL->m_iHwy = 1;		// better to have actual highway number
		break;
	case 2:
		// Primary Road Without Limited Access
		pL->m_iLanes = 2;
		pL->m_iHwy = 1;		// better to have actual highway number
		break;
	case 3:
		// Secondary and Connecting Road
		pL->m_iLanes = 2;
		break;
	case 4:
		// Local, Neighborhood, and Rural Road
		pL->m_iLanes = 2;
		break;
	case 5:
		// Vehicular Trail
		pL->m_iLanes = 1;
		pL->m_Surface = SURFT_2TRACK;
		break;
	// Road with Special Characteristics
	case 6:
		if (code2 == 1)
		{
			// cul-de-sac
		}
		if (code2 == 2)
		{
			// traffic circle
		}
		if (code2 == 3)
		{
			// access ramp
			// 1 lane, 1 direction
			pL->m_iLanes = 1;
			pL->m_iFlags &= ~RF_REVERSE;
		}
		if (code2 == 4)
		{
			 // service drive
		}
		if (code2 == 5)
		{
			// ferry crossing
			bReject = true;
		}
		break;
	//
	case 7:
		// Road as Other Thoroughfare
		if (code2 == 1)
		{
			// Walkway or trail for pedestrians, usually unnamed
			pL->m_iLanes = 1;
			pL->m_Surface = SURFT_TRAIL;
		}
		if (code2 == 2)
		{
			// Stairway, stepped road for pedestrians, usually unnamed
			bReject = true;
		}
		if (code2 == 3)
		{
			// Alley, road for service vehicles, usually unnamed, located at
			// the rear of buildings and property
			pL->m_iLanes = 1;
		}
		if (code2 == 4)
		{
			// Driveway or service road, usually privately owned and unnamed,
			// used as access to residences, trailer parks, and apartment
			// complexes, or as access to logging areas, oil rigs, ranches,
			// farms, and park lands
			pL->m_iLanes = 1;
		}
		break;
	}
	return bReject;
}

void RoadMapEdit::AddElementsFromSHP(const wxString &filename, const vtProjection &proj,
									 bool progress_callback(int))
{
	// SHPOpen doesn't yet support utf-8 or wide filenames, so convert
	vtString fname_local = UTF8ToLocal(filename.mb_str(wxConvUTF8));

	SHPHandle hSHP = SHPOpen(fname_local, "rb");
	if (hSHP == NULL)
		return;

	int nEntities, nShapeType;

	SHPGetInfo(hSHP, &nEntities, &nShapeType, NULL, NULL);
	if (nShapeType != SHPT_ARC && nShapeType != SHPT_ARCZ)
		return;

	// Open DBF File, if one exists
	int cfcc = -1;
	DBFHandle db = DBFOpen(fname_local, "rb");
	if (db != NULL)
	{
		int fields, i, *pnWidth = 0, *pnDecimals = 0;
		char pszFieldName[20];
		fields = DBFGetFieldCount(db);
		for (i = 0; i < fields; i++)
		{
			DBFGetFieldInfo(db, i, pszFieldName, pnWidth, pnDecimals );
			if (!strcmp(pszFieldName, "CFCC"))
				cfcc = i;
		}
	}

	// set projection
	m_proj = proj;

	NodeEdit *pN1, *pN2;
	LinkEdit *pL;
	int i, j, npoints;

	for (i = 0; i < nEntities; i++)
	{
		if ((i % 32) == 0)
			progress_callback(i * 100 / nEntities);

		SHPObject *psShape = SHPReadObject(hSHP, i);
		npoints = psShape->nVertices;

		// It's possible for some entities to be empty?
		if (npoints == 0)
		{
			SHPDestroyObject(psShape);
			continue;
		}

		// create 2 new nodes (begin/end) and a new line
		pN1 = new NodeEdit;
		pN1->m_p.x = psShape->padfX[0];
		pN1->m_p.y = psShape->padfY[0];
		pN1->SetVisual(VIT_NONE);

		// add to list
		AddNode(pN1);

		pN2 = new NodeEdit;
		pN2->m_p.x = psShape->padfX[npoints-1];
		pN2->m_p.y = psShape->padfY[npoints-1];
		pN2->SetVisual(VIT_NONE);

		// add to list
		AddNode(pN2);

		// create new link
		pL = new LinkEdit;
		pL->m_iLanes = 2;
		pL->m_iPriority = 1;

		if (cfcc != -1)
		{
			const char *str = DBFReadStringAttribute(db, i, cfcc);
			ApplyCFCC(pL, str);
		}
		// copy point data
		pL->SetNode(0, pN1);
		pL->SetNode(1, pN2);

		pL->SetSize(npoints);
		for (j = 0; j < npoints; j++)
		{
			pL->GetAt(j).x = psShape->padfX[j];
			pL->GetAt(j).y = psShape->padfY[j];
		}

		//set bounding box for the link
		pL->ComputeExtent();

		// add to list
		AddLink(pL);

		// inform the Nodes to which it belongs
		pL->GetNode(0)->AddLink(pL, true);
		pL->GetNode(1)->AddLink(pL, false);
		pL->m_fLength = pL->Length();

		SHPDestroyObject(psShape);
	}
	m_bValidExtents = false;
	SHPClose(hSHP);

	//guess and add some intersection behaviors
	// (chopped)
}

bool RoadMapEdit::extract_road_attributes(const char *strEntity, int &lanes,
										  SurfaceType &stype, int &priority)
{
	int numEntity = atoi(strEntity);
	int iMajorAttr = numEntity / 10000;
	int iMinorAttr = numEntity % 10000;

	// check to see if there is an attribute for road type
	int road_type = 0;
	if (iMajorAttr == 170 &&
		((iMinorAttr >= 201 && iMinorAttr <= 213) ||
		(iMinorAttr >= 217 && iMinorAttr <= 222) ||
		(iMinorAttr >= 401 && iMinorAttr <= 405))
		)
	{
		road_type = iMinorAttr;
	}
	if (iMajorAttr == 180 &&
		(iMinorAttr == 201 || iMinorAttr == 202))
	{
		road_type = -iMinorAttr;
	}

	stype = SURFT_NONE;
	priority = 0;

	ApplyDLGAttributes(road_type, lanes, stype, priority);
	return (stype != SURFT_NONE);
}


void RoadMapEdit::AddElementsFromOGR(OGRDataSource *pDatasource,
									 bool progress_callback(int))
{
	int i, j, feature_count, count;
	OGRLayer		*pLayer;
	OGRFeature		*pFeature;
	OGRGeometry		*pGeom;
	OGRPoint		*pPoint;
	OGRLineString   *pLineString;
	OGRFeatureDefn	*defn;
	const char		*layer_name;

	NodeEdit *pN;
	LinkEdit *pL;
	NodeEditPtr *pNodeLookup = NULL;

	//
	// Check if this data source is a USGS SDTS DLG
	//
	// Iterate through the layers looking for the ones we care about
	//
	bool bIsSDTS = false;
	int num_layers = pDatasource->GetLayerCount();
	for (i = 0; i < num_layers; i++)
	{
		pLayer = pDatasource->GetLayer(i);
		defn = pLayer->GetLayerDefn();
		layer_name = defn->GetName();
		if (!strcmp(layer_name, "NO01"))
			bIsSDTS = true;
	}

	for (i = 0; i < num_layers; i++)
	{
		pLayer = pDatasource->GetLayer(i);
		feature_count = pLayer->GetFeatureCount();
  		pLayer->ResetReading();
		defn = pLayer->GetLayerDefn();
		layer_name = defn->GetName();

		// Nodes (from an SDTS DLG file)
		if (!strcmp(layer_name, "NO01"))
		{
			// Get the projection (SpatialReference) from this layer
			OGRSpatialReference *pSpatialRef = pLayer->GetSpatialRef();
			if (pSpatialRef)
				m_proj.SetSpatialReference(pSpatialRef);

			pNodeLookup = new NodeEditPtr[feature_count+1];

			int id = 1;
			while( (pFeature = pLayer->GetNextFeature()) != NULL )
			{
				// make sure we delete the feature no matter how the loop exits
				std::auto_ptr<OGRFeature> ensure_deletion(pFeature);

				pGeom = pFeature->GetGeometryRef();
				if (!pGeom) continue;
				pPoint = (OGRPoint *) pGeom;
				pN = new NodeEdit;
				pN->m_id = id++;

				pN->m_p.x = pPoint->getX();
				pN->m_p.y = pPoint->getY();

				AddNode(pN);

				//add to array
				pNodeLookup[pN->m_id] = pN;
			}
		}
		// Lines (Arcs, Roads) (from an SDTS DLG file)
		else if (!strcmp(layer_name, "LE01"))
		{
			// get field indices
			int index_snid = defn->GetFieldIndex("SNID");
			int index_enid = defn->GetFieldIndex("ENID");
			int index_entity = defn->GetFieldIndex("ENTITY_LABEL");
			int index_lanes = defn->GetFieldIndex("LANES");
			//int index_class = defn->GetFieldIndex("FUNCTIONAL_CLASS");
			int index_route = defn->GetFieldIndex("ROUTE_NUMBER");
			int index_rtype = defn->GetFieldIndex("ROUTE_TYPE");

			count = 0;
			while( (pFeature = pLayer->GetNextFeature()) != NULL )
			{
				// make sure we delete the feature no matter how the loop exits
				std::auto_ptr<OGRFeature> ensure_deletion(pFeature);

				count++;
				progress_callback(count * 100 / feature_count);

				// Ignore non-entities
				if (!pFeature->IsFieldSet(index_entity))
					continue;

				// The "ENTITY_LABEL" contains the same information as the old
				// DLG classification.  First, try to use this field to guess
				// values such as number of lanes, etc.
				const char *str_entity = pFeature->GetFieldAsString(index_entity);
				int lanes;
				SurfaceType stype;
				int priority;
				bool result = extract_road_attributes(str_entity, lanes, stype, priority);
				if (!result)
					continue;

				pGeom = pFeature->GetGeometryRef();
				if (!pGeom) continue;
				pLineString = (OGRLineString *) pGeom;

				pL = new LinkEdit;
				pL->m_fWidth = 1.0f;	// defaults
				pL->m_Surface = stype;
				pL->m_iLanes = lanes;		// defaults
				pL->m_iPriority = priority;

				if (pFeature->IsFieldSet(index_lanes))
				{
					// If the 'lanes' field is set, it should override the
					// previous guess
					int value_lanes = pFeature->GetFieldAsInteger(index_lanes);
					if (value_lanes > 0)
						pL->m_iLanes = value_lanes;
				}
				if (pFeature->IsFieldSet(index_route))
				{
					const char *str_route = pFeature->GetFieldAsString(index_route);
					// should probably eventually store the route as a string.
					// currently, it is only supported as an integer
					if (!strncmp(str_route, "SR", 2))
					{
						int int_route = atoi(str_route+2);
						pL->m_iHwy = int_route;
					}
				}
				if (pFeature->IsFieldSet(index_rtype))
				{
					//const char *str_rtype = pFeature->GetFieldAsString(index_rtype);
				}

				int num_points = pLineString->getNumPoints();
				pL->SetSize(num_points);
				for (j = 0; j < num_points; j++)
					pL->SetAt(j, DPoint2(pLineString->getX(j),
						pLineString->getY(j)));

#if 0
				// Slow way to guess at road-node connectivity: compare points
				for (pN = GetFirstNode(); pN; pN=pN->GetNext())
				{
					if (pL->GetAt(0) == pN->m_p)
						pL->SetNode(0, pN);
					if (pL->GetAt(num_points-1) == pN->m_p)
						pL->SetNode(1, pN);
				}
#else
				// Much faster: Get start/end information from SDTS via OGR
				int snid = pFeature->GetFieldAsInteger(index_snid);
				int enid = pFeature->GetFieldAsInteger(index_enid);
				pL->SetNode(0, pNodeLookup[snid]);
				pL->SetNode(1, pNodeLookup[enid]);
#endif
				pL->ComputeExtent();

				AddLink(pL);

				// inform the Nodes to which it belongs
				pL->GetNode(0)->AddLink(pL, true);
				pL->GetNode(1)->AddLink(pL, false);
			}
		}
		else if (!bIsSDTS)
		{
			// For OGR import from a file that isn't an SDTS-DLG, import what
			// we can from the first layer, then stop.
			AppendFromOGRLayer(pLayer);
			break;
		}
	}
	if (pNodeLookup)
		delete [] pNodeLookup;
	if (bIsSDTS)
		GuessIntersectionTypes();
}


bool RoadMapEdit::AppendFromOGRLayer(OGRLayer *pLayer)
{
	int i, num_fields, feature_count, count;
	OGRFeature		*pFeature;
	OGRGeometry		*pGeom;
	OGRFeatureDefn	*defn;
	const char		*layer_name;
	OGRwkbGeometryType geom_type;

	// Get basic information about the layer we're reading
	feature_count = pLayer->GetFeatureCount();
  	pLayer->ResetReading();
	defn = pLayer->GetLayerDefn();
	if (!defn)
		return false;

	layer_name = defn->GetName();
	num_fields = defn->GetFieldCount();
	geom_type = defn->GetGeomType();

	// Get the projection (SpatialReference) from this layer, if we can.
	// Sometimes (e.g. for GML) the layer doesn't have it; may have to
	// use the first Geometry instead.
	bool bGotCS = false;
	OGRSpatialReference *pSpatialRef = pLayer->GetSpatialRef();
	if (pSpatialRef)
	{
		m_proj.SetSpatialReference(pSpatialRef);
		bGotCS = true;
	}

	// Convert from OGR to our geometry type
	bool bGood = false;
	while (!bGood)
	{
		switch (geom_type)
		{
		case wkbLineString:
		case wkbMultiLineString:
			bGood = true;
			break;
		case wkbUnknown:
			// This usually indicates that the file contains a mix of different
			// geometry types.  Look at the first geometry.
			pFeature = pLayer->GetNextFeature();
			pGeom = pFeature->GetGeometryRef();
			geom_type = pGeom->getGeometryType();
			delete pFeature;
			break;
		default:
			return false;	// don't know what to do with this geom type
		}
	}

	// Read Data from OGR into memory
	int num_geoms;
	OGRLineString   *pLineString;
	OGRMultiLineString   *pMulti;

	pLayer->ResetReading();
	count = 0;
	while( (pFeature = pLayer->GetNextFeature()) != NULL )
	{
		// make sure we delete the feature no matter how the loop exits
		std::auto_ptr<OGRFeature> ensure_deletion(pFeature);

		pGeom = pFeature->GetGeometryRef();
		if (!pGeom)
			continue;

		if (!bGotCS)
		{
			OGRSpatialReference *pSpatialRef = pGeom->getSpatialReference();
			if (pSpatialRef)
			{
				m_proj.SetSpatialReference(pSpatialRef);
				bGotCS = true;
			}
		}
		// Beware - some OGR-supported formats, such as MapInfo,
		//  will have more than one kind of geometry per layer.
		geom_type = pGeom->getGeometryType();
		num_geoms = 1;

		switch (geom_type)
		{
		case wkbLineString:
			pLineString = (OGRLineString *) pGeom;
			AddLinkFromLineString(pLineString);
			break;
		case wkbMultiLineString:
			pMulti = (OGRMultiLineString *) pGeom;
			num_geoms = pMulti->getNumGeometries();
			for (i = 0; i < num_geoms; i++)
			{
				pLineString = (OGRLineString *) pMulti->getGeometryRef(i);
				AddLinkFromLineString(pLineString);
			}
			break;
		default:
			continue;	// ignore all other geometry types
		}
		// track total features
		feature_count += (num_geoms-1);
	}
	return true;
}


void RoadMapEdit::AddLinkFromLineString(OGRLineString *pLineString)
{
	NodeEdit *pN1, *pN2;
	LinkEdit *pL;
	DPoint2 p2;

	int j, num_points = pLineString->getNumPoints();

	// create new link
	pL = new LinkEdit;
	pL->m_iLanes = 2;
	pL->m_iPriority = 1;

	pL->SetSize(num_points);
	for (j = 0; j < num_points; j++)
	{
		p2.Set(pLineString->getX(j), pLineString->getY(j));
		pL->SetAt(j, p2);
	}

	// create 2 new nodes (begin/end) and a new line
	pN1 = new NodeEdit;
	pN1->m_p.x = pLineString->getX(0);
	pN1->m_p.y = pLineString->getY(0);
	pN1->SetVisual(VIT_NONE);

	// add to list
	AddNode(pN1);

	pN2 = new NodeEdit;
	pN2->m_p.x = pLineString->getX(num_points-1);
	pN2->m_p.y = pLineString->getY(num_points-1);
	pN2->SetVisual(VIT_NONE);

	// add to list
	AddNode(pN2);

	// point link to nodes
	pL->SetNode(0, pN1);
	pL->SetNode(1, pN2);

	//set bounding box for the link
	pL->ComputeExtent();

	// add to list
	AddLink(pL);

	// point node to links
	pL->GetNode(0)->AddLink(pL, true);
	pL->GetNode(1)->AddLink(pL, false);
}


////////////////////////////////////////////////////////////////////////
// Visitor class, for XML parsing of an OpenStreetMap file.
//

struct OSMNode {
	DPoint2 p;
	int id;
	bool signal_lights;
};

class VisitorOSM : public XMLVisitor
{
public:
	VisitorOSM(RoadMapEdit *rm) : m_state(0), m_pMap(rm) {}
	void startElement(const char *name, const XMLAttributes &atts);
	void endElement(const char *name);
	void data(const char *s, int length);
	void SetSignalLights();

private:
	//string m_data;
	int m_state;
	int m_rec;

	std::vector<OSMNode> m_nodes;
	int find_node(int id)
	{
		for (size_t i = 0; i < m_nodes.size(); i++)
			if (m_nodes[i].id == id)
				return i;
		return -1;
	}
	std::vector<int> m_refs;

	RoadMapEdit *m_pMap;
	LinkEdit *m_pLink;
	bool	m_bAddLink;
};

void VisitorOSM::SetSignalLights()
{
	// For all the nodes which have signal lights, set the state
	for (unsigned int i = 0; i < m_nodes.size(); i++)
	{
		OSMNode &node = m_nodes[i];
		if (node.signal_lights)
		{
			TNode *tnode = m_pMap->FindNodeByID(node.id);
			if (tnode)
			{
				for (int j = 0; j < tnode->m_iLinks; j++)
					tnode->SetIntersectType(j, IT_LIGHT);
			}
		}
	}
	m_pMap->GuessIntersectionTypes();
}

void VisitorOSM::startElement(const char *name, const XMLAttributes &atts)
{
	const char *val;

	if (m_state == 0)
	{
		if (!strcmp(name, "node"))
		{
			DPoint2 p;
			int id;

			val = atts.getValue("id");
			if (val)
				id = atoi(val);

			val = atts.getValue("lon");
			if (val)
				p.x = atof(val);

			val = atts.getValue("lat");
			if (val)
				p.y = atof(val);

			//TNode *node = m_pMap->NewNode();
			//node->m_p = p;
			//node->m_id = id;
			//m_pMap->AddNode(node)

			OSMNode node;
			node.p = p;
			node.id = id;
			node.signal_lights = false;
			m_nodes.push_back(node);

			m_state = 1;
		}
		else if (!strcmp(name, "way"))
		{
			m_pLink = m_pMap->NewLink();
			m_pLink->m_iLanes = 2;

			m_refs.clear();
			m_state = 2;
			m_bAddLink = true;
		}
	}
	else if (m_state == 1 && !strcmp(name, "tag"))
	{
		vtString key, value;

		val = atts.getValue("k");
		if (val)
			key = val;

		val = atts.getValue("v");
		if (val)
			value = val;

		// Node key/value
		if (key == "highway")
		{
			if (value == "traffic_signals")	//
			{
				m_nodes[m_nodes.size()-1].signal_lights = true;
			}
		}
	}
	else if (m_state == 2)
	{
		if (!strcmp(name, "nd"))
		{
			val = atts.getValue("ref");
			if (val)
			{
				int ref = atoi(val);
				m_refs.push_back(ref);
			}
		}
		else if (!strcmp(name, "tag"))
		{
			vtString key, value;

			val = atts.getValue("k");
			if (val)
				key = val;

			val = atts.getValue("v");
			if (val)
				value = val;

			// There are hundreds of possible Way tags
			if (key == "natural")	// value is coastline, marsh, etc.
				m_bAddLink = false;

			if (key == "route" && value == "ferry")
				m_bAddLink = false;

			if (key == "highway")
			{
				if (value == "motorway")	// like a freeway
					m_pLink->m_iLanes = 4;
				if (value == "motorway_link")	// on/offramp
					m_pLink->m_iLanes = 1;
				if (value == "unclassified")	// lowest form of the interconnecting grid network.
					m_pLink->m_iLanes = 1;
				if (value == "unsurfaced")
					m_pLink->m_Surface = SURFT_DIRT;
				if (value == "track")
				{
					m_pLink->m_iLanes = 1;
					m_pLink->m_Surface = SURFT_2TRACK;
				}
				if (value == "bridleway")
					m_pLink->m_Surface = SURFT_GRAVEL;
				if (value == "footway")
				{
					m_pLink->m_iLanes = 1;
					m_pLink->m_Surface = SURFT_GRAVEL;
				}
			}
			if (key == "waterway")
				m_bAddLink = false;
			if (key == "railway")
				m_pLink->m_Surface = SURFT_RAILROAD;
			if (key == "aeroway")
				m_bAddLink = false;
			if (key == "aerialway")
				m_bAddLink = false;
			if (key == "power")
				m_bAddLink = false;
			if (key == "man_made")
				m_bAddLink = false;
			if (key == "leisure")
				m_bAddLink = false;
			if (key == "amenity")
				m_bAddLink = false;
			if (key == "abutters")
				m_bAddLink = false;
			if (key == "surface")
			{
				if (value == "paved")
					m_pLink->m_Surface = SURFT_PAVED;
				if (value == "unpaved")
					m_pLink->m_Surface = SURFT_GRAVEL;
			}
			if (key == "lanes")
				m_pLink->m_iLanes = atoi(value);
		}
	}
}

void VisitorOSM::endElement(const char *name)
{
	if (m_state == 1 && !strcmp(name, "node"))
	{
		m_state = 0;
	}
	else if (m_state == 2 && !strcmp(name, "way"))
	{
		// Look at the referenced nodes, turn them into a vt link
		unsigned int refs = m_refs.size();

		// must have at least 2 refs
		if (refs >= 2 && m_bAddLink == true)
		{
			int ref_first = m_refs[0];
			int ref_last = m_refs[refs-1];

			int idx_first = find_node(ref_first);
			int idx_last = find_node(ref_last);

			TNode *node0 = m_pMap->FindNodeByID(m_nodes[idx_first].id);
			if (!node0)
			{
				// doesn't exist, create it
				node0 = m_pMap->NewNode();
				node0->m_p = m_nodes[idx_first].p;
				node0->m_id = m_nodes[idx_first].id;
				m_pMap->AddNode(node0);
			}
			m_pLink->SetNode(0, node0);

			TNode *node1 = m_pMap->FindNodeByID(m_nodes[idx_last].id);
			if (!node1)
			{
				// doesn't exist, create it
				node1 = m_pMap->NewNode();
				node1->m_p = m_nodes[idx_last].p;
				node1->m_id = m_nodes[idx_last].id;
				m_pMap->AddNode(node1);
			}
			m_pLink->SetNode(1, node1);

			// Copy all the points
			for (unsigned int r = 0; r < refs; r++)
			{
				int idx = find_node(m_refs[r]);
				m_pLink->Append(m_nodes[idx].p);
			}

			m_pMap->AddLink(m_pLink);

			// point node to links
			node0->AddLink(m_pLink, true);
			node1->AddLink(m_pLink, false);

			m_pLink->ComputeExtent();

			m_pLink = NULL;
		}

		m_state = 0;
	}
}

void VisitorOSM::data(const char *s, int length)
{
	//m_data.append(string(s, length));
}

// Import from OpenStreetMap
bool RoadMapEdit::ImportFromOSM(const char *fname, bool progress_callback(int))
{
	// OSM is always in Geo WGS84
	m_proj.SetWellKnownGeogCS("WGS84");

	VisitorOSM visitor(this);
	try
	{
		readXML(fname, visitor, progress_callback);
	}
	catch (xh_exception &ex)
	{
		VTLOG(ex.getFormattedMessage().c_str());
		return false;
	}
	visitor.SetSignalLights();
	return true;
}
