//
// GeometryUtils.cpp - Utilities gor generating OSG geoemtry
//
// Copyright (c) 2001-2011 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#include "vtlib/vtlib.h"
#include "GeometryUtils.h"
#include "vtlib/core/Building3d.h"
#include "vtlib/core/FelkelStraightSkeleton.h"
#include "vtdata/vtLog.h"
#include "vtdata/Triangulate.h"
#include "vtdata/DataPath.h"
#include "vtlib/core/Light.h"

namespace OSGGeometryUtils {

vtGeode* GenerateBuildingGeometry::Generate()
{
	m_pGeode = new vtGeode;

	int i;
	unsigned int j, k;
	int iLevels = m_Building.GetNumLevels();

	// create the edges (walls and roof)
	float fHeight = 0.0f;
	int level_show = -1, edge_show = -1;
	m_Building.GetValueInt("level", level_show);
	m_Building.GetValueInt("edge", edge_show);

	for (i = 0; i < iLevels; i++)
	{
		const vtLevel *lev = m_Building.GetLevel(i);
		const FPolygon3 &foot = m_Building.GetLocalFootprint(i);
		unsigned int edges = lev->NumEdges();

		// safety check
		if (foot[0].GetSize() < 3)
			return NULL;

		if (lev->IsHorizontal())
		{
			// make flat roof
			AddFlatRoof(foot, lev);
		}
		else if (lev->IsUniform())
		{
			int iHighlightEdge = level_show == i ? edge_show : -1;
			CreateUniformLevel(i, fHeight, iHighlightEdge);
			fHeight += lev->m_iStories * lev->m_fStoryHeight;
		}
		else if (lev->HasSlopedEdges() && edges > 4)
		{
			// For complicated roofs with sloped edges which meet at a
			// roofline of uneven height, we need a sophisticated
			// straight-skeleton solution like Petr Felkel's
			float fRoofHeight = MakeFelkelRoof(foot, lev);
			if (fRoofHeight < 0.0)
			{
				VTLOG("Failed to make Felkel roof - reverting to flat roof\n");
				AddFlatRoof(foot, lev);
			}
			fHeight += fRoofHeight;
		}
		else
		{
			// Build a 'flat roof' for the floor
			AddFlatRoof(foot, lev);

			FPolygon3 poly = foot;
			FPolygon3 poly2;

			// Build a set of walls for each storey of the level
			for (j = 0; j < lev->m_iStories; j++)
			{
				for (unsigned int r = 0; r < poly.size(); r++)
				{
					for (k = 0; k < poly[r].GetSize(); k++)
					{
						poly[r][k].y = fHeight;
					}
				}
				CreateUpperPolygon(lev, poly, poly2);
				int edge_start = 0;
				for (unsigned int r = 0; r < poly.size(); r++)
				{
					for (k = edge_start; k < edge_start + poly[r].GetSize(); k++)
					{
						bool bShowEdge = (level_show == i && edge_show == k);
						CreateEdgeGeometry(lev, poly, poly2, k, bShowEdge);
					}
					edge_start += poly[r].GetSize();
				}
				fHeight += lev->m_fStoryHeight;
			}
		}
	}

//	osgDB::Registry::instance()->writeNode(*m_pGeode, std::string("building.osg"), NULL);

	return m_pGeode.release();
}

void GenerateBuildingGeometry::AddFlatRoof(const FPolygon3 &pp,  const vtLevel *pLev)
{
	FPoint3 up(0.0f, 1.0f, 0.0f);	// vector pointing up
	int rings = pp.size();
	int outer_corners = pp[0].GetSize();
	int i, j;
	FPoint2 uv;

	// Use the material of the first edge 
	vtEdge *pEdge = pLev->GetEdge(0);
	const vtString& Material = *pEdge->m_pMaterial;
	int MaterialIndex = FindMatIndex(Material, pEdge->m_Color);
	vtMaterialDescriptor *md = s_MaterialDescriptors.FindMaterialDescriptor(Material, pEdge->m_Color);
	vtMaterial* pMaterial = GetSharedMaterialArray()->at(MaterialIndex);
	osg::Geometry *pGeometry = FindOrCreateGeometryObject(m_pGeode.get(), *pMaterial, VT_Normals|VT_TexCoords);
	osg::Vec3Array *pVertexArray = static_cast<osg::Vec3Array*>(pGeometry->getVertexArray());
	osg::Vec3Array *pNormalArray = static_cast<osg::Vec3Array*>(pGeometry->getNormalArray());
	osg::Vec2Array *pTexCoordArray = static_cast<osg::Vec2Array*>(pGeometry->getTexCoordArray(0));
	osg::DrawElementsUShort *pPrimSet = static_cast<osg::DrawElementsUShort*>(FindOrCreatePrimitiveSet(pGeometry, osg::PrimitiveSet::TRIANGLES, osg::PrimitiveSet::DrawElementsUShortPrimitiveType));

	if (outer_corners > 4 || rings > 1)
	{
		// roof consists of a polygon which must be split into triangles
		//  Invoke the triangulator to triangulate this polygon.
		// Use 'Triangle'
		const FLine3 &outer = pp[0];
		float roof_y = outer[0].y;
		DPolygon2 foot2d;
		ProjectionXZ(pp, foot2d);

		// Triangle has been known to behave poorly with redundant vertices
		//  We are in meters now, so we can use a centimeter epsilon.
		int removed = foot2d.RemoveDegeneratePoints(0.08);
		if (removed)
			VTLOG("Skipped %d redundant vertices.\n", removed);

		// a polyline to hold the answer in sets of three points
		DLine2 result2d;
		CallTriangle(foot2d, result2d);

		FLine3 result;
		ProjectionXZ(result2d, roof_y, result);

		// use the results.
		int tcount = result.GetSize()/3;
		int ind[3];
		FPoint2 gp;
		FPoint3 p;

		for (i=0; i<tcount; i++)
		{
			for (j = 0; j < 3; j++)
			{
				p = result[i*3+j];
				uv.Set(p.x, p.z);
				if (md)
					uv.Div(md->GetUVScale());	// divide meters by [meters/uv] to get uv
				pVertexArray->push_back(osg::Vec3(p.x, p.y, p.z));
				ind[j] = pVertexArray->size() - 1;
				pNormalArray->push_back(osg::Vec3(up.x, up.y, up.z));
				pTexCoordArray->push_back(osg::Vec2(uv.x, uv.y));
			}
			pPrimSet->addElement(ind[0]);
			pPrimSet->addElement(ind[1]);
			pPrimSet->addElement(ind[2]);
		}
	}
	else
	{
		int idx[MAX_WALLS];
		for (i = 0; i < outer_corners; i++)
		{
			FPoint3 p = pp[0][i];
			uv.Set(p.x, p.z);
			if (md)
				uv.Div(md->GetUVScale());	// divide meters by [meters/uv] to get uv
			pVertexArray->push_back(osg::Vec3(p.x, p.y, p.z));
			idx[i] = pVertexArray->size() - 1;
			pNormalArray->push_back(osg::Vec3(up.x, up.y, up.z));
			pTexCoordArray->push_back(osg::Vec2(uv.x, uv.y));
		}
		if (outer_corners > 2)
		{
			pPrimSet->addElement(idx[0]);
			pPrimSet->addElement(idx[1]);
			pPrimSet->addElement(idx[2]);
		}
		if (outer_corners > 3)
		{
			pPrimSet->addElement(idx[2]);
			pPrimSet->addElement(idx[3]);
			pPrimSet->addElement(idx[0]);
		}
	}
}

//
// Walls which consist of regularly spaced windows and 'siding' material
// can be modelled far more efficiently.  This is very useful for rendering
// speed for large scenes in which the user doesn't have or doesn't care
// about the exact material/windows of the buildings.  We create
// optimized geometry in which each whole wall is a single quad.
//
void GenerateBuildingGeometry::CreateUniformLevel(int iLevel, float fHeight,
	int iHighlightEdge)
{
	const vtLevel *pLev = m_Building.GetLevel(iLevel);

	const FPolygon3 &polygon1 = m_Building.GetLocalFootprint(iLevel);

	int i;
	int base_edge = 0;
	for (unsigned int ring = 0; ring < polygon1.size(); ring++)
	{
		FLine3 poly1 = polygon1[ring];
		FLine3 poly2;

		int edges = poly1.GetSize();
		for (i = 0; i < edges; i++)
			poly1[i].y = fHeight;

		poly2 = poly1;
		for (i = 0; i < edges; i++)
			poly2[i].y += pLev->m_fStoryHeight;

		for (i = 0; i < edges; i++)
		{
			int a = i, b = (a+1)%edges;

			FLine3 quad(4);

			vtEdge	*pEdge = pLev->GetEdge(base_edge+i);

			// do the whole wall section
			quad[0] = poly1[a];
			quad[1] = poly1[b];
			quad[2] = poly2[a];
			quad[3] = poly2[b];

			if (pEdge->m_Facade != "")
			{
				float extraheight = pLev->m_fStoryHeight * (pLev->m_iStories-1);
				quad[2].y += extraheight;
				quad[3].y += extraheight;
				// If we can successfully construct the facade, we don't need to
				//  use the edge features.
				if (MakeFacade(pEdge, quad, pLev->m_iStories))
					continue;
			}
			quad[2] = poly2[a];
			quad[3] = poly2[b];

			float h1 = 0.0f;
			float h2 = (float) pLev->m_iStories;
			float hf1 = (float) pEdge->NumFeaturesOfCode(WFC_WINDOW);
			AddWallSection(pEdge, true, quad, h1, h2, hf1);

			if (base_edge+i == iHighlightEdge)
			{
				for (unsigned int j = 0; j < pLev->m_iStories; j++)
				{
					AddHighlightSection(pEdge, quad);
					quad[0].y += pLev->m_fStoryHeight;
					quad[1].y += pLev->m_fStoryHeight;
					quad[2].y += pLev->m_fStoryHeight;
					quad[3].y += pLev->m_fStoryHeight;
				}
			}
		}
		base_edge += edges;
	}
}

bool GenerateBuildingGeometry::MakeFacade(vtEdge *pEdge, FLine3 &quad, int stories)
{
	// Paint a facade on this edge
	// Add the facade image to the materials array
	// Assume quad is ordered 0,1,3,2
	osg::Vec3 norm = Normal(quad[0],quad[1],quad[3]);

	vtString fname = "Facade/";
	fname += pEdge->m_Facade;
	fname = FindFileOnPaths(vtGetDataPath(), (pcchar)fname);
	if (fname == "")
	{
		// Older files may have their facades in 'BuildingModels'
		fname = "BuildingModels/";
		fname += pEdge->m_Facade;
		fname = FindFileOnPaths(vtGetDataPath(), (pcchar)fname);
	}
	if (fname == "")
	{
		VTLOG(" Couldn't find facade texture '%s'\n", (const char*)pEdge->m_Facade);
		return false;
	}

	// !!!!!! TODO we should probably search for this image rather than just adding it each time it is used
	int MaterialIndex = GetSharedMaterialArray()->AddTextureMaterial2(fname,
			true, true, false, false,
			TERRAIN_AMBIENT,
			TERRAIN_DIFFUSE,
			1.0f,		// alpha
			TERRAIN_EMISSIVE);

	vtMaterial* pMaterial = GetSharedMaterialArray()->at(MaterialIndex);
	osg::Geometry *pGeometry = FindOrCreateGeometryObject(m_pGeode.get(), *pMaterial, VT_Normals|VT_TexCoords);
	osg::Vec3Array *pVertexArray = static_cast<osg::Vec3Array*>(pGeometry->getVertexArray());
	osg::Vec3Array *pNormalArray = static_cast<osg::Vec3Array*>(pGeometry->getNormalArray());
	osg::Vec2Array *pTexCoordArray = static_cast<osg::Vec2Array*>(pGeometry->getTexCoordArray(0));
	osg::DrawArrays *pPrimSet = static_cast<osg::DrawArrays*>(FindOrCreatePrimitiveSet(pGeometry, osg::PrimitiveSet::QUADS, osg::PrimitiveSet::DrawArraysPrimitiveType));

	float v = (float) stories;
	pVertexArray->push_back(osg::Vec3(quad[0].x, quad[0].y, quad[0].z));
	pNormalArray->push_back(norm);
	pTexCoordArray->push_back(osg::Vec2(0.0, 0.0));
	pVertexArray->push_back(osg::Vec3(quad[1].x, quad[1].y, quad[1].z));
	pNormalArray->push_back(norm);
	pTexCoordArray->push_back(osg::Vec2(1.0, 0.0));
	pVertexArray->push_back(osg::Vec3(quad[3].x, quad[3].y, quad[3].z));
	pNormalArray->push_back(norm);
	pTexCoordArray->push_back(osg::Vec2(1.0, v));
	pVertexArray->push_back(osg::Vec3(quad[2].x, quad[2].y, quad[2].z));
	pNormalArray->push_back(norm);
	pTexCoordArray->push_back(osg::Vec2(0.0, v));
	// setFirst has already been done for us
	pPrimSet->setCount(4);
	return true;
}

osg::Vec3 GenerateBuildingGeometry::Normal(const vtVec3 &p0, const vtVec3 &p1, const vtVec3 &p2)
{
	osg::Vec3 a = p0 - p1;
	osg::Vec3 b = p2 - p1;
	osg::Vec3 norm = b ^ a;
	norm.normalize();
	return norm;
}

/**
 * Builds a wall, given material index, starting and end points, height, and
 * starting height.
 */
void GenerateBuildingGeometry::AddWallSection(vtEdge *pEdge, bool bUniform,
	const FLine3 &quad, float vf1, float vf2, float hf1)
{
	// determine 4 points at corners of wall section
	vtVec3 up1 = quad[2] - quad[0];
	vtVec3 up2 = quad[3] - quad[1];
	vtVec3 p0 = vtVec3(quad[0]) + (up1 * vf1);
	vtVec3 p1 = vtVec3(quad[1]) + (up2 * vf1);
	vtVec3 p3 = vtVec3(quad[0]) + (up1 * vf2);
	vtVec3 p2 = vtVec3(quad[1]) + (up2 * vf2);

	int MaterialIndex;
	if (bUniform)
		MaterialIndex = FindMatIndex(BMAT_NAME_WINDOWWALL, pEdge->m_Color);
	else
		MaterialIndex = FindMatIndex(*pEdge->m_pMaterial, pEdge->m_Color);
	vtMaterial* pMaterial = GetSharedMaterialArray()->at(MaterialIndex);
	osg::Geometry *pGeometry = FindOrCreateGeometryObject(m_pGeode.get(), *pMaterial, VT_Normals|VT_TexCoords);
	osg::Vec3Array *pVertexArray = static_cast<osg::Vec3Array*>(pGeometry->getVertexArray());
	osg::Vec3Array *pNormalArray = static_cast<osg::Vec3Array*>(pGeometry->getNormalArray());
	osg::Vec2Array *pTexCoordArray = static_cast<osg::Vec2Array*>(pGeometry->getTexCoordArray(0));
	osg::DrawArrays *pPrimSet = static_cast<osg::DrawArrays*>(FindOrCreatePrimitiveSet(pGeometry, osg::PrimitiveSet::QUADS, osg::PrimitiveSet::DrawArraysPrimitiveType));

	// determine normal and primary axes of the face
	osg::Vec3 norm = Normal(p0, p1, p2);
	vtVec3 axis0, axis1;
	axis0 = p1 - p0;
	axis0.normalize();
	axis1 = norm ^ axis0;

	// determine UVs - special case for window-wall texture
	vtVec2 uv0, uv1, uv2, uv3;
	if (bUniform)
	{
		uv0.set(0, 0);
		uv1.set(hf1, 0);
		uv2.set(hf1, vf2);
		uv3.set(0, vf2);
	}
	else
	{
		float u1 = (p1 - p0) * axis0;
		float u2 = (p2 - p0) * axis0;
		float u3 = (p3 - p0) * axis0;
		float v2 = (p2 - p0) * axis1;
		vtMaterialDescriptor *md = s_MaterialDescriptors.FindMaterialDescriptor(*pEdge->m_pMaterial, pEdge->m_Color);
		uv0.set(0, 0);
		uv1.set(u1, 0);
		uv2.set(u2, v2);
		uv3.set(u3, v2);
		if (md != NULL)
		{
			// divide meters by [meters/uv] to get uv
			vtVec2 UVScale = md->GetUVScale();
			uv0 /= UVScale;
			uv1 /= UVScale;
			uv2 /= UVScale;
			uv3 /= UVScale;
		}
	}

	pVertexArray->push_back(p0);
	pNormalArray->push_back(norm);
	pTexCoordArray->push_back(uv0);
	pVertexArray->push_back(p1);
	pNormalArray->push_back(norm);
	pTexCoordArray->push_back(uv1);
	pVertexArray->push_back(p2);
	pNormalArray->push_back(norm);
	pTexCoordArray->push_back(uv2);
	pVertexArray->push_back(p3);
	pNormalArray->push_back(norm);
	pTexCoordArray->push_back(uv3);
	// setFirst has already been done for us
	pPrimSet->setCount(4);
}

/**
 * Creates geometry for a highlighted area (an edge).
 */
void GenerateBuildingGeometry::AddHighlightSection(vtEdge *pEdge,
	const FLine3 &quad)
{
	// determine 4 points at corners of wall section
	vtVec3 p0 = quad[0];
	vtVec3 p1 = quad[1];
	vtVec3 p3 = quad[2];
	vtVec3 p2 = quad[3];

	int MaterialIndex = FindMatIndex(BMAT_NAME_PLAIN, RGBi(255,255,255));
	vtMaterial* pMaterial = GetSharedMaterialArray()->at(MaterialIndex);
	osg::Geometry *pGeometry = FindOrCreateGeometryObject(m_pGeode.get(), *pMaterial, 0);
	osg::Vec3Array *pVertexArray = static_cast<osg::Vec3Array*>(pGeometry->getVertexArray());
	osg::DrawArrayLengths *pPrimSet = static_cast<osg::DrawArrayLengths*>(FindOrCreatePrimitiveSet(pGeometry, osg::PrimitiveSet::LINE_STRIP, osg::PrimitiveSet::DrawArrayLengthsPrimitiveType));

	// determine normal (not used for shading)
	vtVec3 norm = Normal(p0,p1,p2);

	pVertexArray->push_back(p0 + norm);
	pVertexArray->push_back(p1 + norm);
	pVertexArray->push_back(p2 + norm);
	pVertexArray->push_back(p3 + norm);
	pVertexArray->push_back(p0 + norm);
	pPrimSet->push_back(5);

	pVertexArray->push_back(p0);
	pVertexArray->push_back(p0 + norm);
	pPrimSet->push_back(2);

	pVertexArray->push_back(p1);
	pVertexArray->push_back(p1 + norm);
	pPrimSet->push_back(2);

	pVertexArray->push_back(p2);
	pVertexArray->push_back(p2 + norm);
	pPrimSet->push_back(2);

	pVertexArray->push_back(p3);
	pVertexArray->push_back(p3 + norm);
	pPrimSet->push_back(2);

	norm *= 0.95f;

	MaterialIndex = FindMatIndex(BMAT_NAME_PLAIN, RGBi(255,0,0));
	pMaterial = GetSharedMaterialArray()->at(MaterialIndex);
	pGeometry = FindOrCreateGeometryObject(m_pGeode.get(), *pMaterial, 0);
	pVertexArray = static_cast<osg::Vec3Array*>(pGeometry->getVertexArray());
	osg::DrawArrays *pPrimSet2 = static_cast<osg::DrawArrays*>(FindOrCreatePrimitiveSet(pGeometry, osg::PrimitiveSet::LINE_STRIP, osg::PrimitiveSet::DrawArraysPrimitiveType));

	pVertexArray->push_back(p0 + norm);
	pVertexArray->push_back(p1 + norm);
	pVertexArray->push_back(p2 + norm);
	pVertexArray->push_back(p3 + norm);
	pVertexArray->push_back(p0 + norm);
	// setFirst has already been done for us
	pPrimSet2->setCount(5);
}

float GenerateBuildingGeometry::MakeFelkelRoof(const FPolygon3 &EavePolygons, const vtLevel *pLev)
{
	vtStraightSkeleton StraightSkeleton;
	CSkeleton Skeleton;
	float fMaxHeight = 0.0;
	ContourVector RoofEaves(EavePolygons.size());
	int i;
	CSkeletonLine *pStartEdge;
	CSkeletonLine *pEdge;
	CSkeletonLine *pNextEdge;
	bool bEdgeReversed;
	float EaveY = EavePolygons[0][0].y;
#ifdef FELKELDEBUG
	float DebugX;
	float DebugY;
	float DebugZ;
#endif

	// Make a roof using felkels straight skeleton algorithm

	// First of all build the eave footprint.
	ContourVector::iterator itV = RoofEaves.begin();
	for (FPolygon3::const_iterator itP = EavePolygons.begin(); itP != EavePolygons.end(); itP++, itV++)
	{
		int iVertices = (*itP).GetSize();
		for (i = 0; i < iVertices; i++)
		{
			FPoint3 CurrentPoint = (*itP)[i];
			FPoint3 NextPoint = (*itP)[(i+1)%iVertices];
			FPoint3 PreviousPoint = (*itP)[(iVertices+i-1)%iVertices];
			int iSlope = pLev->GetEdge(i)->m_iSlope;
			if (iSlope > 89)
				iSlope = 90;
			else if (iSlope < 1)
				iSlope = 0;
			int iPrevSlope = pLev->GetEdge((iVertices+i-1)%iVertices)->m_iSlope;
			if (iPrevSlope > 89)
				iPrevSlope = 90;
			else if (iPrevSlope < 1)
				iPrevSlope = 0;
			// If edges are in line and slopes are different then
			if ((iPrevSlope != iSlope)
				&& Collinear2d(PreviousPoint, CurrentPoint, NextPoint))
			{
#ifdef FELKELDEBUG
				VTLOG("Adding dummy eave segment at %d\n", i);
#endif
				// Duplicate the current edge vector
				FPoint3 OldEdge = NextPoint - CurrentPoint;
				FPoint3 NewEdge;
				int iNewSlope;
				if (iSlope > iPrevSlope)
				{
					// Rotate new vertex inwards (clockwise)
					NewEdge.x = OldEdge.z;
					NewEdge.z = -OldEdge.x;
					iNewSlope = iPrevSlope;
				}
				else
				{
					// Rotate new vertext outwards (anticlockwise)
					NewEdge.x = -OldEdge.z;
					NewEdge.z = OldEdge.x;
					iNewSlope = iSlope;
				}
				// Scale to .01 of a co-ord unit
				NewEdge.Normalize();
				NewEdge = NewEdge/100.0f;
				NewEdge += CurrentPoint;
				(*itV).push_back(CEdge(NewEdge.x, 0, NewEdge.z,
					iNewSlope / 180.0f * PIf, pLev->GetEdge(i)->m_pMaterial,
					pLev->GetEdge(i)->m_Color));
			}
			(*itV).push_back(CEdge(CurrentPoint.x, 0, CurrentPoint.z,
				iSlope / 180.0f * PIf, pLev->GetEdge(i)->m_pMaterial,
				pLev->GetEdge(i)->m_Color));
		}
	}

	// Now build the skeleton
	StraightSkeleton.MakeSkeleton(RoofEaves);

	if (0 == StraightSkeleton.m_skeleton.size())
		return -1.0;

	// Merge the original eaves back into the skeleton
	Skeleton = StraightSkeleton.CompleteWingedEdgeStructure(RoofEaves);

	if (0 == Skeleton.size())
		return -1.0;

#ifdef FELKELDEBUG
	VTLOG("Building Geometry\n");
#endif
	// TODO - texture co-ordinates
	// Build the geometry
	for (size_t ci = 0; ci < RoofEaves.size(); ci++)
	{
		Contour& points = RoofEaves[ci];
		for (size_t pi = 0; pi < points.size(); pi++)
		{
			// For each boundary edge zip round the polygon anticlockwise
			// and build the vertex array
			const vtString bmat = *points[pi].m_pMaterial;
			const vtString& Material = *points[pi].m_pMaterial;
			int MaterialIndex = FindMatIndex(Material, points[pi].m_Color);
			vtMaterialDescriptor *pMd = s_MaterialDescriptors.FindMaterialDescriptor(Material, points[pi].m_Color);
			vtMaterial* pMaterial = GetSharedMaterialArray()->at(MaterialIndex);
			osg::Geometry *pGeometry = FindOrCreateGeometryObject(m_pGeode.get(), *pMaterial, VT_Normals|VT_TexCoords);
			osg::Vec3Array *pVertexArray = static_cast<osg::Vec3Array*>(pGeometry->getVertexArray());
			osg::Vec3Array *pNormalArray = static_cast<osg::Vec3Array*>(pGeometry->getNormalArray());
			osg::Vec2Array *pTexCoordArray = static_cast<osg::Vec2Array*>(pGeometry->getTexCoordArray(0));
			osg::DrawElementsUShort *pPrimSet = static_cast<osg::DrawElementsUShort*>(FindOrCreatePrimitiveSet(pGeometry, osg::PrimitiveSet::TRIANGLES, osg::PrimitiveSet::DrawElementsUShortPrimitiveType));
			FPoint2 UVScale;
			if (NULL != pMd)
				UVScale = pMd->GetUVScale();
			else
				UVScale = FPoint2(1.0, 1.0);
			FLine3 RoofSection3D;
			FLine3 TriangulatedRoofSection3D;
			int iTriangleCount = 0;
			vtVec3 PanelNormal;
			vtVec3 UAxis;
			vtVec3 VAxis;
			vtVec3 TextureOrigin;
			int i, j;
			vtArray<int> iaVertices;

			C3DPoint& p1 = points[pi].m_Point;
			C3DPoint& p2 = points[(pi+1)%points.size()].m_Point;
			// Find the starting edge
			CSkeleton::iterator s1;
			for (s1 = Skeleton.begin(); s1 != Skeleton.end(); s1++)
			{
				if (((*s1).m_lower.m_vertex->m_point == p1) &&
					((*s1).m_higher.m_vertex->m_point == p2))
					break;
			}
			if (s1 == Skeleton.end())
				break;

			pStartEdge = &(*s1);
			pEdge = pStartEdge;
			bEdgeReversed = false;
#ifdef FELKELDEBUG
			VTLOG("Building panel\n");
#endif
			unsigned int iNumberofPoints = 0;
			do
			{
				if (iNumberofPoints++ > Skeleton.size())
				{
					VTLOG("MakeFelkelRoof - Roof geometry too complex - giving up\n");
					return -1.0;
				}
				if (bEdgeReversed)
				{
#ifdef FELKELDEBUG
					DebugX = pEdge->m_higher.m_vertex->m_point.m_x;
					DebugY = pEdge->m_higher.m_vertex->m_point.m_y;
					DebugZ = pEdge->m_higher.m_vertex->m_point.m_z;
#endif
					if (pEdge->m_higher.m_vertex->m_point.m_z > (double)fMaxHeight)
						fMaxHeight = (float) pEdge->m_higher.m_vertex->m_point.m_z;
					RoofSection3D.Append(FPoint3(pEdge->m_higher.m_vertex->m_point.m_x, pEdge->m_higher.m_vertex->m_point.m_y + EaveY, pEdge->m_higher.m_vertex->m_point.m_z));
					pNextEdge = pEdge->m_higher.m_right;
//					if (pEdge->m_higher.m_vertex->m_point != pNextEdge->m_higher.m_vertex->m_point)
					if (pEdge->m_higher.VertexID() != pNextEdge->m_higher.VertexID())
						bEdgeReversed = true;
					else
						bEdgeReversed = false;
				}
				else
				{
#ifdef FELKELDEBUG
					DebugX = pEdge->m_lower.m_vertex->m_point.m_x;
					DebugY = pEdge->m_lower.m_vertex->m_point.m_y;
					DebugZ = pEdge->m_lower.m_vertex->m_point.m_z;
#endif
					if (pEdge->m_lower.m_vertex->m_point.m_z > (double)fMaxHeight)
						fMaxHeight = (float) pEdge->m_lower.m_vertex->m_point.m_z;
					RoofSection3D.Append(FPoint3(pEdge->m_lower.m_vertex->m_point.m_x, pEdge->m_lower.m_vertex->m_point.m_y + EaveY, pEdge->m_lower.m_vertex->m_point.m_z));
					pNextEdge = pEdge->m_lower.m_right;
//					if (pEdge->m_lower.m_vertex->m_point != pNextEdge->m_higher.m_vertex->m_point)
					if (pEdge->m_lower.VertexID() != pNextEdge->m_higher.VertexID())
						bEdgeReversed = true;
					else
						bEdgeReversed = false;
				}
#ifdef FELKELDEBUG
				VTLOG("Adding point (ID %d) x %e y %e z %e\n", pEdge->m_ID, DebugX, DebugY, DebugZ);
#endif
				pEdge = pNextEdge;
			}
			// For some reason the pointers dont end up quite the same
			// I will work it out someday
			while (pEdge->m_ID != pStartEdge->m_ID);


			// Remove any vertices that are the same
			for (i = 0; i < (int)RoofSection3D.GetSize(); i++)
			{
				FPoint3& Point = RoofSection3D[i];

				for (j = i + 1; j < (int)RoofSection3D.GetSize(); j++)
				{
					FPoint3& NextPoint = RoofSection3D[j];
					if (NextPoint == Point)
					{
						RoofSection3D.RemoveAt(j);
						j--;
					}
				}
			}


			// determine normal and primary axes of the face
			j = RoofSection3D.GetSize();
			PanelNormal = Normal(RoofSection3D[1], RoofSection3D[0], RoofSection3D[j-1]);
			UAxis = FPoint3(RoofSection3D[j-1] - RoofSection3D[0]).Normalize();
			VAxis = PanelNormal ^ UAxis;
			TextureOrigin = RoofSection3D[0];
#ifdef FELKELDEBUG
			VTLOG("Panel normal x %e y %e z %e\n", PanelNormal.x, PanelNormal.y, PanelNormal.z);
#endif
			// Build transform to rotate plane parallel to the xz plane.
			// N.B. this only work with angles from the plane normal to the y axis
			// in the rangle 0 to pi/2 (this is ok for roofs). If you want
			// it to work over a greater range you will have to mess with the sign of the cosine
			// of this angle.
			float fHypot = sqrtf(PanelNormal.x() * PanelNormal.x() + PanelNormal.z() * PanelNormal.z());
			FMatrix3 Transform;
			Transform.SetRow(0, PanelNormal.x() * PanelNormal.y() / fHypot, PanelNormal.x(), -PanelNormal.z() / fHypot);
			Transform.SetRow(1, -fHypot, PanelNormal.y(), 0);
			Transform.SetRow(2, PanelNormal.z() * PanelNormal.y() / fHypot, PanelNormal.z(), PanelNormal.x() / fHypot);

			// Build vertex list
			for (i = 0; i < j; i++)
			{
				vtVec3 Vertex = RoofSection3D[i];
				vtVec2 UV = osg::Vec2((Vertex - TextureOrigin) * UAxis, (Vertex - TextureOrigin) * VAxis);
				UV /= UVScale;
				pVertexArray->push_back(Vertex);
				iaVertices.Append(pVertexArray->size() - 1);
				pNormalArray->push_back(PanelNormal);
				pTexCoordArray->push_back(UV);
			}

			if ((PanelNormal.x() != 0) && (PanelNormal.z() != 0))
			{
				for (i = 0; i < j; i++)
				{
					// Source and dest cannot be the same
					FPoint3 Temp = RoofSection3D[i];
					Transform.Transform(Temp, RoofSection3D[i]);
				}
			}

			if (!Triangulate_f::Process(RoofSection3D, TriangulatedRoofSection3D))
				return -1.0;

			iTriangleCount = TriangulatedRoofSection3D.GetSize() / 3;

			for (i = 0; i < iTriangleCount; i++)
			{
				int iaIndex[3];

				for (j = 0; j < 3; j++)
				{
					FPoint3 Point = TriangulatedRoofSection3D[i * 3 + j];
					if (-1 == (iaIndex[j] = FindVertex(Point, RoofSection3D, iaVertices)))
						return -1.0;
				}
				pPrimSet->addElement(iaIndex[0]);
				pPrimSet->addElement(iaIndex[2]);
				pPrimSet->addElement(iaIndex[1]);
#ifdef FELKELDEBUG
				VTLOG("AddTri1 %d %d %d\n", iaIndex[0], iaIndex[2], iaIndex[1]);
#endif
			}
		}
	}

	return fMaxHeight;
}

bool GenerateBuildingGeometry::Collinear2d(const FPoint3& Previous, const FPoint3& Current, const FPoint3& Next)
{
	FPoint3 l1 = Previous - Current;
	FPoint3 l2 = Next - Current;

	l1.y = 0;
	l2.y = 0;

	l1.Normalize();
	l2.Normalize();

	float CosTheta = l1.Dot(l2);
	if (CosTheta < -1.0)
		CosTheta = -1.0;
	else if (CosTheta > 1.0)
		CosTheta = 1.0;
	float fTheta = acosf(CosTheta) / PIf * 180;

	if (fabs(fTheta - 180.0) < 1.0)
		return true;
	else
		return false;
}

void GenerateBuildingGeometry::CreateUpperPolygon(const vtLevel *lev, FPolygon3 &polygon,
									  FPolygon3 &polygon2)
{
	int i, prev, next;
	int rings = polygon.size();

	polygon2 = polygon;

	int base_edge = 0;
	for (int ring = 0; ring < rings; ring++)
	{
		FLine3 &poly = polygon[ring];
		FLine3 &poly2 = polygon2[ring];

		int edges = poly.GetSize();

		for (i = 0; i < edges; i++)
		{
			prev = (i-1 < 0) ? edges-1 : i-1;
			next = (i+1 == edges) ? 0 : i+1;

			FPoint3 p = poly[i];

			int islope1 = lev->GetEdge(base_edge + prev)->m_iSlope;
			int islope2 = lev->GetEdge(base_edge + i)->m_iSlope;
			if (islope1 == 90 && islope2 == 90)
			{
				// easy case
				p.y += lev->m_fStoryHeight;
			}
			else
			{
				float slope1 = (islope1 / 180.0f * PIf);
				float slope2 = (islope2 / 180.0f * PIf);

				// get edge vectors
				FPoint3 vec1 = poly[prev] - poly[i];
				FPoint3 vec2 = poly[next] - poly[i];
				vec1.Normalize();
				vec2.Normalize();

				// get perpendicular (upward pointing) vectors
				FPoint3 perp1, perp2;
				perp1.Set(0, 1, 0);
				perp2.Set(0, 1, 0);

				// create rotation matrices to rotate them upward
				FMatrix4 mat1, mat2;
				mat1.Identity();
				mat1.AxisAngle(vec1, -slope1);
				mat2.Identity();
				mat2.AxisAngle(vec2, slope2);

				// create normals
				FPoint3 norm1, norm2;
				mat1.TransformVector(perp1, norm1);
				mat2.TransformVector(perp2, norm2);

				// vector of plane intersection is cross product of their normals
				FPoint3 inter = norm1.Cross(norm2);
				// Test that intersection vector is pointing into the polygon
				// need a better test if we are going to handle downward sloping roofs
				if (inter.y < 0)
					inter = -inter;	// Reverse vector to point upward

				inter.Normalize();
				inter *= (lev->m_fStoryHeight / inter.y);

				p += inter;
			}
			poly2[i] = p;
		}
		base_edge += edges;
	}
}

//
// Edges are created from a series of features ("panels", "sections")
//
void GenerateBuildingGeometry::CreateEdgeGeometry(const vtLevel *pLev, const FPolygon3 &polygon1,
									  const FPolygon3 &polygon2, int iEdge, bool bShowEdge)
{
	// Get edge from complete list
	vtEdge	*pEdge = pLev->GetEdge(iEdge);

	// Then determine which ring its on
	int ring = polygon1.WhichRing(iEdge);
	const FLine3 &poly1 = polygon1[ring];
	const FLine3 &poly2 = polygon2[ring];

	// number of edges in this ring
	int num_edges = poly1.GetSize();
	int i = iEdge, j = (i+1)%num_edges;

	FLine3 quad(4);

	// start with the whole wall section
	quad[0] = poly1[i];
	quad[1] = poly1[j];
	quad[2] = poly2[i];
	quad[3] = poly2[j];

	// length of the edge
	FPoint3 dir1 = quad[1] - quad[0];
	FPoint3 dir2 = quad[3] - quad[2];
	float total_length1 = dir1.Length();
	float total_length2 = dir2.Length();
	if (total_length1 > 0.0f)
		dir1.Normalize();
	if (total_length2 > 0.0f)
		dir2.Normalize();

	if (bShowEdge)
	{
		AddHighlightSection(pEdge, quad);
	}

	// How wide should each feature be?
	// Determine how much space we have for the proportional features after
	// accounting for the fixed-width features
	float fixed_width = pEdge->FixedFeaturesWidth();
	float total_prop = pEdge->ProportionTotal();
	float dyn_width = total_length1 - fixed_width;

	if (pEdge->m_Facade != "")
	{
		// If we can successfully construct the facade, we don't need to
		//  use the edge features.
		if (MakeFacade(pEdge, quad, 1))
			return;
	}

	// build the edge features.
	// point[0] is the first starting point of a panel.
	for (i = 0; i < (int)pEdge->NumFeatures(); i++)
	{
		vtEdgeFeature &feat = pEdge->m_Features[i];

		// determine real width
		float meter_width = 0.0f;
		if (feat.m_width >= 0)
			meter_width = feat.m_width;
		else
			meter_width = (feat.m_width / total_prop) * dyn_width;
		quad[1] = quad[0] + dir1 * meter_width;
		quad[3] = quad[2] + dir2 * (meter_width * total_length2 / total_length1);

		if (feat.m_code == WFC_WALL)
		{
			AddWallNormal(pEdge, &feat, quad);
		}
		if (feat.m_code == WFC_GAP)
		{
			// do nothing
		}
		if (feat.m_code == WFC_POST)
		{
			// TODO
		}
		if (feat.m_code == WFC_WINDOW)
		{
			AddWindowSection(pEdge, &feat, quad);
		}
		if (feat.m_code == WFC_DOOR)
		{
			AddDoorSection(pEdge, &feat, quad);
		}
		quad[0] = quad[1];
		quad[2] = quad[3];
	}
}

void GenerateBuildingGeometry::AddWallNormal(vtEdge *pEdge, vtEdgeFeature *pFeat,
	const FLine3 &quad)
{
	float vf1 = pFeat->m_vf1;
	float vf2 = pFeat->m_vf2;
	AddWallSection(pEdge, false, quad, vf1, vf2);
}

//builds a window section.  builds the wall below and above a window too.
void GenerateBuildingGeometry::AddWindowSection(vtEdge *pEdge, vtEdgeFeature *pFeat,
	const FLine3 &quad)
{
	float vf1 = pFeat->m_vf1;
	float vf2 = pFeat->m_vf2;

	// build wall to base of window.
	AddWallSection(pEdge, false, quad, 0, vf1);

	// build wall above window
	AddWallSection(pEdge, false, quad, vf2, 1.0f);

	// determine 4 points at corners of section
	FPoint3 up1 = (quad[2] - quad[0]);
	FPoint3 up2 = (quad[3] - quad[1]);
	vtVec3 p0 = quad[0] + (up1 * vf1);
	vtVec3 p1 = quad[1] + (up2 * vf1);
	vtVec3 p3 = quad[0] + (up1 * vf2);
	vtVec3 p2 = quad[1] + (up2 * vf2);

	int MaterialIndex = MaterialIndex = FindMatIndex(BMAT_NAME_WINDOW, pEdge->m_Color);
	vtMaterial* pMaterial = GetSharedMaterialArray()->at(MaterialIndex);
	osg::Geometry *pGeometry = FindOrCreateGeometryObject(m_pGeode.get(), *pMaterial, VT_Normals|VT_TexCoords);
	osg::Vec3Array *pVertexArray = static_cast<osg::Vec3Array*>(pGeometry->getVertexArray());
	osg::Vec3Array *pNormalArray = static_cast<osg::Vec3Array*>(pGeometry->getNormalArray());
	osg::Vec2Array *pTexCoordArray = static_cast<osg::Vec2Array*>(pGeometry->getTexCoordArray(0));
	osg::DrawArrays *pPrimSet = static_cast<osg::DrawArrays*>(FindOrCreatePrimitiveSet(pGeometry, osg::PrimitiveSet::QUADS, osg::PrimitiveSet::DrawArraysPrimitiveType));

	// determine normal (flat shading, all vertices have the same normal)
	osg::Vec3  norm = Normal(p0,p1,p2);

	pVertexArray->push_back(p0);
	pNormalArray->push_back(norm);
	pTexCoordArray->push_back(osg::Vec2(0.0f, 0.0f));
	pVertexArray->push_back(p1);
	pNormalArray->push_back(norm);
	pTexCoordArray->push_back(osg::Vec2(1.0f, 0.0f));
	pVertexArray->push_back(p2);
	pNormalArray->push_back(norm);
	pTexCoordArray->push_back(osg::Vec2(1.0f, 1.0f));
	pVertexArray->push_back(p3);
	pNormalArray->push_back(norm);
	pTexCoordArray->push_back(osg::Vec2(0.0f, 1.0f));
	// setFirst has already been done for us
	pPrimSet->setCount(4);
}

/**
 * Builds a door section.  will also build the wall above the door to ceiling
 * height.
 */
void GenerateBuildingGeometry::AddDoorSection(vtEdge *pEdge, vtEdgeFeature *pFeat,
	const FLine3 &quad)
{
	float vf1 = 0;
	float vf2 = pFeat->m_vf2;

	// determine 4 points at corners of section
	FPoint3 up1 = (quad[2] - quad[0]);
	FPoint3 up2 = (quad[3] - quad[1]);
	vtVec3 p0 = quad[0] + (up1 * vf1);
	vtVec3 p1 = quad[1] + (up2 * vf1);
	vtVec3 p3 = quad[0] + (up1 * vf2);
	vtVec3 p2 = quad[1] + (up2 * vf2);

	int MaterialIndex = MaterialIndex = FindMatIndex(BMAT_NAME_DOOR, pEdge->m_Color);
	vtMaterial* pMaterial = GetSharedMaterialArray()->at(MaterialIndex);
	osg::Geometry *pGeometry = FindOrCreateGeometryObject(m_pGeode.get(), *pMaterial, VT_Normals|VT_TexCoords);
	osg::Vec3Array *pVertexArray = static_cast<osg::Vec3Array*>(pGeometry->getVertexArray());
	osg::Vec3Array *pNormalArray = static_cast<osg::Vec3Array*>(pGeometry->getNormalArray());
	osg::Vec2Array *pTexCoordArray = static_cast<osg::Vec2Array*>(pGeometry->getTexCoordArray(0));
	osg::DrawArrays *pPrimSet = static_cast<osg::DrawArrays*>(FindOrCreatePrimitiveSet(pGeometry, osg::PrimitiveSet::QUADS, osg::PrimitiveSet::DrawArraysPrimitiveType));

	// determine normal (flat shading, all vertices have the same normal)
	osg::Vec3 norm = Normal(p0, p1, p2);

	pVertexArray->push_back(p0);
	pNormalArray->push_back(norm);
	pTexCoordArray->push_back(osg::Vec2(0.0f, 0.0f));
	pVertexArray->push_back(p1);
	pNormalArray->push_back(norm);
	pTexCoordArray->push_back(osg::Vec2(1.0f, 0.0f));
	pVertexArray->push_back(p2);
	pNormalArray->push_back(norm);
	pTexCoordArray->push_back(osg::Vec2(1.0f, 1.0f));
	pVertexArray->push_back(p3);
	pNormalArray->push_back(norm);
	pTexCoordArray->push_back(osg::Vec2(0.0f, 1.0f));
	// setFirst has already been done for us
	pPrimSet->setCount(4);

	//add wall above door
	AddWallSection(pEdge, false, quad, vf2, 1.0f);
}

int GenerateBuildingGeometry::FindVertex(FPoint3 Point, FLine3 &RoofSection3D,
	vtArray<int> &iaVertices)
{
	int iSize = RoofSection3D.GetSize();

	int i;
	for (i = 0; i < iSize; i++)
	{
		if ((Point.x == RoofSection3D[i].x) &&
			(Point.y == RoofSection3D[i].y) &&
			(Point.z == RoofSection3D[i].z))
			break;
	}
	if (i < iSize)
		return iaVertices[i];
	else
	{
		VTLOG("FindVertex - vertex not found\n");
		return -1;
	}
}

class osg::Geometry* FindOrCreateGeometryObject(osg::Geode *pGeode, vtMaterial& Material, const int ArraysRequired)
{
	const osg::Geode::DrawableList& Drawables = pGeode->getDrawableList();

