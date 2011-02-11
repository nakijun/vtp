//
// NodeOSG.cpp
//
// Encapsulate behavior for OSG scene graph nodes.
//
// Copyright (c) 2001-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#include "vtlib/vtlib.h"
#include "vtdata/vtLog.h"
#if VTLISPSM
#include "LightSpacePerspectiveShadowTechnique.h"
#endif
#include "SimpleInterimShadowTechnique.h"

#include <osg/Polytope>
#include <osg/Projection>
#include <osg/Depth>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>
#include <osgUtil/Optimizer>
#include <osg/Version>
#include <osg/TexGen>
#include <osgParticle/ModularEmitter>
#include <osgParticle/ParticleSystemUpdater>
#include <osgShadow/ShadowMap>
#include <osgShadow/ShadowTexture>

using namespace osg;

#define DEBUG_NODE_LOAD	0

// We use bits 1 and 2 of the node mask as shadow flags.
const int ReceivesShadowTraversalMask = 0x1;
const int CastsShadowTraversalMask = 0x2;


///////////////////////////////////////////////////////////////////////
// vtNode
//

vtNode::vtNode()
{
	m_bCastShadow = true; // osg nodes default to all mask bits set
						  // so set this true to match
}

/**
 * Releases a node.  Use this method instead of C++'s delete operator when
 * you are done with a node.  Internally, the node is reference counted so
 * it is not deleted until all references to it are removed.
 */
void vtNode::Release()
{
	// shouldn't happen but... safety check anyway
	if (m_pNode == NULL)
		return;

	if (m_pNode->referenceCount() == 1)
	{
#if DEBUG_NODE_LOAD
		VTLOG("Deleting vtNode: %lx (\"%s\") (osg %lx, rc %d", this,
			m_pNode->getName().c_str(), m_pNode, m_pNode->referenceCount());
		osg::Group *cg = dynamic_cast<osg::Group*>(m_pNode.get());	// container group
		if (cg)
		{
			unsigned int nc = cg->getNumChildren();
			VTLOG(", children %d", nc);
			if (nc)
			{
				osg::Node *child = cg->getChild(0);
				VTLOG(", child0 %lx", child);
			}
		}
		VTLOG1(")\n");
#endif
		// Tell OSG that we're through with this node.
		// The following statement calls unref() on m_pNode, which deletes the
		//  OSG node, which decrements its reference to us, which deletes us.
		m_pNode = NULL;
	}
}

void vtNode::SetOsgNode(osg::Node *n)
{
	// set refptr to the OSG node, which bumps its refcount
	m_pNode = n;

	if (m_pNode.valid())
	{
		// set its user data back to use, which bumps our refcount
		m_pNode->setUserData((vtNode *)this);
	}
}

/**
 * Set the enabled state of this node.  When the node is not enabled, it
 * is not rendered.  If it is a group node, all of the nodes children are
 * also not rendered.
 */
void vtNode::SetEnabled(bool bOn)
{
	// OSG controls traversal of the scene node tree using
	// a traversal mask which the node visitor applies to the inappropriately named
	// NodeMask of a node. NodeFlagBits would probably be a more descriptive name.
	// The traversal mask is applied using a bitwise and (& operator) and a none zero result will
	// cause the node to be traversed. A zero result will cause the node and all its children
	// to be skipped. The important consequence of this is that all the unmasked bits need to be unset
	// for the node to "disabled" or skipped. This means that when we disable a node and subsequently
	// enable it we need to have the correct information to set any bits that we set to zero back
	// to one. i.e. we have to have sufficent state elsewhere in the node to do this.
	//
	// We use the least significant two bits of the node mask to
	// control both the node enabled state and the shadow rendering.
	// At the moment all the nodes in our tree receive shadows so we dont have
	// to separately store the "receive shadows" bit, we just unconditionally turn it on
	// when we enable the node. Not all nodes cast shadows so we have to store the required state
	// of that bit elsewhere in the node so that we can set it to the correct state when we enable the node.
	// If we ever decide to have some nodes not receiving shadows (and the osg shadowers implement this)
	// we will need to store the state of the "receive shadows" bit as well. We will also have to use another
	// bit in the NodeMask(NodeFlagBits!) to cover the case for enabled nodes that neither cast nor receive shadows.
	// I will leave you to work out why we do not need to store the state of this bit :-)
	//
	// For the time being valid values for the bottom bits are
	// 0x0 Node is disabled
	// 0x1 Node is enabled and will receive shadows
	// 0x3 Node is enabled and will cast and receive shadows
	osg::Node::NodeMask nm = m_pNode->getNodeMask();
	if (bOn)
	{
		if (m_bCastShadow)
			m_pNode->setNodeMask(nm | 3);
		else
			m_pNode->setNodeMask(nm & ~3 | 1);
	}
	else
		m_pNode->setNodeMask(nm & ~3);
}

/**
 * Return the enabled state of a node.
 */
bool vtNode::GetEnabled() const
{
	int mask = m_pNode->getNodeMask();
	return ((mask&0x3) != 0);
}

void vtNode::SetName2(const char *name)
{
	if (m_pNode != NULL)
		m_pNode->setName((char *)name);
}

const char *vtNode::GetName2() const
{
	if (m_pNode != NULL)
		return m_pNode->getName().c_str();
	else
		return NULL;
}

//////////////////////////////////////////////////////////////////////////////
// class ExtentsVisitor
//
// description: visit all nodes and compute bounding box extents
//
class ExtentsVisitor : public osg::NodeVisitor
{
public:
	// constructor
	ExtentsVisitor():NodeVisitor(NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
	// destructor
	~ExtentsVisitor() {}

	virtual void apply(osg::Geode &node)
	{
		// update bounding box
		osg::BoundingBox bb;
		for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
		{
			// expand overall bounding box
			bb.expandBy(node.getDrawable(i)->getBound());
		}

		// transform corners by current matrix
		osg::BoundingBox xbb;
		for (unsigned int i = 0; i < 8; ++i)
		{
			osg::Vec3 xv = bb.corner(i) * m_TransformMatrix;
			xbb.expandBy(xv);
		}

		// update overall bounding box size
		m_BoundingBox.expandBy(xbb);

		// continue traversing the graph
		traverse(node);
	}
	// handle geode drawable extents to expand the box
	virtual void apply(osg::MatrixTransform &node)
	{
		m_TransformMatrix *= node.getMatrix();
		// continue traversing the graph
		traverse(node);
	}
	// handle transform to expand bounding box
	// return bounding box
	osg::BoundingBox &GetBound() { return m_BoundingBox; }

protected:
	osg::BoundingBox m_BoundingBox;	// bound box
	osg::Matrix m_TransformMatrix;	// current transform matrix
};


/**
 * Calculates the bounding box of the geometry contained in and under
 * this node in the scene graph.  Note that unlike bounding sphere which
 * is cached, this value is calculated every time.
 *
 * \param box Will receive the bounding box.
 */
void vtNode::GetBoundBox(FBox3 &box)
{
	ExtentsVisitor ev;
	m_pNode->accept(ev);
	osg::BoundingBox extents = ev.GetBound();
	s2v(extents, box);
}

void vtNode::GetBoundSphere(FSphere &sphere, bool bGlobal)
{
	// Try to just get the bounds normally
	BoundingSphere bs = m_pNode->getBound();

	// Hack to support particle systems, which do not return a reliably
	//  correct bounding sphere, because of the extra transform inside which
	//  provides the particle effects with an absolute position
	if (ContainsParticleSystem())
	{
		osg::Group *grp = dynamic_cast<osg::Group*>(m_pNode.get());
		for (int i = 0; i < 3; i++)
		{
			grp = dynamic_cast<osg::Group*>(grp->getChild(0));
			if (grp)
				bs = grp->getBound();
			else
				break;
		}
	}

	s2v(bs, sphere);
	if (bGlobal)
	{
		// Note that this isn't 100% complete; we should be
		//  transforming the radius as well, with scale.
		LocalToWorld(sphere.center);
	}
}

class PolygonCountVisitor : public osg::NodeVisitor
{
public:
	int numVertices, numFaces, numObjects;
	PolygonCountVisitor() :
		osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
	{
		reset();
		memset(&info, 0, sizeof(info));
	}
	virtual void reset() { numVertices=numFaces=numObjects=0; }
	virtual void apply(osg::Geode& geode);
	vtPrimInfo info;
};

void PolygonCountVisitor::apply(osg::Geode& geode)
{
	numObjects++;
	for (unsigned int i=0; i<geode.getNumDrawables(); ++i)
	{
		osg::Geometry* geometry = geode.getDrawable(i)->asGeometry();
		if (!geometry) continue;
		for (unsigned int j=0; j<geometry->getPrimitiveSetList().size(); ++j)
		{
			osg::PrimitiveSet *pset = geometry->getPrimitiveSet(j);
			osg::DrawArrayLengths *dal = dynamic_cast<osg::DrawArrayLengths*>(pset);
			//osg::DrawArray *da = dynamic_cast<osg::DrawArray*>(pset);

			int numIndices = pset->getNumIndices();
			int numPrimitives = pset->getNumPrimitives();

			info.Vertices += numIndices;
			info.Primitives += numPrimitives;

			// This code isn't completely finished as it doesn't iterate down
			//  into each variable-length indexed primitives to count each
			//  component line or triangle, but it does get most useful info.
			GLenum mode = pset->getMode();
			switch (mode)
			{
			case GL_POINTS:			info.Points += numPrimitives;	 break;
			case GL_LINES:			info.LineSegments += numPrimitives; break;
			case GL_TRIANGLES:		info.Triangles += numPrimitives; break;
			case GL_QUADS:			info.Quads += numPrimitives;
									info.Triangles += numPrimitives*2; break;
			case GL_POLYGON:		info.Polygons += numPrimitives;
									//info.Triangles += ...;
									break;
			case GL_LINE_STRIP:		info.LineStrips += numPrimitives;
									//info.LineSegments += ...;
									break;
			case GL_TRIANGLE_STRIP:	info.TriStrips += numPrimitives;
									//info.Triangles += ...;
									break;
			case GL_TRIANGLE_FAN:	info.TriFans += numPrimitives;
									//info.Triangles += ...;
									break;
			case GL_QUAD_STRIP:		info.QuadStrips += numPrimitives;
									//info.Quads += ...;
									//info.Triangles += ...;
									break;
			}
			// This kind of thing is incomplete because pset can be something
			//  called 'osg::DrawElementsUShort'
			if (mode == GL_TRIANGLE_STRIP && dal != NULL)
			{
				for (osg::DrawArrayLengths::const_iterator itr=dal->begin(); itr!=dal->end(); ++itr)
				{
					int iIndicesInThisStrip = static_cast<int>(*itr);
					info.Triangles += (iIndicesInThisStrip-1);
				}
			}
		}
		info.MemVertices += geometry->getVertexArray()->getNumElements();
	}
	traverse(geode);
}

/**
 * This method walks through a node (and all its children), collecting
 * information about all the geometric primitives.  The result is placed
 * in an object of type vtPrimInfo.  This includes information such as
 * number of vertices, number of triangles, and so forth.  Note that this
 * can be a time-consuming operation if your geometry is large or complex.
 * The results are not cached, so this method should be called only when
 * needed.
 *
 * \param info A vtPrimInfo object which will receive all the information
 *	about this node and its children.
 */
void vtNode::GetPrimCounts(vtPrimInfo &info)
{
	PolygonCountVisitor pv;
	m_pNode->accept(pv);
	info = pv.info;
}

/**
 * Transform a 3D point from a node's local frame of reference to world
 * coordinates.  This is done by walking the scene graph upwards, applying
 * all transforms that are encountered.
 *
 * \param point A reference to the input point is modified in-place with
 *	world coordinate result.
 */
void vtNode::LocalToWorld(FPoint3 &point)
{
#if 0
	// Work our way up the tree to the root, accumulating the transforms,
	//  to get the point in the world reference frame.
	// This code uses VTLIB calls and works find on a purely VTLIB graph.
	vtNode *node = this;
	while (node = node->GetParent(0))
	{
		vtTransform *trans = dynamic_cast<vtTransform *>(node);
		if (trans)
		{
			FMatrix4 mat;
			trans->GetTransform1(mat);
			FPoint3 result;
			mat.Transform(point, result);
			point = result;
		}
	}
#else
	// We must use OSG native calls instead if we want to support raw OSG
	//  nodes which might be returned from collision detection
	osg::Vec3 pos = v2s(point);
	osg::Node *node = GetOsgNode();
	while (node = node->getParent(0))
	{
		osg::MatrixTransform *mt = dynamic_cast<osg::MatrixTransform *>(node);
		if (mt != NULL)
		{
			const osg::Matrix &mat = mt->getMatrix();
			pos = mat.preMult(pos);
		}
		if (node->getNumParents() == 0)
			break;
	}
	s2v(pos, point);
#endif
}

/**
 * Return the parent of this node in the scene graph.  If the node is not
 * in the scene graph, NULL is returned.
 *
 * \param iParent If the node has multiple parents, you can specify which
 *	one you want.
 */
vtGroup *vtNode::GetParent(int iParent)
{
	int num = m_pNode->getNumParents();
	if (iParent >= num)
		return NULL;
	osg::Group *parent = m_pNode->getParent(iParent);
	if (!parent)
		return NULL;

	vtGroup *pGroup =  dynamic_cast<vtGroup *>(parent->getUserData());
	if (NULL == pGroup)
	{
		pGroup = new vtGroup(true);
		pGroup->SetOsgGroup(parent);
	}
	return pGroup;
}

vtNode *vtNode::Clone(bool bDeep)
{
	// We should never get here, because only subclasses of vtNode are
	//  ever instantiated.
	return NULL;
}

class CFindNodeVisitor : public osg::NodeVisitor
{
public:
	CFindNodeVisitor(const char *pName, osg::NodeVisitor::TraversalMode Mode = osg::NodeVisitor::TRAVERSE_ALL_CHILDREN):
	osg::NodeVisitor(Mode)
	{
		m_Name = pName;
		_nodeMaskOverride = 0xffffffff;
	}

