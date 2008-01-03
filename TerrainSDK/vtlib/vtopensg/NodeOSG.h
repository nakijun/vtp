//
// NodeOSG.h
//
// Encapsulate behavior for OpenSG scene graph nodes.
//
// Copyright (c) 2006 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifndef VTOSG_NODEH
	#define VTOSG_NODEH

//OpenSG includes
	#include <OpenSG/OSGNode.h>
	#include <OpenSG/OSGGroup.h>
	#include <OpenSG/OSGRefPtr.h>
	#include <OpenSG/OSGComponentTransform.h>
	#include <OpenSG/OSGMaterialGroup.h>
	#include <OpenSG/OSGDistanceLOD.h>
	#include <OpenSG/OSGDirectionalLight.h>
	#include <OpenSG/OSGPassiveViewport.h>
	#include <OpenSG/OSGDepthClearBackground.h>
	#include <OpenSG/OSGMatrixCamera.h>

	#include "vtOsgDynMesh.h"

/** \addtogroup sg */
/*@{*/

/** Contains information about the primitives in a set of geometry */
struct vtPrimInfo {
	/// Number of vertices which the geometry will draw.
	int Vertices;
	/// Total number of primitives of all types.
	int Primitives;
	/// Number of vertices stored in memory, which may appear more than once in indexed primitives.
	int MemVertices;

	/// Number of Point primitives.
	int Points;
	/// Number of Triangle Strip primitives.
	int TriStrips;
	/// Number of Triangle Fan primitives.
	int TriFans;
	/// Number of total Triangles in all the primitives.
	int Triangles;
	/// Number of Quad primitives.
	int Quads;
	/// Number of Quad Strip primitives.
	int QuadStrips;
	/// Number of Polygon primitives.
	int Polygons;
	/// Number of Line Strip primitives.
	int LineStrips;
	/// Number of Line Segments in all the primitives.
	int LineSegments;
};

class vtNode;

class vtMultiTexture
{
public:
	int	m_iTextureUnit;
	vtNode *m_pNode;
	//osg::ref_ptr<osg::Texture2D> m_pTexture;
};

/**
 * Represents a node in the vtlib scenegraph.  The scenegraph is simply
 * a tree of nodes, with a root node at the top.
 */
class vtNode : public vtNodeBase// EXCEPT, protected osg::Referenced 
{
public:
	virtual void Release();

	// implement vtNodeBase methods
	void SetEnabled(bool bOn);
	bool GetEnabled() const;

	/** Set the name of the node. */
	void SetName2(const char *str);
	/** Get the name of the node. */
	const char *GetName2() const;

	/// Get the Bounding Box of the node, in world coordinates
	void GetBoundBox(FBox3 &box);

	/** Get the Bounding Sphere of the node */
	void GetBoundSphere(FSphere &sphere, bool bGlobal = false);

	/// Get information about the number of display primitives
	void GetPrimCounts(vtPrimInfo &info);

	/// Transform a point from a node's local coordinates to world coordinates
	void LocalToWorld(FPoint3 &point);

	vtGroup *GetParent(int iParent = 0);
	virtual vtNode *Clone(bool bDeep = false);

	vtMultiTexture *AddMultiTexture(int iTextureUnit, vtImage *pImage, int iTextureMode,
										const FPoint2 &scale, const FPoint2 &offset);
	void EnableMultiTexture(vtMultiTexture *mt, bool bEnable);
	bool MultiTextureIsEnabled(vtMultiTexture *mt);

	void SetFog(bool bOn, float start = 0, float end = 10000, const RGBf &color = s_white, enum FogType Type = FM_LINEAR);

	// OSG access
	void SetOsgNode(osg::NodePtr n);
	osg::NodePtr GetOsgNode() {return m_pNode;};
	const osg::NodePtr GetOsgNode() const { return m_pNode;};
	void ApplyVertexRotation(const FPoint3 &axis, float angle);
	void DecorateNativeGraph();
	vtNode *FindNativeNode(const char *pName, bool bDescend = true);

	/// Load a 3D model file
	static vtNode *LoadModel(const char *filename, bool bAllowCache = true, bool bDisableMipmaps = false);
	static void ClearOsgModelCache();
	static bool s_bDisableMipmaps;	// set to disable ALL mipmaps

protected:
	osg::RefPtr<osg::NodePtr> m_pNode;
	//osg::NodePtr m_pNode;

	// Constructor is protected because vtNode is an abstract base class,
	//  not to be instantiated directly.
	vtNode() {;};

	// Destructor is protected so that people will use Release() instead,
	//  to ensure that reference counting is respected.
	virtual ~vtNode() {;};
};