	osg::Array *pVertexArray = NULL;
	osg::Array *pNormalArray = NULL;
	osg::Array *pColorArray = NULL;
	osg::Array *pTexCoordArray = NULL;

	osg::Geode::DrawableList::const_iterator Drawable;
	for (Drawable = Drawables.begin(); Drawable != Drawables.end(); Drawable++)
	{
		osg::Geometry* pGeometry = dynamic_cast<osg::Geometry*>((*Drawable).get());
		if (NULL != pGeometry)
		{
			// First check if this object has the arrays we require
			// Must have a vertex array
			if (pGeometry->getVertexArray() == NULL)
				continue;
			if (ArraysRequired & VT_Normals)
			{
				if (pGeometry->getNormalArray() == NULL)
					continue;
				assert(pGeometry->getVertexArray()->getNumElements() == pGeometry->getNormalArray()->getNumElements());
			}
			else
				if (pGeometry->getNormalArray() != NULL)
					continue;
			if (ArraysRequired & VT_Colors)
			{
				if (pGeometry->getColorArray() == NULL)
					continue;
				assert(pGeometry->getVertexArray()->getNumElements() == pGeometry->getColorArray()->getNumElements());
			}
			else
				if (pGeometry->getColorArray() != NULL)
					continue;
			// At the moment we only use one texcoord array - FOR FURTHER STUDY !!!!!
			if (ArraysRequired & VT_TexCoords)
			{
				if (pGeometry->getTexCoordArray(0) == NULL)
					continue;
				assert(pGeometry->getVertexArray()->getNumElements() == pGeometry->getTexCoordArray(0)->getNumElements());
			}
			else
				if (pGeometry->getTexCoordArray(0) != NULL)
					continue;

			if (NULL == pVertexArray)
			{
				pVertexArray = pGeometry->getVertexArray();
				pNormalArray = pGeometry->getNormalArray();
				pColorArray = pGeometry->getColorArray();
				pTexCoordArray = pGeometry->getTexCoordArray(0);
			}

			osg::StateSet *pStateSet = pGeometry->getStateSet();
			if (NULL != pStateSet)
				if (*pStateSet == Material) // This does a compare with compareAttributeContents = false
					break;
		}
	}
	if (Drawable != Drawables.end())
		return dynamic_cast<osg::Geometry*>((*Drawable).get());