	virtual void apply(osg::Node& node)
	{
		if (node.getName().find(m_Name) != std::string::npos)
		{
			m_pNode = &node;

			_traversalMode = TRAVERSE_NONE;
			return;
		}
		traverse(node);
	}

	osg::ref_ptr<osg::Node> m_pNode;
	std::string m_Name;
};

// Find and decorate a native node
// To allow subsequent graph following from the VT side
// return osg::Group as vtGroup
vtNode *vtNode::FindNativeNode(const char *pName, bool bDescend)
{
	vtNode *pVTNode = NULL;
	osg::Group *pGroup = dynamic_cast<osg::Group*>(m_pNode.get());
	if (NULL == pGroup)
		return NULL;
	osg::ref_ptr<osg::Node> pNode;
	osg::ref_ptr<CFindNodeVisitor> pFNVisitor = new CFindNodeVisitor(pName,
																	bDescend ?
																	osg::NodeVisitor::TRAVERSE_ALL_CHILDREN :
																	osg::NodeVisitor::TRAVERSE_NONE);
	if (bDescend)
		pGroup->accept(*pFNVisitor.get());
	else
		pGroup->traverse(*pFNVisitor.get());
	pNode = pFNVisitor->m_pNode.get();

	if (!pNode.valid())
		return NULL;

	pVTNode = (vtNode*)pNode->getUserData();
	if (NULL != pVTNode)
		return pVTNode;

	// Now we know it's an OSG node that's not already wrapped.
	// Maybe it's a transform.
	osg::MatrixTransform *mt = dynamic_cast<osg::MatrixTransform*>(pNode.get());
	if (mt)
	{
		vtTransform *pVTXform = new vtTransform(mt);
		return pVTXform;
	}

	// Maybe it's a group.
	osg::Group *grp = dynamic_cast<osg::Group*>(pNode.get());
	if (grp)
	{
		vtGroup *pVTGroup = new vtGroup(true);
		pVTGroup->SetOsgGroup(grp);
		return pVTGroup;
	}

	// Otherwise we have to use the opaque wrapper "Native Node"
	return new vtNativeNode(pNode.get());
}

//
// This visitor looks in a node tree for any instance of ModularEmitter.
//
#include <osgParticle/ModularEmitter>
class findParticlesVisitor : public osg::NodeVisitor
{
public:
	findParticlesVisitor() : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN) {
		bFound = false;
	}
	virtual void apply(osg::Node &searchNode) {
		if (osgParticle::ModularEmitter* me = dynamic_cast<osgParticle::ModularEmitter*> (&searchNode) )
			bFound = true;
		else
			traverse(searchNode);
	}
	bool bFound;
};

bool vtNode::ContainsParticleSystem() const
{
	osg::ref_ptr<findParticlesVisitor> fp = new findParticlesVisitor;
	findParticlesVisitor *fpp = fp.get();
	m_pNode->accept(*fpp);
	return fp->bFound;
}


