//
// PagedLodGrid.h
//
// Copyright (c) 2007 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifndef PAGEDLODGRIDH
#define PAGEDLODGRIDH

#include "LodGrid.h"

class vtStructure3d;
class vtStructureArray3d;

/*
 Implementation scene graph:
			A
		   / \
	      B-C B-C
	     /|\  |\
        D D D D D

 A = vtPagedStructureLodGrid, contans an array of cells consisting of:
 B = vtPagedStructureLOD, which is paired with a:
 C = PagedNativeLOD, and has any number of:
 D = vtStructure/vtStructure3d, which produces a vtTransform when built.

 The only part which is OSG-specific is C.
 */
class PagedNativeLOD;

/**
 * An vtPagedStructureLOD node controls the visibility of its child nodes.
 *
 * You set a single distance value (range) for all the children, which
 * is the distance from the camera at which all nodes will be rendered.
 */
class vtPagedStructureLOD : public vtGroup
{
public:
	vtPagedStructureLOD();
	void Release();

	void SetRange(float range) { m_fRange = range; }
	void SetCenter(const FPoint3 &center);
	void GetCenter(FPoint3 &center);
	bool TestVisible(float fDistance, bool bLoad);

	void Add(vtStructure3d *str3d) { m_Structures.Append(str3d); }
	void SetArray(vtStructureArray3d *sa) { m_pStructureArray = sa; }
	void Construct();
	void Deconstruct();
	bool IsConstructed() { return m_bConstructed; }

protected:
	float m_fRange;
	PagedNativeLOD *m_pNativeLOD;
	virtual ~vtPagedStructureLOD() {}

	bool m_bConstructed;
	vtArray<vtStructure3d*> m_Structures;
	vtStructureArray3d *m_pStructureArray;
};

#if VTLIB_OSG
#include "osg/LOD"
class PagedNativeLOD : public osg::LOD
{
public:
	void SetCenter(const FPoint3 &center)
	{
		osg::Vec3 p;
		v2s(center, p);
		setCenter(p);
	}
	void GetCenter(FPoint3 &center)
	{
		s2v(getCenter(), center);
	}
	// Implement OSG's traversal with our own logic
	virtual void traverse(osg::NodeVisitor& nv)
	{
		switch(nv.getTraversalMode())
		{
		case(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN):
			std::for_each(_children.begin(),_children.end(),osg::NodeAcceptOp(nv));
			break;
		case(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN):
			{
				// 'Active' children are those within the given distance
				float distance = nv.getDistanceToEyePoint(getCenter(),true);

				// Get the vtlib node from this OSG node
				vtPagedStructureLOD *vnode = dynamic_cast<vtPagedStructureLOD *>(getUserData());

				// _visitorType might be NODE_VISITOR (in cases such as
				//  intersection testing) or CULL_VISITOR (during rendering).
				//  We only want do visibility testing / page loading during
				//  rendering.

				// Test distance and contruct geometry if needed
				if (vnode->TestVisible(distance,
					nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR))
				{
					// Tell OSG to traverse all children
					std::for_each(_children.begin(),_children.end(),osg::NodeAcceptOp(nv));
				}
				break;
			}
		default:
			break;
		}
	}
};
#else
// dummy
class PagedNativeLOD {
public:
	void SetCenter(const FPoint3 &center) {}
	void GetCenter(FPoint3 &center) {}
};
#endif // VTLIB_OSG

/**
 * vtPagedStructureLodGrid provides a more complex implementation of vtLodGrid.
 *
 * Like vtSimpleLodGrid, it consists of a 2D grid of LOD nodes, each covering
 * a rectangular cell, which causes that cell's children to only be shown
 * when within a given distance.  Additionally, the cells can contain
 * structures (vtStructure) which are not constructed until the cell is
 * visible.
 */
class vtPagedStructureLodGrid : public vtLodGrid
{
public:
	vtPagedStructureLodGrid();
	void Setup(const FPoint3 &origin, const FPoint3 &size,
		int iDimension, float fLODDistance, vtHeightField3d *pHF = NULL);
	void Release();

	// methods
	void SetDistance(float fLODDistance);
	void SetArray(vtStructureArray3d *sa) { m_pStructureArray = sa; }
	bool AppendToGrid(vtStructure *str, vtStructure3d *str3d);
	void DeleteFarawayStructures(const FPoint3 &CamPos,
		int iMaxStructures, float fDistance);

protected:
	vtStructureArray3d *m_pStructureArray;
	vtPagedStructureLOD **m_pCells;

	vtGroup *FindCellParent(const FPoint3 &point);
	vtPagedStructureLOD *FindPagedCellParent(const FPoint3 &point);
	void AllocateCell(int a, int b);
	vtGroup *GetCell(int a, int b);
};

#endif // PAGEDLODGRIDH