	osg::Geometry *pGeometry = new osg::Geometry;

	// Set up the state
	pGeometry->setStateSet(&Material);

	// Set up the arrays
	if (NULL == pVertexArray)
	{
		pGeometry->setVertexArray(new osg::Vec3Array);
		if (ArraysRequired & VT_Normals)
		{
			pGeometry->setNormalArray(new osg::Vec3Array);
			pGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
		}
		if (ArraysRequired & VT_Colors)
		{
			pGeometry->setColorArray(new osg::Vec4Array);
			pGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
		}
		if (ArraysRequired & VT_TexCoords)
			pGeometry->setTexCoordArray(0, new osg::Vec2Array);
	}
	else
	{
		pGeometry->setVertexArray(pVertexArray);
		if (ArraysRequired & VT_Normals)
		{
			pGeometry->setNormalArray(pNormalArray);
			pGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
		}
		if (ArraysRequired & VT_Colors)
		{
			pGeometry->setColorArray(pColorArray);
			pGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
		}
		if (ArraysRequired & VT_TexCoords)
			pGeometry->setTexCoordArray(0, pTexCoordArray);
	}

	// Use vertex buffer objects if available
	pGeometry->setUseVertexBufferObjects(true);

	pGeode->addDrawable(pGeometry);

	return pGeometry;
}

class osg::PrimitiveSet* FindOrCreatePrimitiveSet(osg::Geometry* pGeometry, const osg::PrimitiveSet::Mode Mode, const osg::PrimitiveSet::Type Type)
{
	osg::Geometry::PrimitiveSetList& PrimitiveSets = pGeometry->getPrimitiveSetList();