// Walk an OSG scenegraph looking for Texture states, and disable mipmap.
class MipmapVisitor : public NodeVisitor
{
public:
	MipmapVisitor() : NodeVisitor(NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
	virtual void apply(osg::Geode& geode)
	{
		for (unsigned i=0; i<geode.getNumDrawables(); ++i)
		{
			osg::Geometry *geo = dynamic_cast<osg::Geometry *>(geode.getDrawable(i));
			if (!geo) continue;

			StateSet *stateset = geo->getStateSet();
			if (!stateset) continue;

			StateAttribute *state = stateset->getTextureAttribute(0, StateAttribute::TEXTURE);
			if (!state) continue;

			Texture2D *texture = dynamic_cast<Texture2D *>(state);
			if (texture)
				texture->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
		}
		NodeVisitor::apply(geode);
	}
};

// Walk an OSG scenegraph looking for geodes with textures that have an alpha
//	map, and enable alpha blending for them.  I cannot imagine why this is not
//  the default OSG behavior, but since it isn't, we have this visitor.
class AlphaVisitor : public NodeVisitor
{
public:
	AlphaVisitor() : NodeVisitor(NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
	virtual void apply(osg::Geode& geode)
	{
		for (unsigned i=0; i<geode.getNumDrawables(); ++i)
		{
			osg::Geometry *geo = dynamic_cast<osg::Geometry *>(geode.getDrawable(i));
			if (!geo) continue;

			StateSet *stateset = geo->getStateSet();
			if (!stateset) continue;

			StateAttribute *state = stateset->getTextureAttribute(0, StateAttribute::TEXTURE);
			if (!state) continue;

			Texture2D *texture = dynamic_cast<Texture2D *>(state);
			if (!texture) continue;

			Image *image = texture->getImage();
			if (!image) continue;

			if (image->isImageTranslucent())
			{
				stateset->setAttributeAndModes(new BlendFunc, StateAttribute::ON);
				stateset->setRenderingHint(StateSet::TRANSPARENT_BIN);
			}
		}
		NodeVisitor::apply(geode);
	}
};

// Our own cache of models loaded from OSG
typedef std::map< vtString, osg::ref_ptr<Node> > NodeCache;
NodeCache m_ModelCache;
bool vtNode::s_bDisableMipmaps = false;
osg::Node *(*s_NodeCallback)(osg::Transform *input) = NULL;

void SetLoadModelCallback(osg::Node *callback(osg::Transform *input))
{
	s_NodeCallback = callback;
}

/**
 * Load a 3D model from a file.
 *
 * The underlying scenegraph (e.g. OSG) is used to load the model, which is
 * returned as a vtNode.  You can then use this node normally, for example
 * add it to your scenegraph with vtGroup::AddChild(), or to your terrain's
 * subgraph with vtTerrain::AddNode().
 *
 * \param filename The filename to load from.
 * \param bAllowCache Default is true, to allow vtosg to cache models.
 *	This means that if you load from the same filename more than once, you
 *  will get the same model again instantly.  If you don't want this, for
 *	example if the model has changed on disk and you want to force loading,
 *	pass false.
 * \param bDisableMipmaps Pass true to turn off mipmapping in the texture maps
 *	in the loaded model.  Default is false (enable mipmapping).
 *
 * \return A node pointer if successful, or NULL if the load failed.
 */
vtNode *vtNode::LoadModel(const char *filename, bool bAllowCache,
						  bool bDisableMipmaps)
{
	// Some of OSG's file readers, such as the Wavefront OBJ reader, have
	//  sensitivity to stdio issues with '.' and ',' in European locales.
	LocaleWrap normal_numbers(LC_NUMERIC, "C");

	// Workaround for OSG's OBJ-MTL reader which doesn't like backslashes
	vtString fname = filename;
	for (int i = 0; i < fname.GetLength(); i++)
	{
		if (fname.GetAt(i) == '\\') fname.SetAt(i, '/');
	}

	// We must track whether we have loaded this object already; that is, whether
	//  it is in the OSG object cache.  If it is in the cache, then we musn't
	//  apply the rotation below, because it's already been applied to the
	//  one in the cache that we've gotten again.
	Node *node, *existing_node = NULL;
	bool bInCache = (m_ModelCache.count(fname) != 0);
	if (bInCache)
		existing_node = m_ModelCache[fname].get();

	bool bDoLoad = (!bInCache || !bAllowCache);
	if (bDoLoad)
	{
#define HINT osgDB::ReaderWriter::Options::CacheHintOptions
		// In case of reloading a previously loaded model, we must empty
		//  our own cache as well as disable OSG's cache.
		osgDB::Registry *reg = osgDB::Registry::instance();
		osgDB::ReaderWriter::Options *opts;

		opts = reg->getOptions();
		if (!opts)
		{
			opts = new osgDB::ReaderWriter::Options;
			opts->ref();	// workaround!  otherwise OSG might crash when
				// closing its DLL, as the options get deleted twice (?) or
				// perhaps it doesn't like deleting the object WE allocated.
		}
		// Always disable OSG's cache, we don't use it.
		opts->setObjectCacheHint((HINT) ((opts->getObjectCacheHint() & ~(osgDB::ReaderWriter::Options::CACHE_NODES))));
		reg->setOptions(opts);

		// OSG doesn't yet support utf-8 or wide filenames, so convert
		vtString fname_local = UTF8ToLocal(fname);

		// Now actually request the node from OSG
#if VTDEBUG
		VTLOG("[");
#endif
		node = osgDB::readNodeFile((const char *)fname_local);
#if VTDEBUG
		VTLOG("]");
#endif
		// If OSG could not load it, there is nothing more to do
		if (!node)
			return NULL;
	}
	else
	{
		// Simple case: use cached node
		node = existing_node;
	}

#if DEBUG_NODE_LOAD
	VTLOG("LoadModel: osg raw node %lx (rc %d), ", node, node->referenceCount());
#endif

	if (bDoLoad)
	{
		// We must insert a 'Normalize' state above the geometry objects
		// that we load, otherwise when they are scaled, the vertex normals
		// will cause strange lighting.  Fortunately, we only need to create
		// a single State object which is shared by all loaded models.
		StateSet *stateset = node->getOrCreateStateSet();
		stateset->setMode(GL_NORMALIZE, StateAttribute::ON);

		// For some reason, some file readers (at least .obj) will load models with
		//  alpha textures, but _not_ enable blending for them or put them in the
		//  transparent bin.  This visitor walks the tree and corrects that.
		AlphaVisitor visitor_a;
		node->accept(visitor_a);

		// If the user wants to, we can disable mipmaps at this point, using
		//  another visitor.
		if (bDisableMipmaps || s_bDisableMipmaps)
		{
			MipmapVisitor visitor;
			node->accept(visitor);
		}

		// We must insert a rotation transform above the model, because OSG's
		//  file loaders (now mostly consistently) tweak the model to put Z
		//  up, and the VTP uses OpenGL coordinates which has Y up.
		float fRotation = -PID2f;

		// OSG expects OBJ models to have Y up.  I have seen models with Z
		//  up, and we used to correct for that here (fRotation = -PIf).
		//  However, over time it has appeared that there are more OBJ out
		//  there with Y up than with Z up.  So, we now treat all models from
		//  OSG the same.

		osg::MatrixTransform *transform = new osg::MatrixTransform;
		transform->setName("corrective 90 degrees");
		transform->setMatrix(osg::Matrix::rotate(fRotation, Vec3(1,0,0)));
		transform->addChild(node);
		// it's not going to change, so tell OSG that it can be optimized
		transform->setDataVariance(osg::Object::STATIC);

		if (s_NodeCallback == NULL)
		{
#if 0
			// Now do some OSG voodoo, which should spread ("flatten") the
			//  transform downward through the loaded model, and delete the transform.
			// In practice, i find that it doesn't actually do any flattening.
			osg::Group *group = new osg::Group;
			group->addChild(transform);

			osgUtil::Optimizer optimizer;
			optimizer.optimize(group, osgUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS);
			node = group;
#else
			node = transform;
#endif
		}
		else
		{
			osg::Node *node_new = s_NodeCallback(transform);
			node = node_new;
		}
		//VTLOG1("--------------\n");
		//vtLogNativeGraph(node);

		// Store the node in the cache by filename so we'll know next
		//  time that we have already have it
		m_ModelCache[fname] = node;
	}

	// Since there must be a 1-1 correspondence between VTP nodes
	//  at OSG nodes, we can't have multiple VTP nodes sharing a
	//  single OSG node.  So, we can't simply use the OSG node ptr
	//  that we get: we must wrap it.
	osg::Group *container_group = new osg::Group;
	container_group->addChild(node);

	// The final resulting node is the container of that operation
	vtNativeNode *pNode = new vtNativeNode(container_group);

	// Use the filename as the node's name
	pNode->SetName2(fname);

#if DEBUG_NODE_LOAD
	VTLOG("node %lx (rc %d),\n ", node, node->referenceCount());
	VTLOG("container %lx (rc %d, nc %d), ", container_group, container_group->referenceCount(), container_group->getNumChildren());
	VTLOG("VTP node %lx (rc %d)\n", pNode, pNode->referenceCount());
#endif

	return pNode;
}

/**
 * Given a node with geometry under it, rotate the vertices of that geometry
 *  by a given axis/angle.
 */
void vtNode::ApplyVertexRotation(const FPoint3 &axis, float angle)
{
	FMatrix4 mat;
	mat.AxisAngle(axis, angle);
	ApplyVertexTransform(mat);
}

/**
 * Given any node with geometry under it, transform the vertices of that geometry.
 */
void vtNode::ApplyVertexTransform(const FMatrix4 &mat)
{
#if 0
	vtGroup *vtgroup = dynamic_cast<vtGroup*>(this);
	if (!vtgroup)
		return;
	osg::Group *container_group = vtgroup->GetOsgGroup();
	osg::Node *unique_node = container_group->getChild(0);
	if (!unique_node)
		return;
	osg::Group *unique_group = dynamic_cast<osg::Group*>(unique_node);
	if (!unique_group)
		return;
	osg::Node *node = unique_group->getChild(0);
#else
	osg::Node *node = GetOsgNode();
	osg::ref_ptr<osg::Group> temp = new osg::Group;
#endif
	if (!node)
		return;

	osg::Matrix omat;
	ConvertMatrix4(&mat, &omat);

	osg::MatrixTransform *transform = new osg::MatrixTransform;
	transform->setMatrix(omat);
	// it's not going to change, so tell OSG that it can be optimized
	transform->setDataVariance(osg::Object::STATIC);

	node->ref();	// avoid losing this node
#if 0
	unique_group->removeChild(node);
	unique_group->addChild(transform);
#endif
	temp->addChild(transform);
	transform->addChild(node);
	node->unref();

	// Now do some OSG voodoo, which should spread the transform downward
	//  through the loaded model, and delete the transform.
	//
	// NOTE: OSG 1.0 seems to have a bug (limitation): Optimizer doesn't
	//  inform the display lists that they have changed.  So, this doesn't
	//  produce a visual update for objects which have already been rendered.
	// It is a one-line fix in Optimizer.cpp (CollectLowestTransformsVisitor::doTransform)
	// I wrote the OSG list with the fix on 2006.04.12.
	//
	osgUtil::Optimizer optimizer;
	optimizer.optimize(temp.get(), osgUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS);
}

void vtNode::ClearOsgModelCache()
{
#if DEBUG_NODE_LOAD
	VTLOG1("Clearing OSG Model Cache.  Contents:\n");
	for (NodeCache::iterator iter = m_ModelCache.begin();
		iter != m_ModelCache.end(); iter++)
	{
		vtString str = iter->first;
		osg::Node *node = iter->second.get();
		VTLOG("  Model '%s', node %lx (rc %d, parents %d)\n", (const char *) str,
			node, node->referenceCount(), node->getNumParents());
	}
#endif
	// Each model in the cache, at exit time, should have a refcount
	//  of 1.  Deleting the cache will push them to 0 and delete them.
	m_ModelCache.clear();
}

void DecorateVisit(osg::Node *node)
{
	if (!node) return;

	vtNode *vnode = NULL;
	osg::Group *group = NULL;
	vtGroup *vgroup = NULL;

	// first, determine if this node is already decorated
	vnode = (vtNode *) (node->getUserData());
	group = dynamic_cast<osg::Group*>(node);
	if (!vnode)
	{
		// needs decorating.  it is a group?
		if (group)
		{
			vgroup = new vtGroup(true);
			vgroup->SetOsgGroup(group);
			vnode = vgroup;
		}
		else
		{
			// decorate as plain native node
			vnode = new vtNativeNode(node);
		}
	}
	if (group)
	{
		for (unsigned int i = 0; i < group->getNumChildren(); i++)
			DecorateVisit(group->getChild(i));
	}
}

void vtNode::DecorateNativeGraph()
{
	DecorateVisit(m_pNode.get());
}

vtMultiTexture *vtNode::AddMultiTexture(int iTextureUnit, vtImage *pImage, int iTextureMode,
										const FPoint2 &scale, const FPoint2 &offset)
{
	vtMultiTexture *mt = new vtMultiTexture;
	mt->m_pNode = this;
	mt->m_iTextureUnit = iTextureUnit;
#if VTLISPSM
	mt->m_iMode = iTextureMode;
#endif

	// Currently, multi-texture support is OSG-only
	osg::Node *onode = GetOsgNode();

	mt->m_pTexture = new osg::Texture2D(pImage->GetOsgImage());

	mt->m_pTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
	mt->m_pTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
	mt->m_pTexture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_BORDER);
	mt->m_pTexture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_BORDER);

	// Set up the texgen
	osg::ref_ptr<osg::TexGen> pTexgen = new osg::TexGen;
	pTexgen->setMode(osg::TexGen::EYE_LINEAR);
	pTexgen->setPlane(osg::TexGen::S, osg::Vec4(scale.x, 0.0f, 0.0f, -offset.x));
	pTexgen->setPlane(osg::TexGen::T, osg::Vec4(0.0f, 0.0f, scale.y, -offset.y));

	osg::TexEnv::Mode mode;
	if (iTextureMode == GL_ADD) mode = osg::TexEnv::ADD;
	if (iTextureMode == GL_BLEND) mode = osg::TexEnv::BLEND;
	if (iTextureMode == GL_REPLACE) mode = osg::TexEnv::REPLACE;
	if (iTextureMode == GL_MODULATE) mode = osg::TexEnv::MODULATE;
	if (iTextureMode == GL_DECAL) mode = osg::TexEnv::DECAL;
	osg::ref_ptr<osg::TexEnv> pTexEnv = new osg::TexEnv(mode);

	// Apply state
	osg::ref_ptr<osg::StateSet> pStateSet = onode->getOrCreateStateSet();

	pStateSet->setTextureAttributeAndModes(iTextureUnit, mt->m_pTexture.get(), osg::StateAttribute::ON);
	pStateSet->setTextureAttributeAndModes(iTextureUnit, pTexgen.get(), osg::StateAttribute::ON);
	pStateSet->setTextureMode(iTextureUnit, GL_TEXTURE_GEN_S,  osg::StateAttribute::ON);
	pStateSet->setTextureMode(iTextureUnit, GL_TEXTURE_GEN_T,  osg::StateAttribute::ON);
	pStateSet->setTextureAttributeAndModes(iTextureUnit, pTexEnv.get(), osg::StateAttribute::ON);

	// If texture mode is DECAL and intenal texture format does not have an alpha channel then
	// force the format to be converted on texture binding
	if ((GL_DECAL == iTextureMode) &&
		(pImage->GetOsgImage()->getInternalTextureFormat() != GL_RGBA))
	{
		// Force the internal format to RGBA
		pImage->GetOsgImage()->setInternalTextureFormat(GL_RGBA);
	}

	return mt;
}

void vtNode::EnableMultiTexture(vtMultiTexture *mt, bool bEnable)
{
	osg::Node *onode = GetOsgNode();
	osg::ref_ptr<osg::StateSet> pStateSet = onode->getOrCreateStateSet();
	if (bEnable)
		pStateSet->setTextureAttributeAndModes(mt->m_iTextureUnit, mt->m_pTexture.get(), osg::StateAttribute::ON);
	else
	{
		osg::StateAttribute *attr = pStateSet->getTextureAttribute(mt->m_iTextureUnit, osg::StateAttribute::TEXTURE);
		if (attr != NULL)
			pStateSet->removeTextureAttribute(mt->m_iTextureUnit, attr);
	}
}

bool vtNode::MultiTextureIsEnabled(vtMultiTexture *mt)
{
	osg::Node *onode = GetOsgNode();
	osg::ref_ptr<osg::StateSet> pStateSet = onode->getOrCreateStateSet();
	osg::StateAttribute *attr = pStateSet->getTextureAttribute(mt->m_iTextureUnit, osg::StateAttribute::TEXTURE);
	return (attr != NULL);
}

void vtNode::SetCastShadow(bool b)
{
	m_bCastShadow = b;
	if (GetEnabled())
		SetEnabled(true);
}

bool vtNode::GetCastShadow()
{
	return m_bCastShadow;
}

///////////////////////////////////////////////////////////////////////