/**
 * This class represents a node which is native to the underlying scene graph
 * libraries, e.g. OSG.
 */
class vtNativeNode : public vtNode {
public:
	vtNativeNode(osg::NodePtr node) { SetOsgNode(node);};
	vtNode *vtNativeNode::FindParentVTNode();

protected:
	// Destructor is protected so that people will use Release() instead,
	//  to ensure that reference counting is respected.
	virtual ~vtNativeNode() {};
};

/**
 * Represents a Group (a node that can have children) in the vtlib Scene Graph.
 */
class vtGroup : public vtNode, public vtGroupBase {
public:
	vtGroup(bool suppress = false);
	virtual vtNode *Clone(bool bDeep = false);
	void CopyFrom(const vtGroup *rhs);
	virtual void Release();

	// implement vtGroupBase methods

	/** Add a node as a child of this Group. */
	virtual void AddChild(vtNode *pChild);

	/** Remove a node as a child of this Group.  If the indicated node is not
	 a child, then this method has no effect. */
	void RemoveChild(vtNode *pChild);

	/** Return a child node, by index. */
	vtNode *GetChild(unsigned int num) const;

	/** Return the number of child nodes */
	unsigned int GetNumChildren() const;

	/** Looks for a descendent node with a given name.  If not found, NULL
	 is returned. */
	const vtNode *FindDescendantByName(const char *name) const;

	/** Return true if the given node is a child of this group. */
	bool ContainsChild(vtNode *pNode) const;

protected:
	// Destructor is protected so that people will use Release() instead,
	//  to ensure that reference counting is respected.
	virtual ~vtGroup() {;};
};

/**
 * A Transform node allows you to apply a transform (scale, rotate, translate)
 * to all its child nodes.
 */
class vtTransform : public vtGroup, public vtTransformBase {
public:
	vtTransform();
	virtual vtNode *Clone(bool bDeep = false);
	void CopyFrom(const vtTransform *rhs);
	void Release();

	// implement vtTransformBase methods

	/** Set this transform to identity (no scale, rotation, or translation). */
	void Identity();

	/** Set the translation component of the transform */
	void SetTrans(const FPoint3 &pos);

	/** Get the translation component of the transform */
	FPoint3 GetTrans() const;

	/** Apply a relative offset (translation) to the transform, in the frame
		of its parent. */
	void Translate1(const FPoint3 &pos);

	/** Apply a relative offset (translation) to the transform, in its own
		frame of reference. */
	void TranslateLocal(const FPoint3 &pos);

	/** Rotate around a given axis by a given angle, in radians. */
	void Rotate2(const FPoint3 &axis, double angle);

	/** Similar to Rotate2, but operates in the local frame of reference. */
	void RotateLocal(const FPoint3 &axis, double angle);

	/** Similar to Rotate2, but the rotation occurs in the parent's frame of
		reference. */
	void RotateParent(const FPoint3 &axis, double angle);

	/** Get the orientation (rotate relative to default) of the transform */
	FQuat GetOrient() const;

	/** Get the direction, which by convention is shorthand for the -Z axis
		of the local frame.  For oriented objects such as the camera, this
		is the direction the object is facing. */
	FPoint3 GetDirection() const;

	/** Set the direction, which by convention is shorthand for the -Z axis
		of the local frame.  For oriented objects such as the camera, this
		is the direction the object is facing. */
	void SetDirection(const FPoint3 &point, bool bPitch = true);

	/** Scale (stretch) by given factors in the x,y,z dimensions. */
	void Scale3(float x, float y, float z);

	void SetTransform1(const FMatrix4 &mat);
	void GetTransform1(FMatrix4 &mat) const;

	/** Rotate the object such that it points toward a given point.  By
		convention, this means that the object's -Z axis points in the
		desired direction. */
	void PointTowards(const FPoint3 &point, bool bPitch = true);

	osg::TransformPtr GetOsgTransform() { return m_pTransform;}
	const osg::TransformPtr GetOsgTransform() const { return m_pTransform;}

protected:
	// OSG-specific Implementation
	osg::TransformPtr m_pTransform;

	// Destructor is protected so that people will use Release() instead,
	//  to ensure that reference counting is respected.
	virtual ~vtTransform() {};
};

/**
 * A Light node is placed into the scene graph to illumninate all
 * lit geometry with vertex normals.
 */
class vtLight : public vtGroup {
public:
	vtLight();
	virtual vtNode *Clone(bool bDeep = false);
	void CopyFrom(const vtLight *rhs);
	void Release();

