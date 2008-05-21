//
// Content3d.cpp
//
// 3D Content Management class.
//
// Copyright (c) 2003-2008 Virtual Terrain Project.
// Free for all uses, see license.txt for details.
//

#include "vtlib/vtlib.h"
#include "vtdata/FilePath.h"
#include "vtdata/vtLog.h"
#include "Content3d.h"

vtItem3d::vtItem3d()
{
	m_pNode = NULL;
}

vtItem3d::~vtItem3d()
{
	// Don't need to explicitly release the item's node here, because all nodes
	//  are released when the the manager's group is released.
	m_pNode = NULL;
}

/**
 * Load the model(s) associated with an item.  If there are several models,
 * generally these are different levels of detail (LOD) for the item.
 */
bool vtItem3d::LoadModels()
{
	VTLOG(" Loading models for item.\n");

	if (m_pNode)
		return true;	// already loaded

	int i, models = NumModels();

	// attempt to instantiate the item
	vtLOD *pLod=NULL;

	if (models > 1)
	{
		pLod = new vtLOD;
		m_pNode = pLod;
	}
	float ranges[20];
	ranges[0] = 0.0f;

	for (i = 0; i < models; i++)
	{
		vtModel *model = GetModel(i);
		vtNode *pNode = NULL;

		// perhaps it's directly resolvable
		pNode = vtNode::LoadModel(model->m_filename);

		// if there are some data path(s) to search, use them
		if (!pNode)
		{
			vtString fullpath = FindFileOnPaths(vtGetDataPath(), model->m_filename);
			if (fullpath != "")
				pNode = vtNode::LoadModel(fullpath);
		}

		if (pNode)
			VTLOG(" Loaded successfully.\n");
		else
		{
			VTLOG(" Couldn't load model from %hs\n",
				(const char *) model->m_filename);
			return false;
		}

		// Shadow control will be higher in the scene graph, so enable it by
		//  default for each loaded node
		pNode->SetCastShadow(true);

		if (model->m_scale != 1.0f)
		{
			// Wrap in a transform node so that we can scale/rotate the node
			vtTransform *pTrans = new vtTransform;
			pTrans->SetName2("scaling xform");
			pTrans->AddChild(pNode);
			pTrans->Identity();
			pTrans->Scale3(model->m_scale, model->m_scale, model->m_scale);
			pNode = pTrans;
		}

		if (models > 1)
			pLod->AddChild(pNode);
		else
			m_pNode = pNode;

		if (models > 1)
			ranges[i+1] = model->m_distance;
	}
	if (models > 1)
		pLod->SetRanges(ranges, models+1);

	// Shadow control is here at the top node in the sub-graph
	m_pNode->SetCastShadow(false);

	return true;
}

//
// An item can store some extents, which give a rough indication of
//  the 2D area taken up by the model, useful for drawing it in traditional
//  2D GIS environments like VTBuilder.
//
//  Whenever a model is added, or the scale factor changes, the extents
//   should be updated.
//
void vtItem3d::UpdateExtents()
{
	m_extents.Empty();

	if (m_pNode == NULL)
		return;

	// A good way to do it would be to try to get a tight bounding box,
	//  but that's non-trivial to compute with OSG.  For now, settle for
	//  making a rectangle from the loose bounding sphere.

	// Both the 3D model and the extents are in approximate meters and
	//  centered on the item's local origin.
	FSphere sph;
	m_pNode->GetBoundSphere(sph);
	m_extents.left = sph.center.x - sph.radius;
	m_extents.right = sph.center.x + sph.radius;

	// However, the XY extents of the extents have Y pointing up, whereas
	//  the world coords have Z pointing down.
	m_extents.top = -sph.center.z + sph.radius;
	m_extents.bottom = -sph.center.z - sph.radius;
}


///////////////////////////////////////////////////////////////////////

vtContentManager3d::vtContentManager3d()
{
	m_pGroup = NULL;
}

void vtContentManager3d::ReleaseContents()
{
	if (m_pGroup)
		m_pGroup->Release();
	m_pGroup = NULL;
}

vtContentManager3d::~vtContentManager3d()
{
	ReleaseContents();
}

vtNode *vtContentManager3d::CreateNodeFromItemname(const char *itemname)
{
	vtItem3d *pItem = (vtItem3d *) FindItemByName(itemname);
	if (!pItem)
		return NULL;

	if (!pItem->m_pNode)
	{
		if (!pItem->LoadModels())
			return NULL;

		if (!m_pGroup)
		{
			m_pGroup = new vtGroup;
			m_pGroup->SetName2("Content Manager Container Group");
		}

		// Add to content container: must keep a reference to each item's
		//  model node so it doesn't get deleted while the manager is alive.
		m_pGroup->AddChild(pItem->m_pNode);
	}
	return pItem->m_pNode;
}