vtNativeNode::vtNativeNode(osg::Node *node)
{
	SetOsgNode(node);
}

vtNode *vtNativeNode::Clone(bool bDeep)
{
	if (bDeep)
	{
		// 'Deep' copies all the nodes, but not the geometry inside them
		osg::CopyOp deep_op(osg::CopyOp::DEEP_COPY_OBJECTS |
							osg::CopyOp::DEEP_COPY_NODES |
							osg::CopyOp::DEEP_COPY_DRAWABLES |
							osg::CopyOp::DEEP_COPY_STATESETS);
		osg::Node *newnode = (osg::Node *) m_pNode->clone(deep_op);
		return new vtNativeNode(newnode);
	}
	else
		// A shallow copy of a native node is just a second reference
		return this;
}

vtNode *vtNativeNode::FindParentVTNode()
{
	osg::Node *node = GetOsgNode();
	while (node)
	{
		// safety check: handle the case in which moving upwards leads nowhere
		int parents = node->getNumParents();
		if (parents == 0)
			return NULL;

		node = node->getParent(0);
		osg::Referenced *ref = node->getUserData();
		if (ref)
		{
			vtNode *vnode = dynamic_cast<vtNode *>(ref);
			if (vnode)
				return vnode;
		}
	}
	return NULL;
}


///////////////////////////////////////////////////////////////////////
// vtGroup
//

vtGroup::vtGroup(bool suppress) : vtNode(), vtGroupBase()
{
	if (!suppress)
	{
		m_pGroup = new Group;
		SetOsgNode(m_pGroup);
	}
}

void vtGroup::SetOsgGroup(osg::Group *g)
{
	m_pGroup = g;
	SetOsgNode(g);
}

vtNode *vtGroup::Clone(bool bDeep)
{
	vtGroup *newgroup = new vtGroup;
	newgroup->CloneFrom(this, bDeep);
	return newgroup;
}

void vtGroup::CloneFrom(vtGroup *group, bool bDeep)
{
	if (bDeep)
	{
		osg::CopyOp deep_op(osg::CopyOp::DEEP_COPY_OBJECTS | osg::CopyOp::DEEP_COPY_NODES);

		// Deep copy: Duplicate the entire tree of nodes
		for (unsigned int i = 0; i < group->GetNumChildren(); i++)
		{
			vtNode *child = group->GetChild(i);
			if (child)
				AddChild(child->Clone(bDeep));
			else
			{
				// Might be an internal (native OSG) node - try to cope with that
				Node *pOsgChild = const_cast<Node *>( group->m_pGroup->getChild(i) );
				if (pOsgChild)
				{
					osg::Node *newnode = (osg::Node *) pOsgChild->clone(deep_op);
					m_pGroup->addChild(newnode);
				}
			}
		}
	}
	else
	{
		// Shallow copy: Add another reference to the existing children.
		for (unsigned int i = 0; i < group->GetNumChildren(); i++)
		{
			vtNode *child = group->GetChild(i);
			if (child)
				AddChild(child);
			else
			{
				// Might be an internal (native OSG) node - try to cope with that
				Node *pOsgChild = const_cast<Node *>( group->m_pGroup->getChild(i) );
				if (pOsgChild)
					m_pGroup->addChild(pOsgChild);
			}
		}
	}
}

void vtGroup::Release()
{
	// Check if there are no more external references to this group node.
	// If so, clean up the VTP side of the scene graph.
	if (m_pNode->referenceCount() == 1)
	{
		// it's over for this node, start the destruction process
		// Release children depth-first
		int children = GetNumChildren();
		vtNode *pChild;

		for (int i = 0; i < children; i++)
		{
			if (NULL == (pChild = GetChild(0)))
			{
				// Probably a raw osg node Group, access it directly.
				Node *node = m_pGroup->getChild(0);
				// This deletes the node as well as there is no outer vtNode
				// holding a reference.
				m_pGroup->removeChild(node);
			}
			else
			{
				m_pGroup->removeChild(pChild->GetOsgNode());
				pChild->Release();
			}
		}
		m_pGroup = NULL;
	}
	// Now release itself
	vtNode::Release();
}

vtNode *FindNodeByName(vtNode *node, const char *name)
{
	if (!strcmp(node->GetName2(), name))
		return node;

	const vtGroupBase *pGroup = dynamic_cast<const vtGroupBase *>(node);
	if (pGroup)
	{
		unsigned int children = pGroup->GetNumChildren();
		for (unsigned int i = 0; i < children; i++)
		{
			vtNode *pChild = pGroup->GetChild(i);
			vtNode *pResult = FindNodeByName(pChild, name);
			if (pResult)
				return pResult;
		}
	}
	return NULL;
}

const vtNode *vtGroup::FindDescendantByName(const char *name) const
{
	return FindNodeByName((vtNode *)this, name);
}

void vtGroup::AddChild(vtNode *pChild)
{
	if (pChild)
		m_pGroup->addChild(pChild->GetOsgNode());
}

void vtGroup::RemoveChild(vtNode *pChild)
{
	if (pChild)
		m_pGroup->removeChild(pChild->GetOsgNode());
}

vtNode *vtGroup::GetChild(unsigned int num) const
{
	unsigned int children = m_pGroup->getNumChildren();
	if (num < children)
	{
		Node *pChild = (Node *) m_pGroup->getChild(num);
		osg::Referenced *ref = pChild->getUserData();
		if (ref)
			return dynamic_cast<vtNode*>(ref);
		else
		{
			// We have found an unwrapped node
			vtNativeNode *native = new vtNativeNode(pChild);
			return native;
		}
	}
	else
		return NULL;
}

unsigned int vtGroup::GetNumChildren() const
{
	// shoudln't happen but... safety check anyway
	if (m_pGroup == NULL)
		return 0;
	return m_pGroup->getNumChildren();
}

bool vtGroup::ContainsChild(vtNode *pNode) const
{
	int i, children = GetNumChildren();
	for (i = 0; i < children; i++)
	{
		if (GetChild(i) == pNode)
			return true;
	}
	return false;
}


///////////////////////////////////////////////////////////////////////
// vtTransform
//

vtTransform::vtTransform() : vtGroup(true), vtTransformBase()
{
	m_pTransform = new osg::MatrixTransform;
	SetOsgGroup(m_pTransform);
}

vtTransform::vtTransform(osg::MatrixTransform *mt) : vtGroup(true), vtTransformBase()
{
	m_pTransform = mt;
	SetOsgGroup(m_pTransform);
}

vtNode *vtTransform::Clone(bool bDeep)
{
	vtTransform *newtrans = new vtTransform;
	newtrans->CloneFrom(this, bDeep);
	return newtrans;
}

void vtTransform::CloneFrom(vtTransform *xform, bool bDeep)
{
	// copy the transform's matrix
	const osg::MatrixTransform *mt = xform->m_pTransform;
	m_pTransform->setMatrix(mt->getMatrix());

	// and the parent members
	vtGroup::CloneFrom(xform, bDeep);
}

void vtTransform::Release()
{
	// Check if there are no more external references to this transform node.
	if (m_pNode->referenceCount() == 1)
		m_pTransform = NULL;
	vtGroup::Release();
}

void vtTransform::Identity()
{
	m_pTransform->setMatrix(Matrix::identity());
}

FPoint3 vtTransform::GetTrans() const
{
	Vec3 v = m_pTransform->getMatrix().getTrans();
	return FPoint3(v[0], v[1], v[2]);
}

void vtTransform::SetTrans(const FPoint3 &pos)
{
	osg::Matrix m = m_pTransform->getMatrix();
	m.setTrans(pos.x, pos.y, pos.z);
	m_pTransform->setMatrix(m);

	m_pTransform->dirtyBound();
}

void vtTransform::Translate1(const FPoint3 &pos)
{
	// OSG 0.8.43 and later
	m_pTransform->postMult(Matrix::translate(pos.x, pos.y, pos.z));
}

void vtTransform::TranslateLocal(const FPoint3 &pos)
{
	// OSG 0.8.43 and later
	m_pTransform->preMult(Matrix::translate(pos.x, pos.y, pos.z));
}

void vtTransform::Rotate2(const FPoint3 &axis, double angle)
{
	// OSG 0.8.43 and later
	m_pTransform->postMult(Matrix::rotate(angle, axis.x, axis.y, axis.z));
}

void vtTransform::RotateLocal(const FPoint3 &axis, double angle)
{
	// OSG 0.8.43 and later
	m_pTransform->preMult(Matrix::rotate(angle, axis.x, axis.y, axis.z));
}

void vtTransform::RotateParent(const FPoint3 &axis, double angle)
{
	// OSG 0.8.43 and later
	Vec3 trans = m_pTransform->getMatrix().getTrans();
	m_pTransform->postMult(Matrix::translate(-trans)*
			  Matrix::rotate(angle, axis.x, axis.y, axis.z)*
			  Matrix::translate(trans));
}

FQuat vtTransform::GetOrient() const
{
	const Matrix &xform = m_pTransform->getMatrix();
	Quat q;
	xform.get(q);
	return FQuat(q.x(), q.y(), q.z(), q.w());
}

FPoint3 vtTransform::GetDirection() const
{
	const Matrix &xform = m_pTransform->getMatrix();
	const osg_matrix_value *ptr = xform.ptr();
	return FPoint3(-ptr[8], -ptr[9], -ptr[10]);
}

void vtTransform::SetDirection(const FPoint3 &point, bool bPitch)
{
	// get current matrix
	FMatrix4 m4;
	GetTransform1(m4);

	// remember where it is now
	FPoint3 trans = m4.GetTrans();

	// orient it in the desired direction
	FMatrix3 m3;
	m3.MakeOrientation(point, bPitch);
	m4.SetFromMatrix3(m3);

	// restore translation
	m4.SetTrans(trans);

	// set current matrix
	SetTransform1(m4);
}

void vtTransform::Scale3(float x, float y, float z)
{
	// OSG 0.8.43 and later
	m_pTransform->preMult(Matrix::scale(x, y, z));
}

void vtTransform::SetTransform1(const FMatrix4 &mat)
{
	Matrix mat_osg;

	ConvertMatrix4(&mat, &mat_osg);

	m_pTransform->setMatrix(mat_osg);
	m_pTransform->dirtyBound();
}

void vtTransform::GetTransform1(FMatrix4 &mat) const
{
	const Matrix &xform = m_pTransform->getMatrix();
	ConvertMatrix4(&xform, &mat);
}

void vtTransform::PointTowards(const FPoint3 &point, bool bPitch)
{
	SetDirection(point - GetTrans(), bPitch);
}


///////////////////////////////////////////////////////////////////////
// vtFog
//

RGBf vtFog::s_white(1, 1, 1);

vtFog::vtFog() : vtGroup(false)
{
}

vtNode *vtFog::Clone(bool bDeep)
{
	vtFog *newfog = new vtFog;
	newfog->CloneFrom(this, bDeep);
	return newfog;
}

void vtFog::CloneFrom(vtFog *fog, bool bDeep)
{
	// copy
	// TODO

	// and the parent members
	vtGroup::CloneFrom(fog, bDeep);
}

void vtFog::Release()
{
	if (m_pNode->referenceCount() == 1)
	{
		m_pFogStateSet = NULL;
		m_pFog = NULL;
	}
	vtGroup::Release();
}