	void SetDiffuse(const RGBf &color);
	RGBf GetDiffuse() const;
	void SetAmbient(const RGBf &color);
	RGBf GetAmbient() const;
	void SetSpecular(const RGBf &color);
	RGBf GetSpecular() const;

	// provide override to catch this state
	virtual void SetEnabled(bool bOn);

	//set the transform node of the directional light
	void SetBeacon (vtTransform *vtTrans);

	osg::DirectionalLightPtr GetOsgLight() const { return m_pLight;}

protected:
	osg::DirectionalLightPtr m_pLight;

	// Destructor is protected so that people will use Release() instead,
	//  to ensure that reference counting is respected.
	virtual ~vtLight() {};
};

class vtTextMesh;

/** The vtGeom class represents a Geometry Node which can contain any number
	of visible vtMesh objects.
	\par
	A vtGeom also manages a set of Materials (vtMaterial).  Each contained
	mesh is assigned one of these materials, by index.
	\par
	This separation (Group/Mesh) provides the useful ability to define a vtMesh
	once in memory, and have multiple vtGeom nodes which contain it, which
	permits a large number of visual instances (each with potentially different
	material and transform) with very little memory cost.
 */
class vtGeom : public vtNode {
public:
	vtGeom();
	virtual vtNode *Clone(bool bDeep = false);
	void CopyFrom(const vtGeom *rhs);
	void Release();

	/** Add a mesh to this geometry.
		\param pMesh The mesh to add
		\param iMatIdx The material index for this mesh, which is an index
			into the material array of the geometry. */
	void AddMesh(vtMesh *pMesh, int iMatIdx);

	/** Remove a mesh from the geomtry.  Has no effect if the mesh is not
		currently contained. */
	void RemoveMesh(vtMesh *pMesh);

	/** Add a text mesh to this geometry.
		\param pMesh The mesh to add
		\param iMatIdx The material index for this mesh, which is an index
			into the material array of the geometry. */
	void AddTextMesh(vtTextMesh *pMesh, int iMatIdx);

	/** Return the number of contained meshes. */
	unsigned int GetNumMeshes() const;

	/** Return a contained vtMesh by index. */
	vtMesh *GetMesh(int i) const;

	/** Return a contained vtTextMesh by index. */
	vtTextMesh *GetTextMesh(int i) const;

	virtual void SetMaterials(const class vtMaterialArray *mats);
	const vtMaterialArray   *GetMaterials() const;

	vtMaterial *GetMaterial(int idx);

	void SetMeshMatIndex(vtMesh *pMesh, int iMatIdx);

protected:
	// Destructor is protected so that people will use Release() instead,
	//  to ensure that reference counting is respected.
	virtual ~vtGeom() {};

	const vtMaterialArray *m_pMaterialArray;
};

/**
 * A utility class which simply wraps a geometry (vtGeom) inside a
 * transform (vtTransform) so that you can move it.
 */
class vtMovGeom : public vtTransform {
public:
	vtMovGeom(vtGeom *pContained) : vtTransform()
	{
		m_pGeom = pContained;
		AddChild(m_pGeom);
	}
	vtGeom  *m_pGeom;
};


/**
 * vtDynGeom extends the vtGeom class with the ability to have dynamic geometry
 * which changes every frame.  The most prominent use of this feature is to do
 * Continuous Level of Detail (CLOD) for terrain.
 * \par
 * To implement, you must create your own subclass and override the following
 * methods:
 * - DoRender()
 * - DoCalcBoundBox()
 * - DoCull()
 * \par
 * Many helpful methods are provided to make doing your own view culling very easy:
 * - IsVisible(sphere)
 * - IsVisible(triangle)
 * - IsVisible(point)
 * \par
 * \see vtDynTerrainGeom
 */
class vtDynGeom : public vtGeom {
public:
	vtDynGeom();

	// Tests a sphere or triangle, and return one of:
	//	0				- not in view
	//  VT_Visible		- partly in view
	//  VT_AllVisible	- entirely in view
	//
	int IsVisible(const FSphere &sphere) const;
	int IsVisible(const FPoint3 &point0,
				  const FPoint3 &point1,
				  const FPoint3 &point2,
				  const float fTolerance = 0.0f) const;
	int IsVisible(const FPoint3 &point, float radius);

	// Tests a single point, returns true if in view
	bool IsVisible(const FPoint3 &point) const;

	// vt methods (must be overriden)
	virtual void DoRender() = 0;
	virtual void DoCalcBoundBox(FBox3 &box) = 0;
	virtual void DoCull(const vtCamera *pCam) = 0;

	void ApplyMaterial(vtMaterial *mat);
	static void UnApplyMaterial();