	// Only search for DrawElements types as we cannot assume anything about the values of _first and
	// _count for the DrawArrays and DrawArrayLengths types
	if ((Type != osg::PrimitiveSet::DrawArraysPrimitiveType) && (Type != osg::PrimitiveSet::DrawArrayLengthsPrimitiveType))
	{
		osg::Geometry::PrimitiveSetList::const_iterator PrimitiveSet;
		for (PrimitiveSet = PrimitiveSets.begin(); PrimitiveSet != PrimitiveSets.end(); PrimitiveSet++)
		{
			if (((*PrimitiveSet)->getMode() != Mode) && ((*PrimitiveSet)->getType() != Type))
				continue;
		}
		if (PrimitiveSet != PrimitiveSets.end())
			return (*PrimitiveSet).get();
	}



	osg::PrimitiveSet* pPrimSet;
	
	switch(Type)
	{
		case osg::PrimitiveSet::DrawArraysPrimitiveType:
			pPrimSet = new osg::DrawArrays(Mode);
			// For convenience set _first to the next free vertex position
			static_cast<osg::DrawArrays*>(pPrimSet)->setFirst(static_cast<osg::Vec3Array*>(pGeometry->getVertexArray())->size());
			break;
		case osg::PrimitiveSet::DrawArrayLengthsPrimitiveType:
			pPrimSet = new osg::DrawArrayLengths(Mode);
			// For convenience set _first to the next free vertex position
			static_cast<osg::DrawArrayLengths*>(pPrimSet)->setFirst(static_cast<osg::Vec3Array*>(pGeometry->getVertexArray())->size());
			break;
		case osg::PrimitiveSet::DrawElementsUBytePrimitiveType:
			pPrimSet = new osg::DrawElementsUByte(Mode);
			break;
		case osg::PrimitiveSet::DrawElementsUShortPrimitiveType:
			pPrimSet = new osg::DrawElementsUShort(Mode);
			break;
		case osg::PrimitiveSet::DrawElementsUIntPrimitiveType:
			pPrimSet = new osg::DrawElementsUInt(Mode);
			break;
	}

	pGeometry->addPrimitiveSet(pPrimSet);

	return pPrimSet;
}

} // using namespace OSGGeometryUtils