/**
 * Set the Fog state.
 *
 * You can turn fog on or off.  When you turn fog on, it affects all others
 * below it in the scene graph.
 *
 * \param bOn True to turn fog on, false to turn it off.
 * \param start The distance from the camera at which fog starts, in meters.
 * \param end The distance from the camera at which fog end, in meters.  This
 *		is the point at which it becomes totally opaque.
 * \param color The color of the fog.  All geometry will be faded toward this
 *		color.
 * \param Type Can be FM_LINEAR, FM_EXP, or FM_EXP2 for linear or exponential
 *		increase of the fog density.
 */
void vtFog::SetFog(bool bOn, float start, float end, const RGBf &color,
				   enum FogType Type)
{
	osg::StateSet *set = GetOsgNode()->getStateSet();
	if (!set)
	{
		m_pFogStateSet = new osg::StateSet;
		set = m_pFogStateSet.get();
		GetOsgNode()->setStateSet(set);
	}

	if (bOn)
	{
		Fog::Mode eType;
		switch (Type)
		{
		case FM_LINEAR: eType = Fog::LINEAR; break;
		case FM_EXP: eType = Fog::EXP; break;
		case FM_EXP2: eType = Fog::EXP2; break;
		default: return;
		}
		m_pFog = new Fog;
		m_pFog->setMode(eType);
		m_pFog->setDensity(0.25f);	// not used for linear
		m_pFog->setStart(start);
		m_pFog->setEnd(end);
		m_pFog->setColor(osg::Vec4(color.r, color.g, color.b, 1));

		set->setAttributeAndModes(m_pFog.get(), StateAttribute::OVERRIDE | StateAttribute::ON);
	}
	else
	{
		// turn fog off
		set->setMode(GL_FOG, StateAttribute::OFF);
	}
}


//////////////////////////////////////////////////////////////////////
// vtFog
//

vtShadow::vtShadow(const int ShadowTextureUnit) : m_ShadowTextureUnit(ShadowTextureUnit), vtGroup(true)
{
	m_pShadowedScene = new osgShadow::ShadowedScene;

	m_pShadowedScene->setReceivesShadowTraversalMask(ReceivesShadowTraversalMask);
	m_pShadowedScene->setCastsShadowTraversalMask(CastsShadowTraversalMask);

#if VTLISPSM
	osg::ref_ptr<CLightSpacePerspectiveShadowTechnique> pShadowTechnique = new CLightSpacePerspectiveShadowTechnique;
	// No need to set the BaseTextureUnit as the default of zero is OK for us
	// But the ShadowTextureUnit might be different (default 1)
	pShadowTechnique->setShadowTextureUnit(m_ShadowTextureUnit);
#else
	osg::ref_ptr<CSimpleInterimShadowTechnique> pShadowTechnique = new CSimpleInterimShadowTechnique;
#endif

#if !VTLISPSM
#if VTDEBUGSHADOWS
	// add some instrumentation
	pShadowTechnique->m_pParent = this;
#endif

	pShadowTechnique->SetShadowTextureUnit(m_ShadowTextureUnit);
	pShadowTechnique->SetShadowSphereRadius(50.0);
#endif
	m_pShadowedScene->setShadowTechnique(pShadowTechnique.get());

	SetOsgGroup(m_pShadowedScene);
}

vtNode *vtShadow::Clone(bool bDeep)
{
	vtShadow *newshadow = new vtShadow(m_ShadowTextureUnit);
	newshadow->CloneFrom(this, bDeep);
	return newshadow;
}

void vtShadow::CloneFrom(vtShadow *shadow, bool bDeep)
{
	// copy
	// TODO

	// and the parent members
	vtGroup::CloneFrom(shadow, bDeep);
}

void vtShadow::Release()
{
	if (m_pNode->referenceCount() == 1)
		m_pShadowedScene = NULL;

	vtGroup::Release();
}

void vtShadow::SetDarkness(float bias)
{
	CSimpleInterimShadowTechnique *pTechnique = dynamic_cast<CSimpleInterimShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
		pTechnique->SetShadowDarkness(bias);
}

float vtShadow::GetDarkness()
{
	CSimpleInterimShadowTechnique *pTechnique = dynamic_cast<CSimpleInterimShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
		return pTechnique->GetShadowDarkness();
	else
		return 1.0f;
}

void vtShadow::AddAdditionalTerrainTextureUnit(const unsigned int Unit, const unsigned int Mode)
{
#if VTLISPSM
	CLightSpacePerspectiveShadowTechnique *pTechnique = dynamic_cast<CLightSpacePerspectiveShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
		pTechnique->AddAdditionalTerrainTextureUnit(Unit, Mode);
#else
	CSimpleInterimShadowTechnique *pTechnique = dynamic_cast<CSimpleInterimShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
		pTechnique->AddMainSceneTextureUnit(Unit, Mode);
#endif
}

void vtShadow::RemoveAdditionalTerrainTextureUnit(const unsigned int Unit)
{
#if VTLISPSM
	CLightSpacePerspectiveShadowTechnique *pTechnique = dynamic_cast<CLightSpacePerspectiveShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
		pTechnique->RemoveAdditionalTerrainTextureUnit(Unit);
#else
	CSimpleInterimShadowTechnique *pTechnique = dynamic_cast<CSimpleInterimShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
		pTechnique->RemoveMainSceneTextureUnit(Unit);
#endif
}

void vtShadow::RemoveAllAdditionalTerrainTextureUnits()
{
#if VTLISPSM
	CLightSpacePerspectiveShadowTechnique *pTechnique = dynamic_cast<CLightSpacePerspectiveShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
		pTechnique->RemoveAllAdditionalTerrainTextureUnits();
#endif
}

void vtShadow::SetShadowTextureResolution(const unsigned int ShadowTextureResolution)
{
#if VTLISPSM
	CLightSpacePerspectiveShadowTechnique *pTechnique = dynamic_cast<CLightSpacePerspectiveShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
		pTechnique->setTextureSize(osg::Vec2s(ShadowTextureResolution,ShadowTextureResolution));
#else
	CSimpleInterimShadowTechnique *pTechnique = dynamic_cast<CSimpleInterimShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
		pTechnique->SetShadowTextureResolution(ShadowTextureResolution);
#endif
}

void vtShadow::SetRecalculateEveryFrame(const bool RecalculateEveryFrame)
{
	CSimpleInterimShadowTechnique *pTechnique = dynamic_cast<CSimpleInterimShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
		pTechnique->SetRecalculateEveryFrame(RecalculateEveryFrame);
}

bool vtShadow::GetRecalculateEveryFrame() const
{
	CSimpleInterimShadowTechnique *pTechnique = dynamic_cast<CSimpleInterimShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
		return pTechnique->GetRecalculateEveryFrame();
	else 
		return false;
}

void vtShadow::SetShadowSphereRadius(const float ShadowSphereRadius)
{
#if VTLISPSM
	CLightSpacePerspectiveShadowTechnique *pTechnique = dynamic_cast<CLightSpacePerspectiveShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
		pTechnique->setMaxFarPlane(ShadowSphereRadius);
#else
	CSimpleInterimShadowTechnique *pTechnique = dynamic_cast<CSimpleInterimShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
		pTechnique->SetShadowSphereRadius(ShadowSphereRadius);
#endif
}

void vtShadow::SetHeightField3d(vtHeightField3d *pHeightField3d)
{
	CSimpleInterimShadowTechnique *pTechnique = dynamic_cast<CSimpleInterimShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
		pTechnique->SetHeightField3d(pHeightField3d);
}

void vtShadow::AddLodGridToIgnore(vtLodGrid* pLodGrid)
{
	CSimpleInterimShadowTechnique *pTechnique = dynamic_cast<CSimpleInterimShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
		pTechnique->AddLodGridToIgnore(pLodGrid);
}

void vtShadow::ForceShadowUpdate()
{
	CSimpleInterimShadowTechnique *pTechnique = dynamic_cast<CSimpleInterimShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
		pTechnique->ForceShadowUpdate();
}

void vtShadow::SetDebugHUD(vtGroup *pGroup)
{
#if defined (VTDEBUG) && defined (VTDEBUGSHADOWS)
	CSimpleInterimShadowTechnique *pTechnique = dynamic_cast<CSimpleInterimShadowTechnique *>(m_pShadowedScene->getShadowTechnique());
	if (pTechnique)
	{
		osg::ref_ptr<osg::Camera> pCamera = pTechnique->makeDebugHUD();
		pCamera->setName("Shadow DEBUG HUD camera");
		pGroup->GetOsgGroup()->addChild(pCamera.get());
	}
#endif
}

///////////////////////////////////////////////////////////////////////
// vtLight
//

vtLight::vtLight()
{
	// Lights can now go into the scene graph in OSG, with LightSource.
	// A lightsource creates a light, which we can get with getLight().
	m_pLightSource = new osg::LightSource;
	SetOsgNode(m_pLightSource);

	// However, because lighting is also a 'state', we need to inform
	// the whole scene graph that we have another light.
	m_pLightSource->setLocalStateSetModes(osg::StateAttribute::ON);
	m_pLightSource->setStateSetModes(*vtGetScene()->getViewer()->getCamera()->getOrCreateStateSet(),osg::StateAttribute::ON);
}

vtNode *vtLight::Clone(bool bDeep)
{
	vtLight *light = new vtLight;
	light->CloneFrom(this);
	return light;
}

void vtLight::CloneFrom(const vtLight *rhs)
{
	// copy attributes
	SetDiffuse(rhs->GetDiffuse());
	SetAmbient(rhs->GetAmbient());
}

void vtLight::Release()
{
	// Check if we are completely deferenced
	if (m_pNode->referenceCount() == 1)
		m_pLightSource = NULL;
	vtNode::Release();
}

void vtLight::SetDiffuse(const RGBf &color)
{
	GetOsgLight()->setDiffuse(v2s(color));
}

RGBf vtLight::GetDiffuse() const
{
	return s2v(GetOsgLight()->getDiffuse());
}

void vtLight::SetAmbient(const RGBf &color)
{
	GetOsgLight()->setAmbient(v2s(color));
}

RGBf vtLight::GetAmbient() const
{
	return s2v(GetOsgLight()->getAmbient());
}

void vtLight::SetSpecular(const RGBf &color)
{
	GetOsgLight()->setSpecular(v2s(color));
}

RGBf vtLight::GetSpecular() const
{
	return s2v(GetOsgLight()->getSpecular());
}

void vtLight::SetEnabled(bool bOn)
{
/*
	// TODO - figure out the long discussion on the OSG list about how to
	//  actually disable a light.
	if (bOn)
		m_pLightSource->setLocalStateSetModes(StateAttribute::ON);
	else
		m_pLightSource->setLocalStateSetModes(StateAttribute::OFF);
*/
	vtNode::SetEnabled(bOn);
}


///////////////////////////////////////////////////////////////////////
// vtCamera
//

vtCamera::vtCamera() : vtTransform()
{
	m_fHither = 1;
	m_fYon = 100;
	m_fFOV = PIf/3.0f;
	m_bOrtho = false;
	m_fWidth = 1;
}

vtNode *vtCamera::Clone(bool bDeep)
{
	vtCamera *newcam = new vtCamera;
	newcam->CloneFrom(this, bDeep);
	return newcam;
}

void vtCamera::CloneFrom(vtCamera *rhs, bool bDeep)
{
	m_fFOV = rhs->m_fFOV;
	m_fHither = rhs->m_fHither;
	m_fYon = rhs->m_fYon;
	m_bOrtho = rhs->m_bOrtho;
	m_fWidth = rhs->m_fWidth;

	// Also parent
	vtTransform::CloneFrom(rhs, bDeep);
}