	// A handy shortcut to the current clipping planes
	FPlane      *m_pPlanes;

protected:
	// remember the last applied material, for UnApply
	static vtMaterial *s_pCurrentMaterial;

	osg::vtOsgDynMeshPtr m_pDynMesh; //this is a node CORE
	osg::NodePtr m_pDynNode;   //node which encapsules the core
};

//////////////////////////////////////////////////

/**
 * An LOD node controls the visibility of its child nodes.
 *
 * You should set a distance value (range) for each child, which determines
 * at what distance from the camera a node should be rendered.
 */
class vtLOD : public vtGroup {
public:
	vtLOD();
	void Release();

	void AddChild(vtNode *pChild);
	void SetRanges(float *ranges, int nranges);
	void SetCenter(FPoint3 &center);

protected:
	//{
	osg::NodePtr m_pNode;
	osg::DistanceLODPtr m_pLOD;
	//}
	virtual ~vtLOD() {};
};

/**
 * A Camera is analogous to a physical camera: it description the location
 * of a point from which the scene is rendered.  It can either be a
 * perspective or orthographic camera, and it very easy to control
 * since it inherits all the methods of a transform (vtTransform).
 *
 * Although the camera is a node, this is purely for convenience.  You
 * do not have to place the camera node in your scene graph.  You may,
 * however, tell your scene (vtScene) which camera to use.  The scene
 * produces a default camera which is used unless you tell it otherwise.
 */
class vtCamera : public vtTransform {
public:
	vtCamera();
	virtual vtNode *Clone(bool bDeep = false);
	void CopyFrom(const vtCamera *rhs);

	void SetHither(float f);
	float GetHither() const;
	void SetYon(float f);
	float GetYon() const;
	void SetFOV(float f);
	float GetFOV() const;
	float GetVertFOV() const;

	void SetOrtho(bool bOrtho);
	bool IsOrtho() const;
	void SetWidth(float f);
	float GetWidth() const;

	void ZoomToSphere(const FSphere &sphere);

protected:
	float m_fFOV;
	float m_fHither;
	float m_fYon;

	bool m_bOrtho;
	float m_fWidth;

	virtual ~vtCamera() {};
};

/**
 * A HUD ("heads-up display") is a group whose whose children are transformed
 * to be drawn in window coordinates, rather than world coordinates.
 */
class vtHUD : public vtGroup {
public:
	vtHUD(bool bPixelCoords = true);
	virtual vtNode *Clone(bool bDeep = false);
	void CopyFrom(const vtHUD *rhs);
	void Release();

	void SetWindowSize(int w, int h);

protected:
	OSG::Matrix4f makeOrtho2D (float left, float right, float bottom, float top);

	bool m_bPixelCoords;
	OSG::Matrix4f m_projection;
	OSG::MatrixCameraPtr m_camera;
	OSG::PassiveViewportPtr m_pHudViewport;
		
	virtual ~vtHUD() {};
};

/**
 * This class which contains a geometry with a single textured rectangle
 *  mesh.  It is particularly useful in conjunction with vtHUD, for
 *  superimposing a single image on the window.
 */
class vtImageSprite {
public:
	vtImageSprite();
	~vtImageSprite();
	bool Create(const char *szTextureName, bool bBlending = false);
	bool Create(vtImage *pImage, bool bBlending = false);
	IPoint2 GetSize() const { return m_Size;};
	void SetPosition(float l, float t, float r, float b, float rot = 0.0f);
	void SetImage(vtImage *image);
	vtNode *GetNode() const { return m_pGeom;};

protected:
	vtMaterialArray *m_pMats;
	vtGeom *m_pGeom;
	vtMesh *m_pMesh;
	IPoint2 m_Size;
};

/* Intersection method */
/**
 * This class describes a single point at which vtIntersect has determined
 * a line has intersected some geometry.  At this point, vtHit tells
 * you the node that was hit, the 3D point of intersection, and the
 * distance from the start of the line.
 */
struct vtHit {
	bool operator < (const vtHit &i) const { return distance < i.distance;};
	vtNode *node;
	FPoint3 point;
	float distance;
};
typedef std::vector<vtHit> vtHitList;
int vtIntersect(vtNode *pTop, const FPoint3 &start, const FPoint3 &end,
				vtHitList &hitlist, bool bLocalCoords = false);
void vtLogGraph(vtNode *node, int indent=0);
void vtLogNativeGraph(osg::NodePtr node, bool bExtents = false, bool bRefCounts = false, int indent=0);

/*@}*/	// Group sg

#endif	// VTOSG_NODEH