/**
 * Set the hither (near) clipping plane distance.
 */
void vtCamera::SetHither(float f)
{
	m_fHither = f;
}

/**
 * Get the hither (near) clipping plane distance.
 */
float vtCamera::GetHither() const
{
	return m_fHither;
}

/**
 * Set the yon (far) clipping plane distance.
 */
void vtCamera::SetYon(float f)
{
	m_fYon = f;
}

/**
 * Get the yon (far) clipping plane distance.
 */
float vtCamera::GetYon() const
{
	return m_fYon;
}

/**
 * Set the camera's horizontal field of view (FOV) in radians.
 */
void vtCamera::SetFOV(float fov_x)
{
	m_fFOV = fov_x;
}

/**
 * Return the camera's horizontal field of view (FOV) in radians.
 */
float vtCamera::GetFOV() const
{
	return m_fFOV;
}

/**
 * Return the camera's vertical field of view (FOV) in radians.
 */
float vtCamera::GetVertFOV() const
{
	IPoint2 size = vtGetScene()->GetWindowSize();
	float aspect = (float) size.x / size.y;

	double a = tan(m_fFOV/2);
	double b = a / aspect;
	return atan(b) * 2;
}

/**
 * Zoom (move) the camera to a sphere, generally the bounding sphere of
 *  something you want to look at.  The camera will be pointing directly
 *  down the -Z axis at the center of the sphere.
 *
 * \param sphere The sphere to look at.
 * \param fPitch An optional pitch angle to look, in radians.  For example,
 *		a pitch of -PIf/10 would be looking down at a mild angle.
 */
void vtCamera::ZoomToSphere(const FSphere &sphere, float fPitch)
{
	Identity();
	Translate1(sphere.center);
	RotateLocal(FPoint3(1,0,0), fPitch);
	TranslateLocal(FPoint3(0.0f, 0.0f, sphere.radius));
}

/**
 * Set this camera to use an orthographic view.  An orthographic view has
 *  no FOV angle, so Set/GetFOV have no affect.  Instead, use Get/SetWidth
 *  to control the width of the orthogonal view.
 */
void vtCamera::SetOrtho(bool bOrtho)
{
	m_bOrtho = bOrtho;
}

/**
 * Return true if the camera is orthographic.
 */
bool vtCamera::IsOrtho() const
{
	return m_bOrtho;
}

/**
 * Set the view width of an orthographic camera.
 */
void vtCamera::SetWidth(float fWidth)
{
	m_fWidth = fWidth;
}

/**
 * Get the view width of an orthographic camera.
 */
float vtCamera::GetWidth() const
{
	return m_fWidth;
}


///////////////////////////////////////////////////////////////////////
// vtGeom
//

vtGeom::vtGeom() : vtNode()
{
	m_pGeode = new Geode;
	SetOsgNode(m_pGeode);
}

vtNode *vtGeom::Clone(bool bDeep)
{
	vtGeom *newgeom = new vtGeom;
	newgeom->CloneFrom(this);
	return newgeom;
}

void vtGeom::CloneFrom(const vtGeom *rhs)
{
	// Shallow copy: just reference the meshes and materials of the
	//  geometry that we are copying from.
	SetMaterials(rhs->GetMaterials());
	int idx;
	for (unsigned int i = 0; i < rhs->GetNumMeshes(); i++)
	{
		vtMesh *mesh = rhs->GetMesh(i);
		if (mesh)
		{
			idx = mesh->GetMatIndex();
			AddMesh(mesh, idx);
		}
		else
		{
			vtTextMesh *tm = rhs->GetTextMesh(i);
			if (tm)
			{
				idx = tm->GetMatIndex();
				AddTextMesh(tm, idx);
			}
		}
	}
}

void vtGeom::Release()
{
	if (m_pNode->referenceCount() == 1)
	{
		// Clean up this geom, it is going away.
		// Release the meshes we contain, which will delete them if there
		//  are no other references to them.
		int i, num = m_pGeode->getNumDrawables();
		for (i = 0; i < num; i++)
		{
			vtMesh *mesh = GetMesh(i);
			if (mesh)
				mesh->Release();
			else
			{
				vtTextMesh *textmesh = GetTextMesh(i);
				if (textmesh)
					textmesh->Release();
			}
		}
#if OSG_VERSION_MAJOR == 1 && OSG_VERSION_MINOR > 0 || OSG_VERSION_MAJOR > 1
		// We are probably OSG 1.1 or newer
		m_pGeode->removeDrawables(0, num);
#else
		m_pGeode->removeDrawable(0, num);
#endif

		m_pGeode = NULL;
		m_pMaterialArray = NULL;		// dereference
	}
	vtNode::Release();
}

void vtGeom::AddMesh(vtMesh *pMesh, int iMatIdx)
{
	m_pGeode->addDrawable(pMesh->m_pGeometry.get());

	// The vtGeom owns/references the meshes it contains
	pMesh->ref();

	SetMeshMatIndex(pMesh, iMatIdx);
}

void vtGeom::AddTextMesh(vtTextMesh *pTextMesh, int iMatIdx)
{
	// connect the underlying OSG objects
	m_pGeode->addDrawable(pTextMesh->m_pOsgText.get());

	// The vtGeom owns/references the meshes it contains
	pTextMesh->ref();

	// Normally, we would assign the material state to the drawable.
	// However, OSG treats Text specially, it cannot be affected by normal
	//  material statesets.  For example, it always sets its own color,
	//  ignore any State settings.  So, disabled the following.
#if 0
	pTextMesh->SetMatIndex(iMatIdx);
	vtMaterial *pMat = GetMaterial(iMatIdx);
	if (pMat)
	{
		StateSet *pState = pMat->m_pStateSet.get();
		pTextMesh->m_pOsgText->setStateSet(pState);
	}
#endif

	// In fact, we need to avoid lighting the text, yet text messes with
	//  own StateSet, so we can't set it there.  We set it here.
	StateSet *sset = m_pGeode->getOrCreateStateSet();
	sset->setMode(GL_LIGHTING, StateAttribute::OFF);
	// also not useful to see the back of text (mirror writing)
	sset->setMode(GL_CULL_FACE, StateAttribute::ON);
}

void vtGeom::SetMeshMatIndex(vtMesh *pMesh, int iMatIdx)
{
	vtMaterial *pMat = GetMaterial(iMatIdx);
	if (pMat)
	{
		StateSet *pState = pMat->m_pStateSet.get();

#if 0
		// Beware: the mesh may already have its own stateset?
		//  In what case would this arise?  The user might be calling this
		//   method on a mesh which already has a material.  In that case,
		//	 we want to cleanly switch to the new material, not merge into
		//	 the old one.
		StateSet *pStateMesh = pMesh->m_pGeometry->getStateSet();
		if (pStateMesh)
			pStateMesh->merge(*pState);
		else
#endif
			pMesh->m_pGeometry->setStateSet(pState);

		// Try to provide color for un-lit meshes
		if (!pMat->GetLighting())
		{
			// unless it's using vertex colors...
			Geometry::AttributeBinding bd = pMesh->m_pGeometry->getColorBinding();
			if (bd != Geometry::BIND_PER_VERTEX)
			{
				// not lit, not vertex colors
				// here is a sneaky way of forcing OSG to use the diffuse
				// color for the unlit color

				// This will leave the original color array alllocated in the vtMesh
				Vec4Array *pColors = new Vec4Array;
				pColors->push_back(pMat->m_pMaterial->getDiffuse(FAB));
				pMesh->m_pGeometry->setColorArray(pColors);
				pMesh->m_pGeometry->setColorBinding(Geometry::BIND_OVERALL);
			}
		}
	}
	pMesh->SetMatIndex(iMatIdx);
}

void vtGeom::RemoveMesh(vtMesh *pMesh)
{
	// If this geom has this mesh, remove it, and check if needs releasing
	if (m_pGeode->removeDrawable(pMesh->m_pGeometry.get()))
	{
		if (pMesh->_refCount == 2)
		{
			// no more references except its default
			// self-reference and the reflexive reference from its m_pGeometry.
			pMesh->Release();
		}
	}
}

unsigned int vtGeom::GetNumMeshes() const
{
	return m_pGeode->getNumDrawables();
}

vtMesh *vtGeom::GetMesh(int i) const
{
	// It is valid to return a non-const pointer to the mesh, since the mesh
	//  can be modified entirely independently of the geometry.
	Drawable *draw = const_cast<Drawable *>( m_pGeode->getDrawable(i) );
	osg::Referenced *ref = draw->getUserData();

	vtMesh *mesh = dynamic_cast<vtMesh*>(ref);
	return mesh;
}

vtTextMesh *vtGeom::GetTextMesh(int i) const
{
	// It is valid to return a non-const pointer to the mesh, since the mesh
	//  can be modified entirely independently of the geometry.
	Drawable *draw = const_cast<Drawable *>( m_pGeode->getDrawable(i) );
	osg::Referenced *ref = draw->getUserData();

	vtTextMesh *mesh = dynamic_cast<vtTextMesh*>(ref);
	return mesh;
}

void vtGeom::SetMaterials(const class vtMaterialArray *mats)
{
	m_pMaterialArray = mats;	// increases reference count
}

const vtMaterialArray *vtGeom::GetMaterials() const
{
	return m_pMaterialArray.get();
}

vtMaterial *vtGeom::GetMaterial(int idx)
{
	if (!m_pMaterialArray.valid())
		return NULL;
	if (idx < 0 || idx >= (int) m_pMaterialArray->GetSize())
		return NULL;
	return m_pMaterialArray->GetAt(idx);
}


///////////////////////////////////////////////////////////////////////
// vtLOD
//

vtLOD::vtLOD() : vtGroup(true)
{
	m_pLOD = new osg::LOD;
	m_pLOD->setCenter(osg::Vec3(0, 0, 0));
	SetOsgGroup(m_pLOD);
}

void vtLOD::Release()
{
	// Check if this node is no longer referenced.
	if (m_pNode->referenceCount() == 1)
		m_pLOD = NULL;
	vtGroup::Release();
}

void vtLOD::SetRanges(float *ranges, int nranges)
{
	int i;
	float next;
	for (i = 0; i < nranges; i++)
	{
		if (i < nranges - 1)
			next = ranges[i+1];
		else
			next = 1E10;
		m_pLOD->setRange(i, ranges[i], next);
	}
}

void vtLOD::SetCenter(FPoint3 &center)
{
	Vec3 p;
	v2s(center, p);
	m_pLOD->setCenter(p);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS


///////////////////////////////////////////////////////////////////////
// OsgDynMesh
//

OsgDynMesh::OsgDynMesh()
{
	// The following line code is a workaround provided by Robert Osfield himself
	// create an empty stateset, to force the traversers
	// to nest any state above it in the inheritance path.
	setStateSet(new StateSet);
}

osg::BoundingBox OsgDynMesh::computeBound() const
{
	FBox3 box;
	m_pDynGeom->DoCalcBoundBox(box);

	// convert it to OSG bounds
	v2s(box.min, _boundingBox._min);
	v2s(box.max, _boundingBox._max);

	_boundingBoxComputed=true;
	return _boundingBox;
}


#if OSG_VERSION_MAJOR >= 2
void OsgDynMesh::drawImplementation(osg::RenderInfo& renderInfo) const
#else
void OsgDynMesh::drawImplementation(State& state) const
#endif
{
	OsgDynMesh *cthis = const_cast<OsgDynMesh*>(this);
#if OSG_VERSION_MAJOR >= 2
	cthis->m_pDrawState = renderInfo.getState();
#else
	cthis->m_pDrawState = (&state);
#endif

	// Our dyamic mesh might use Vertex Arrays, and this can conflict with
	//  other objects in the OSG scene graph which are also using Vertex
	//  Arrays.  To avoid this, we disable the OSG arrays before the
	//  dyamic geometry draws itself.
	// NOTE: I would guess we should be pushing/popping the state here,
	//  but i don't understand how to use osg::State that way, and just
	//  disabling the arrays seems to make things work!
	cthis->m_pDrawState->disableAllVertexArrays();

	vtScene *pScene = vtGetScene();
	vtCamera *pCam = pScene->GetCamera();

	// setup the culling planes
	m_pDynGeom->m_pPlanes = pScene->GetCullPlanes();

	m_pDynGeom->DoCull(pCam);
	m_pDynGeom->DoRender();
}

#endif // DOXYGEN_SHOULD_SKIP_THIS

vtDynGeom::vtDynGeom() : vtGeom()
{
	m_pDynMesh = new OsgDynMesh();
	m_pDynMesh->m_pDynGeom = this;
	m_pDynMesh->setSupportsDisplayList(false);

#if VTLISPSM
	m_pGeode->getOrCreateStateSet()->addUniform( new osg::Uniform( "renderingVTPBaseTexture", int( 1 ) ) );
#endif

	m_pGeode->addDrawable(m_pDynMesh);
}


/**
 * Test a sphere against the view volume.
 *
 * \return VT_AllVisible if entirely inside the volume,
 *			VT_Visible if partly inside,
 *			otherwise 0.
 */
int vtDynGeom::IsVisible(const FSphere &sphere) const
{
	unsigned int vis = 0;

	// cull against standard frustum
	int i;
	for (i = 0; i < 4; i++)
	{
		float dist = m_pPlanes[i].Distance(sphere.center);
		if (dist >= sphere.radius)
			return 0;
		if ((dist < 0) &&
			(dist <= sphere.radius))
			vis = (vis << 1) | 1;
	}

	// Notify renderer that object is entirely within standard frustum, so
	// no clipping is necessary.
	if (vis == 0x0F)
		return VT_AllVisible;
	return VT_Visible;
}


/**
 * Test a single point against the view volume.
 *
 * \return true if inside, false if outside.
 */
bool vtDynGeom::IsVisible(const FPoint3& point) const
{
	// cull against standard frustum
	for (unsigned i = 0; i < 4; i++)
	{
		float dist = m_pPlanes[i].Distance(point);
		if (dist > 0.0f)
			return false;
	}
	return true;
}


/**
 * Test a 3d triangle against the view volume.
 *
 * \return VT_AllVisible if entirely inside the volume,
 *			VT_Visible if partly intersecting,
 *			otherwise 0.
 */
int vtDynGeom::IsVisible(const FPoint3& point0,
							const FPoint3& point1,
							const FPoint3& point2,
							const float fTolerance) const
{
	unsigned int outcode0 = 0, outcode1 = 0, outcode2 = 0;
	register float dist;

	// cull against standard frustum
	int i;
	for (i = 0; i < 4; i++)
	{
		dist = m_pPlanes[i].Distance(point0);
		if (dist > fTolerance)
			outcode0 |= (1 << i);

		dist = m_pPlanes[i].Distance(point1);
		if (dist > fTolerance)
			outcode1 |= (1 << i);

		dist = m_pPlanes[i].Distance(point2);
		if (dist > fTolerance)
			outcode2 |= (1 << i);
	}
	if (outcode0 == 0 && outcode1 == 0 && outcode2 == 0)
		return VT_AllVisible;
	if (outcode0 != 0 && outcode1 != 0 && outcode2 != 0)
	{
		if ((outcode0 & outcode1 & outcode2) != 0)
			return 0;
		else
		{
			// tricky case - just be conservative and assume some intersection
			return VT_Visible;
		}
	}
	// not all in, and not all out
	return VT_Visible;
}

/**
 * Test a sphere against the view volume.
 *
 * \return VT_AllVisible if entirely inside the volume,
 *			VT_Visible if partly intersecting,
 *			otherwise 0.
 */
int vtDynGeom::IsVisible(const FPoint3 &point, float radius)
{
	unsigned int incode = 0;

	// cull against standard frustum
	for (int i = 0; i < 4; i++)
	{
		float dist = m_pPlanes[i].Distance(point);
		if (dist > radius)
			return 0;			// entirely outside this plane
		if (dist < -radius)
			incode |= (1 << i);	// entirely inside this plane
	}
	if (incode == 0x0f)
		return VT_AllVisible;	// entirely inside all planes
	else
		return VT_Visible;
}

void vtDynGeom::ApplyMaterial(vtMaterial *mat)
{
	if (m_pDynMesh && m_pDynMesh->m_pDrawState)
	{
		m_pDynMesh->m_pDrawState->apply(mat->m_pStateSet.get());
		// Dynamic terrain assumes texture unit 0
		m_pDynMesh->m_pDrawState->setActiveTextureUnit(0);
	}
}

///////////////////////////////////////////////////////////
// vtHUD

/**
 * Create a HUD node.  A HUD ("heads-up display") is a group whose whose
 * children are transformed to be drawn in window coordinates, rather
 * than world coordinates.
 *
 * You should only ever create one HUD node in your scenegraph.
 *
 * \param bPixelCoords If true, the child transforms should be interpreted
 *		as pixel coordinates, from (0,0) in the lower-left of the viewpoint.
 *		Otherwise, they are considered in normalized window coordinates,
 *		from (0,0) in the lower-left to (1,1) in the upper right.
 */
vtHUD::vtHUD(bool bPixelCoords) : vtGroup(true)
{
	osg::MatrixTransform* modelview_abs = new osg::MatrixTransform;
	modelview_abs->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	modelview_abs->setMatrix(osg::Matrix::identity());

	m_projection = new osg::Projection;
	m_projection->addChild(modelview_abs);
	SetOsgGroup(m_projection);

	// We can set the projection to pixels (0,width,0,height) or
	//	normalized (0,1,0,1)
	m_bPixelCoords = bPixelCoords;
	if (m_bPixelCoords)
	{
		IPoint2 winsize = vtGetScene()->GetWindowSize();

		// safety check first, avoid /0 crash
		if (winsize.x != 0 && winsize.y != 0)
			m_projection->setMatrix(osg::Matrix::ortho2D(0, winsize.x, 0, winsize.y));
	}
	else
	{
		// Normalized window coordinates, 0 to 1
		m_projection->setMatrix(osg::Matrix::ortho2D(0, 1, 0, 1));
	}

	// To ensure that the sprite appears on top we can use osg::Depth to
	//  force the depth fragments to be placed at the front of the screen.
	osg::StateSet* stateset = m_projection->getOrCreateStateSet();
	stateset->setAttribute(new osg::Depth(osg::Depth::LESS,0.0,0.0001));

	// A HUD node is unlike other group nodes!
	// The modelview node is the container for the node's children.
	m_pGroup = modelview_abs;

	vtGetScene()->SetHUD(this);
}

void vtHUD::Release()
{
	// Check if there are no more external references to this HUD node.
	if (m_pNode->referenceCount() == 1)
	{
		m_projection = NULL;
		vtGetScene()->SetHUD(NULL);
	}
	vtGroup::Release();
}

vtNode *vtHUD::Clone(bool bDeep)
{
	vtHUD *hud = new vtHUD;
	hud->CloneFrom(this, bDeep);
	return hud;
}

void vtHUD::CloneFrom(vtHUD *rhs, bool bDeep)
{
	// TODO
}

void vtHUD::SetWindowSize(int w, int h)
{
	if (m_bPixelCoords)
	{
		if (w != 0 && h != 0)
		{
			m_projection->setMatrix(osg::Matrix::ortho2D(0, w, 0, h));
			// VTLOG("HUD SetWindowSize %d %d\n", w, h);
		}
	}
}


///////////////////////////////////////////////////////////////////////
// vtImageSprite

vtImageSprite::vtImageSprite()
{
	m_pMats = NULL;
	m_pGeom = NULL;
	m_pMesh = NULL;
}

vtImageSprite::~vtImageSprite()
{
	// Do not explicitly free geometry, if it was added to the scene.
}

/**
 * Create a vtImageSprite.
 *
 * \param szTextureName The filename of a texture image.
 * \param bBlending Set to true for alpha-blending, which produces smooth
 *		edges on transparent textures.
 */
bool vtImageSprite::Create(const char *szTextureName, bool bBlending)
{
	vtImage *pImage = vtImageRead(szTextureName);
	if (!pImage)
		return false;
	bool success = Create(pImage, bBlending);
	pImage->Release();	// pass ownership
	return success;
}

/**
 * Create a vtImageSprite.
 *
 * Note that if you are not using the image for anything else, you should give
 * ownership to the imagesprite.  Example:
 \code
 vtImage *image = vtImageRead("foo.png");
 vtImageSprite *sprite = new vtImageSprite(image, false);
 image->Release();	// pass ownership
 \endcode
 *
 * \param pImage A texture image.
 * \param bBlending Set to true for alpha-blending, which produces smooth
 *		edges on transparent textures.
 */
bool vtImageSprite::Create(vtImage *pImage, bool bBlending)
{
	m_Size.x = pImage->GetWidth();
	m_Size.y = pImage->GetHeight();

	// set up material and geometry container
	m_pMats = new vtMaterialArray;
	m_pGeom = new vtGeom;
	m_pGeom->SetMaterials(m_pMats);
	m_pMats->Release();	// pass ownership

	m_pMats->AddTextureMaterial(pImage, false, false, bBlending);

	// default position of the mesh is just 0,0-1,1
	m_pMesh = new vtMesh(vtMesh::QUADS, VT_TexCoords, 4);
	m_pMesh->AddVertexUV(FPoint3(0,0,0), FPoint2(0,0));
	m_pMesh->AddVertexUV(FPoint3(1,0,0), FPoint2(1,0));
	m_pMesh->AddVertexUV(FPoint3(1,1,0), FPoint2(1,1));
	m_pMesh->AddVertexUV(FPoint3(0,1,0), FPoint2(0,1));
	m_pMesh->AddQuad(0, 1, 2, 3);
	m_pGeom->AddMesh(m_pMesh, 0);
	m_pMesh->Release();
	return true;
}

/**
 * Set the XY position of the sprite.  These are in world coordinates,
 *  unless this sprite is the child of a vtHUD, in which case they are
 *  pixel coordinates measured from the lower-left corner of the window.
 *
 * \param l Left.
 * \param t Top.
 * \param r Right.
 * \param b Bottom.
 * \param rot Rotation in radians.
 */
void vtImageSprite::SetPosition(float l, float t, float r, float b, float rot)
{
	if (!m_pMesh)	// safety check
		return;

	FPoint2 p[4];
	p[0].Set(l, b);
	p[1].Set(r, b);
	p[2].Set(r, t);
	p[3].Set(l, t);

	if (rot != 0.0f)
	{
		FPoint2 center((l+r)/2, (b+t)/2);
		for (int i = 0; i < 4; i++)
		{
			p[i] -= center;
			p[i].Rotate(rot);
			p[i] += center;
		}
	}

	for (int i = 0; i < 4; i++)
		m_pMesh->SetVtxPos(i, FPoint3(p[i].x, p[i].y, 0));

	m_pMesh->ReOptimize();
}

/**
 * Set (replace) the image on a sprite that has already been created.
 */
void vtImageSprite::SetImage(vtImage *pImage)
{
	// Sprite must already be created
	if (!m_pMats)
		return;
	vtMaterial *mat = m_pMats->GetAt(0);
	mat->SetTexture(pImage);
}


///////////////////////////////////////////////////////////////////////
// Intersection methods

#include <osgUtil/IntersectVisitor>

/**
 * Check for surface intersections along a line segment in space.
 *
 * \param pTop The top of the scene graph that you want to search for
 *		intersections.  This can be the root node if you want to seach your
 *		entire scene, or any other node to search a subgraph.
 * \param start The start point (world coordinates) of the line segment.
 * \param end	The end point (world coordinates) of the line segment.
 * \param hitlist The results.  If there are intersections (hits), they are
 *		placed in this list, which is simply a std::vector of vtHit objects.
 *		Each vtHit object gives information about the hit point.
 * \param bLocalCoords Pass true to get your results in local coordinates
 *		(in the frame of the object which was hit).  Otherwise, result points
 *		are in world coordinates.
 * \param bNativeNodes Pass true to return the internal (native) scene graph
 *		node that was hit, when there is no corresponding vtNode.  Pass false
 *		to always return a vtNode, by looking up the scene graph as needed.
 *
 * \return The number of intersection hits (size of the hitlist array).
 */
int vtIntersect(vtNode *pTop, const FPoint3 &start, const FPoint3 &end,
				vtHitList &hitlist, bool bLocalCoords, bool bNativeNodes)
{
	// set up intersect visitor and create the line segment
	osgUtil::IntersectVisitor visitor;
	osgUtil::IntersectVisitor::HitList hlist;

	osg::ref_ptr<osg::LineSegment> segment = new osg::LineSegment;
	segment->set(v2s(start), v2s(end));
	visitor.addLineSegment(segment.get());

	// the accept() method does the intersection testing work
	osg::Node *osgnode = pTop->GetOsgNode();
	osgnode->accept(visitor);

	hlist = visitor.getHitList(segment.get());

#if 0	// Diagnostic code
	for(osgUtil::IntersectVisitor::HitList::iterator hitr=hlist.begin();
		hitr!=hlist.end(); ++hitr)
	{
		if (hitr->_geode.valid())
		{
			if (hitr->_geode->getName().empty())
				VTLOG("Geode %lx\n", hitr->_geode.get());
			else
				// the geodes are identified by name.
				VTLOG("Geode '%s'\n", hitr->_geode->getName().c_str());
		}
		else if (hitr->_drawable.valid())
		{
			VTLOG("Drawable class '%s'\n", hitr->_drawable->className());
		}
		else
		{
			osg::Vec3 point = hitr->getLocalIntersectPoint();
			VTLOG("Hitpoint %.1f %.1f %.1f\n", point.x(), point.y(), point.z());
		}
	}
	if (hlist.size() > 0)
		VTLOG("\n");
#endif

	// look through the node hits that OSG returned
	for(osgUtil::IntersectVisitor::HitList::iterator hitr=hlist.begin();
		hitr!=hlist.end(); ++hitr)
	{
		if (!hitr->_geode.valid())
			continue;

		osg::Node *onode = hitr->_geode.get();
		vtNode *vnode = NULL;
		if (bNativeNodes)
		{
			vnode = (vtNode *) (hitr->_geode->getUserData());
			if (vnode == NULL)
			{
				// a bit radical here - wrap the OSG node in a VTLIB wrapper
				//  on the fly.  hope it doesn't get confused with refcounts.
				vtNativeNode *native = new vtNativeNode(onode);
				vnode = native;
			}
		}
		else
		{
			// Look up along the nodepath for a real vtNode
			osg::NodePath &NodePath = hitr->getNodePath();
			for (osg::NodePath::reverse_iterator Ritr = NodePath.rbegin(); Ritr != NodePath.rend(); ++Ritr)
			{
				vnode = dynamic_cast<vtNode*>((*Ritr)->getUserData());
				if (vnode)
					break;
			}
			if (NULL == vnode)
				continue;
		}
		// put it on the list of hit results
		vtHit hit;
		hit.node = vnode;
		if (bLocalCoords)
		{
			hit.point = s2v(hitr->getLocalIntersectPoint());
		}
		else
		{
			hit.point = s2v(hitr->getWorldIntersectPoint());
		}
		hit.distance = (s2v(hitr->getWorldIntersectPoint()) - start).Length();
		hitlist.push_back(hit);
	}
	std::sort(hitlist.begin(), hitlist.end());
	return hitlist.size();
}

#include <osg/PositionAttitudeTransform>
#include <osg/AutoTransform>
#include <osg/TexGenNode>
#include <osg/Switch>
#include <osg/Sequence>

#include <osg/OccluderNode>
#include <osg/CoordinateSystemNode>
#include <osg/Billboard>

#include <osg/ClipNode>


// Diagnostic function to help debugging
void vtLogNativeGraph(osg::Node *node, bool bExtents, bool bRefCounts, int indent)
{
	for (int i = 0; i < indent; i++)
		VTLOG1(" ");
	if (node)
	{
		VTLOG("<%x>", node);

		if (dynamic_cast<osg::PositionAttitudeTransform*>(node))
			VTLOG1(" (PositionAttitudeTransform)");
		else if (dynamic_cast<osg::MatrixTransform*>(node))
			VTLOG1(" (MatrixTransform)");
		else if (dynamic_cast<osg::AutoTransform*>(node))
			VTLOG1(" (AutoTransform)");
		else if (dynamic_cast<osg::Transform*>(node))
			VTLOG1(" (Transform)");

		else if (dynamic_cast<osg::TexGenNode*>(node))
			VTLOG1(" (TexGenNode)");
		else if (dynamic_cast<osg::Switch*>(node))
			VTLOG1(" (Switch)");
		else if (dynamic_cast<osg::Sequence*>(node))
			VTLOG1(" (Sequence)");
		else if (dynamic_cast<osg::Projection*>(node))
			VTLOG1(" (Projection)");
		else if (dynamic_cast<osg::OccluderNode*>(node))
			VTLOG1(" (OccluderNode)");
		else if (dynamic_cast<osg::LightSource*>(node))
			VTLOG1(" (LightSource)");

		else if (dynamic_cast<osg::LOD*>(node))
			VTLOG1(" (LOD)");
		else if (dynamic_cast<osg::CoordinateSystemNode*>(node))
			VTLOG1(" (CoordinateSystemNode)");
		else if (dynamic_cast<osg::ClipNode*>(node))
			VTLOG1(" (ClipNode)");
		else if (dynamic_cast<osg::ClearNode*>(node))
			VTLOG1(" (ClearNode)");
		else if (dynamic_cast<osg::Group*>(node))
			VTLOG1(" (Group)");

		else if (dynamic_cast<osg::Billboard*>(node))
			VTLOG1(" (Billboard)");
		else if (dynamic_cast<osg::Geode*>(node))
			VTLOG1(" (Geode)");

		else if (dynamic_cast<osgParticle::ModularEmitter*>(node))
			VTLOG1(" (Geode)");
		else if (dynamic_cast<osgParticle::ParticleSystemUpdater*>(node))
			VTLOG1(" (Geode)");
		else
			VTLOG1(" (non-node!)");

		VTLOG(" '%s'", node->getName().c_str());

		if (node->getNodeMask() != 0xffffffff)
			VTLOG(" mask=%x", node->getNodeMask());

		if (bExtents)
		{
			const osg::BoundingSphere &bs = node->getBound();
			if (bs._radius != -1)
				VTLOG(" (bs: %.1f %.1f %.1f %1.f)", bs._center[0], bs._center[1], bs._center[2], bs._radius);
		}
		if (bRefCounts)
		{
			VTLOG(" {rc:%d}", node->referenceCount());
		}

		VTLOG1("\n");
	}
	else
		VTLOG1("<null>\n");

	osg::MatrixTransform *mt = dynamic_cast<osg::MatrixTransform*>(node);
	if (mt)
	{
		for (int i = 0; i < indent+1; i++)
			VTLOG1(" ");
		osg::Vec3 v = mt->getMatrix().getTrans();
		VTLOG("[Pos %.2f %.2f %.2f]\n", v.x(), v.y(), v.z());
	}
	osg::Group *grp = dynamic_cast<osg::Group*>(node);
	if (grp)
	{
		for (unsigned int i = 0; i < grp->getNumChildren(); i++)
			vtLogNativeGraph(grp->getChild(i), bExtents, bRefCounts, indent+2);
	}
	osg::Geode *geode = dynamic_cast<osg::Geode*>(node);
	if (geode)
	{
		for (unsigned int i = 0; i < geode->getNumDrawables(); i++)
		{
			osg::Geometry *geo = dynamic_cast<osg::Geometry *>(geode->getDrawable(i));
			if (!geo) continue;

			osg::StateSet *stateset = geo->getStateSet();
			if (!stateset) continue;

			for (int j = 0; j < indent+3; j++)
				VTLOG1(" ");

			VTLOG("drawable %d: geometry %x, stateset %x", i, geo, stateset);

			osg::StateAttribute *state = stateset->getAttribute(osg::StateAttribute::MATERIAL);
			if (state)
			{
				osg::Material *mat = dynamic_cast<osg::Material *>(state);
				if (mat)
					VTLOG(", mat %x", mat);
			}
			VTLOG1("\n");
		}
	}
}

// Diagnostic function to help debugging
void vtLogGraph(vtNode *node, int indent)
{
	for (int i = 0; i < indent; i++)
		VTLOG1(" ");
	if (node)
	{
		VTLOG("<%x>", node);
		if (dynamic_cast<vtHUD*>(node))
			VTLOG1(" (vtHUD)");

		else if (dynamic_cast<vtCamera*>(node))
			VTLOG1(" (vtCamera)");
		else if (dynamic_cast<vtLOD*>(node))
			VTLOG1(" (vtLOD)");
		else if (dynamic_cast<vtDynGeom*>(node))
			VTLOG1(" (vtDynGeom)");
		else if (dynamic_cast<vtMovGeom*>(node))
			VTLOG1(" (vtMovGeom)");
		else if (dynamic_cast<vtGeom*>(node))
			VTLOG1(" (vtGeom)");
		else if (dynamic_cast<vtLight*>(node))
			VTLOG1(" (vtLight)");

		else if (dynamic_cast<vtTransform*>(node))
			VTLOG1(" (vtTransform)");
		else if (dynamic_cast<vtGroup*>(node))
			VTLOG1(" (vtGroup)");
		else if (dynamic_cast<vtNativeNode*>(node))
			VTLOG1(" (vtNativeNode)");
		else if (dynamic_cast<vtNode*>(node))
			VTLOG1(" (vtNode)");
		else
			VTLOG1(" (non-node!)");

		VTLOG(" '%s'\n", node->GetName2());
	}
	else
		VTLOG1("<null>\n");

	vtGroup *grp = dynamic_cast<vtGroup*>(node);
	if (grp)
	{
		for (unsigned int i = 0; i < grp->GetNumChildren(); i++)
			vtLogGraph(grp->GetChild(i), indent+2);
	}
}

