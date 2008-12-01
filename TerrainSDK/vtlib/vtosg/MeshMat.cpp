//
// MeshMat.cpp - Meshes and Materials for vtlib-OSG
//
// Copyright (c) 2001-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#include "vtlib/vtlib.h"
#include "vtdata/vtLog.h"

#include <osg/LineWidth>
#include <osg/PolygonMode>
#include <osg/Depth>
using namespace osg;

///////////////////////////////////

bool vtMaterial::s_bTextureCompression = false;

#define SA_ON	StateAttribute::ON
#define SA_OFF	StateAttribute::OFF

vtMaterial::vtMaterial() : vtMaterialBase()
{
	m_pImage = NULL;
	m_pStateSet = new StateSet;
	m_pMaterial = new Material;
	m_pStateSet->setAttributeAndModes(m_pMaterial.get());

	// Not sure why this is required (should be the default!)
	m_pStateSet->setMode(GL_DEPTH_TEST, SA_ON);
}

vtMaterial::~vtMaterial()
{
	// do these dereferences manually, although it's not really required
	m_pMaterial = NULL;
	m_pTexture = NULL;
	m_pStateSet = NULL;
	m_pBlendFunc = NULL;
	m_pAlphaFunc = NULL;

	// remove convenience pointer: this is not a dereference
	m_pImage = NULL;
}

/**
 * Set the diffuse color of this material.
 *
 * \param r,g,b	The rgb value (0.0 to 1.0) of this material
 * \param a		For a material with transparency enabled, the alpha component
 * of the diffuse color determines the overall transparency of the material.
 * This value ranges from 0 (totally transparent) to 1 (totally opaque.)
 *
 */
void vtMaterial::SetDiffuse(float r, float g, float b, float a)
{
	m_pMaterial->setDiffuse(FAB, Vec4(r, g, b, a));

	if (a < 1.0f)
		m_pStateSet->setMode(GL_BLEND, SA_ON);
}
/**
 * Get the diffuse color of this material.
 */
RGBAf vtMaterial::GetDiffuse() const
{
	Vec4 col = m_pMaterial->getDiffuse(FAB);
	return RGBAf(col[0], col[1], col[2], col[3]);
}

/**
 * Set the specular color of this material.
 */
void vtMaterial::SetSpecular(float r, float g, float b)
{
	m_pMaterial->setSpecular(FAB, Vec4(r, g, b, 1.0f));
}
/**
 * Get the specular color of this material.
 */
RGBf vtMaterial::GetSpecular() const
{
	Vec4 col = m_pMaterial->getSpecular(FAB);
	return RGBf(col[0], col[1], col[2]);
}

/**
 * Set the ambient color of this material.
 */
void vtMaterial::SetAmbient(float r, float g, float b)
{
	m_pMaterial->setAmbient(FAB, Vec4(r, g, b, 1.0f));
}
/**
 * Get the ambient color of this material.
 */
RGBf vtMaterial::GetAmbient() const
{
	Vec4 col = m_pMaterial->getAmbient(FAB);
	return RGBf(col[0], col[1], col[2]);
}

/**
 * Set the emissive color of this material.
 */
void vtMaterial::SetEmission(float r, float g, float b)
{
	m_pMaterial->setEmission(FAB, Vec4(r, g, b, 1.0f));
}
/**
 * Get the emissive color of this material.
 */
RGBf vtMaterial::GetEmission() const
{
	Vec4 col = m_pMaterial->getEmission(FAB);
	return RGBf(col[0], col[1], col[2]);
}

/**
 * Set the backface culling property of this material.
 */
void vtMaterial::SetCulling(bool bCulling)
{
	m_pStateSet->setMode(GL_CULL_FACE, bCulling ? SA_ON : SA_OFF);
}
/**
 * Get the backface culling property of this material.
 */
bool vtMaterial::GetCulling() const
{
	StateAttribute::GLModeValue m;
	m = m_pStateSet->getMode(GL_CULL_FACE);
	return (m == SA_ON);
}

/**
 * Set the lighting property of this material.
 */
void vtMaterial::SetLighting(bool bLighting)
{
	m_pStateSet->setMode(GL_LIGHTING, bLighting ? SA_ON : SA_OFF);
}
/**
 * Get the lighting property of this material.
 */
bool vtMaterial::GetLighting() const
{
	StateAttribute::GLModeValue m;
	m = m_pStateSet->getMode(GL_LIGHTING);
	return (m == SA_ON);
}

/**
 * Set the transparent property of this material.
 *
 * \param bOn True to turn on transparency (blending).
 * \param bAdd True for additive blending.
 */
void vtMaterial::SetTransparent(bool bOn, bool bAdd)
{
//	m_pStateSet->setMode(GL_BLEND, bOn ? SA_ON : SA_OFF);
	if (bOn)
	{
		if (!m_pBlendFunc.valid())
			m_pBlendFunc = new BlendFunc;
		m_pStateSet->setAttributeAndModes(m_pBlendFunc.get(), SA_ON);
		if (!m_pAlphaFunc.valid())
			m_pAlphaFunc = new AlphaFunc;
		m_pAlphaFunc->setFunction(AlphaFunc::GEQUAL,0.05f);
		m_pStateSet->setAttributeAndModes(m_pAlphaFunc.get(), SA_ON );
		m_pStateSet->setRenderingHint(StateSet::TRANSPARENT_BIN);

#if MAYBE_SOMEDAY
		// RJ says he needed this to make multiple transparent surfaces work
		//  properly.  In general, the transparent bin takes care of that,
		//  but there are cases where polygons (sorted by center) can end up
		//  in the wrong order.
		ref_ptr<Depth> pDepth  = new osg::Depth;
		pDepth->setWriteMask(false);
		m_pStateSet->setAttribute(pDepth.get());
#endif
	}
	else
	{
		m_pStateSet->setMode(GL_BLEND, SA_OFF);
		m_pStateSet->setRenderingHint( StateSet::OPAQUE_BIN );
	}

	if (bAdd)
	{
		if (!m_pBlendFunc.valid())
			m_pBlendFunc = new BlendFunc;
//		m_pBlendFunc->setFunction(GL_ONE, GL_ONE);
//		m_pBlendFunc->setFunction(GL_SRC_COLOR, GL_DST_COLOR);
		m_pBlendFunc->setFunction(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
		m_pStateSet->setAttribute(m_pBlendFunc.get());
	}
}
/**
 * Get the transparent property of this material.
 */
bool vtMaterial::GetTransparent() const
{
	// OSG 0.8.45 and before
//	StateAttribute::GLModeValue m = m_pStateSet->getMode(StateAttribute::TRANSPARENCY);
	// OSG 0.9.0 onwards
	StateAttribute::GLModeValue m;
	m = m_pStateSet->getMode(GL_BLEND);
	return (m == SA_ON);
}


/**
 * Set the wireframe property of this material.
 *
 * \param bOn True to turn on wireframe.
 */
void vtMaterial::SetWireframe(bool bOn)
{
	if (bOn)
	{
		PolygonMode *pm = new PolygonMode();
		pm->setMode(PolygonMode::FRONT_AND_BACK, PolygonMode::LINE);
		m_pStateSet->setAttributeAndModes(pm, StateAttribute::OVERRIDE | SA_ON);
	}
	else
	{
		// turn wireframe off
		m_pStateSet->setMode(GL_POLYGON_MODE, StateAttribute::OFF);
	}
}
/**
 * Get the wireframe property of this material.
 */
bool vtMaterial::GetWireframe() const
{
	// OSG 0.9.0
	StateAttribute::GLModeValue m;
	m = m_pStateSet->getMode(StateAttribute::POLYGONMODE);
	return (m == SA_ON);
}

/**
 * Set the texture for this material.
 */
void vtMaterial::SetTexture(vtImage *pImage)
{
	if (!m_pTexture)
		m_pTexture = new Texture2D;

	// this stores a reference so that it won't get deleted without this material's permission
	m_pTexture->setImage(pImage->GetOsgImage());

	// also store a convenience pointer
	m_pImage = pImage;

	/** "Note, If the mode is set USE_IMAGE_DATA_FORMAT, USE_ARB_COMPRESSION,
	 * USE_S3TC_COMPRESSION the internalFormat is automatically selected, and
	 * will overwrite the previous _internalFormat. */
//	m_pTexture->setInternalFormatMode(osg::Texture::USE_S3TC_DXT1_COMPRESSION);
	if (s_bTextureCompression)
		//m_pTexture->setInternalFormatMode(osg::Texture::USE_ARB_COMPRESSION);
		m_pTexture->setInternalFormatMode(osg::Texture::USE_S3TC_DXT3_COMPRESSION);

	// From the OSG list: "Why doesn't the OSG deallocate image buffer right
	// *after* a glTextImage2D?
	// By default the OSG doesn't do it bacause the user may actually want to
	// do things with the image after its been bound.  You can make the
	// osg::Texture classes unref their images automatically by doing:
	// texture->setUnRefImageDataAfterApply(true);

	// So i tried this, but it doesn't seem to have any affect on runtime memory
	//  footprint:
//	m_pTexture->setUnRefImageDataAfterApply(true);

	m_pStateSet->setTextureAttributeAndModes(0, m_pTexture.get(), SA_ON);
}

/**
 * Loads and sets the texture for a material.
 */
bool vtMaterial::SetTexture2(const char *szFilename)
{
	vtImage *image = vtImageRead(szFilename);
	if (image)
	{
		SetTexture(image);
		image->Release();	// don't hold on to it; pass ownership to the material
	}
	return (image != NULL);
}


/**
 * Returns the texture (image) associated with a material.
 */
vtImage	*vtMaterial::GetTexture() const
{
	// It is valid to return a non-const pointer to the image, since the image
	//  can be modified entirely independently of the material.
	return const_cast<vtImage*>(m_pImage);
}

/**
 * Call this method to tell vtlib that you have modified the contents of a
 *  texture so it needs to be sent again to the graphics card.
 */
void vtMaterial::ModifiedTexture()
{
	if (!m_pTexture)
		return;

	// Two steps: first we tell the Texture it's changed, then we tell the
	//  Image it's changed.
	m_pTexture->dirtyTextureObject();

	// OSG calls a modified image 'dirty'
	m_pTexture->getImage()->dirty();
}


/**
 * Set the texture clamping property for this material.
 */
void vtMaterial::SetClamp(bool bClamp)
{
	if (!m_pTexture)
		return;
	if (bClamp)
	{
		// TODO: try   texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
		m_pTexture->setWrap(Texture::WRAP_S, Texture::CLAMP);
		m_pTexture->setWrap(Texture::WRAP_T, Texture::CLAMP);
	}
	else
	{
		m_pTexture->setWrap(Texture::WRAP_S, Texture::REPEAT);
		m_pTexture->setWrap(Texture::WRAP_T, Texture::REPEAT);
	}
}

/**
 * Get the texture clamping property of this material.
 */
bool vtMaterial::GetClamp() const
{
	if (!m_pTexture)
		return false;
	Texture::WrapMode w = m_pTexture->getWrap(Texture::WRAP_S);
	return (w == Texture::CLAMP);
}

/**
 * Set the texture mipmapping property for this material.
 */
void vtMaterial::SetMipMap(bool bMipMap)
{
	if (!m_pTexture)
		return;
	if (bMipMap)
		m_pTexture->setFilter(Texture::MIN_FILTER, Texture::LINEAR_MIPMAP_LINEAR);
	else
		m_pTexture->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
}

/**
 * Get the texture mipmapping property of this material.
 */
bool vtMaterial::GetMipMap() const
{
	if (!m_pTexture)
		return false;
	Texture::FilterMode m = m_pTexture->getFilter(Texture::MIN_FILTER);
	return (m == Texture::LINEAR_MIPMAP_LINEAR);
}


/////////////////////////////////////////////////////////////////////////////
// vtMaterialArray
//

vtMaterialArray::vtMaterialArray()
{
	ref();		// artficially set refcount to 1
}

vtMaterialArray::~vtMaterialArray()
{
}

void vtMaterialArray::Release()
{
	unref();	// trigger self-deletion if no more references
}

/**
 * Adds a material to this material array.
 *
 * \return The index of the material that was added.
 */
int vtMaterialArray::AppendMaterial(vtMaterial *pMat)
{
	// nothing special to do
	return Append(pMat);
}


/////////////////////////////////////////////////////////////////////////////
// vtMesh
//

/**
 * Construct a Mesh.
 * A Mesh is a container for a set of vertices and primitives.
 *
 * \param ePrimType The type of primitive this mesh will contain.  Allowed
 *		values are:
 *		- vtMesh::POINTS
 *		- vtMesh::LINES
 *		- vtMesh::LINE_STRIP
 *		- vtMesh::TRIANGLES
 *		- vtMesh::TRIANGLE_STRIP
 *		- vtMesh::TRIANGLE_FAN
 *		- vtMesh::QUADS
 *		- vtMesh::POLYGON
 *
 * \param VertType Flags which indicate what type of information is stored
 *		with each vertex.  This can be any combination of the following bit
 *		flags:
 *		- VT_Normals - a normal per vertex.
 *		- VT_Colors - a color per vertex.
 *		- VT_TexCoords - a texture coordinate (UV) per vertex.
 *
 * \param NumVertices The expected number of vertices that the mesh will
 *		contain.  If more than this number of vertices are added, the mesh
 *		will automatically grow to contain them.  However it is more
 *		efficient if you know the number at creation time and pass it in
 *		this parameter.
 */
vtMesh::vtMesh(enum PrimType ePrimType, int VertType, int NumVertices) :
	vtMeshBase(ePrimType, VertType, NumVertices)
{
	ref();		// artficially set refcount to 1

	m_pGeometry = new Geometry();

	// set backpointer so we can find ourselves later
	// also increases our own refcount
	m_pGeometry->setUserData(this);

	m_Vert = new Vec3Array;
	m_Vert->reserve(NumVertices);
	m_pGeometry->setVertexArray(m_Vert.get());

	m_Index = new UIntArray;
	m_Index->reserve(NumVertices);
	m_pGeometry->setVertexIndices(m_Index.get());

	if (VertType & VT_Normals)
	{
		m_Norm = new Vec3Array;
		m_Norm->reserve(NumVertices);
		m_pGeometry->setNormalArray(m_Norm.get());
		m_pGeometry->setNormalIndices(m_Index.get());
		m_pGeometry->setNormalBinding(Geometry::BIND_PER_VERTEX);
	}
	if (VertType & VT_Colors)
	{
		m_Color = new Vec4Array;
		m_Color->reserve(NumVertices);
		m_pGeometry->setColorArray(m_Color.get());
		m_pGeometry->setColorIndices(m_Index.get());
		m_pGeometry->setColorBinding(Geometry::BIND_PER_VERTEX);
	}
	if (VertType & VT_TexCoords)
	{
		m_Tex = new Vec2Array;
		m_Tex->reserve(NumVertices);
		m_pGeometry->setTexCoordArray(0, m_Tex.get());
		m_pGeometry->setTexCoordIndices(0, m_Index.get());
	}

	switch (ePrimType)
	{
	case POINTS:
		m_pPrimSet = new DrawArrays(PrimitiveSet::POINTS, 0, NumVertices);
		break;
	case LINES:
		m_pPrimSet = new DrawArrays(PrimitiveSet::LINES, 0, NumVertices);
		break;
	case TRIANGLES:
		m_pPrimSet = new DrawArrays(PrimitiveSet::TRIANGLES, 0, NumVertices);
		break;
	case QUADS:
		m_pPrimSet = new DrawArrays(PrimitiveSet::QUADS, 0, NumVertices);
		break;
	case LINE_STRIP:
		m_pPrimSet = new DrawArrayLengths(PrimitiveSet::LINE_STRIP);
		break;
	case TRIANGLE_STRIP:
		m_pPrimSet = new DrawArrayLengths(PrimitiveSet::TRIANGLE_STRIP);
		break;
	case TRIANGLE_FAN:
		m_pPrimSet = new DrawArrayLengths(PrimitiveSet::TRIANGLE_FAN);
		break;
	case POLYGON:
		m_pPrimSet = new DrawArrayLengths(PrimitiveSet::POLYGON);
		break;
	}
	m_pGeometry->addPrimitiveSet(m_pPrimSet.get());
}

void vtMesh::Release()
{
	unref();	// trigger self-deletion if no more references
	if (_refCount == 1)	// no more references except from m_pGeometry
		// explicit dereference. if Release is not called, this dereference should
		//  also occur implicitly in the destructor
		m_pGeometry = NULL;
}


// Override with ability to get OSG bounding box
void vtMesh::GetBoundBox(FBox3 &box) const
{
	const BoundingBox &osg_box = m_pGeometry->getBound();
	s2v(osg_box, box);
}


/**
 * Add a triangle.
 *  p0, p1, p2 are the indices of the vertices of the triangle.
 */
void vtMesh::AddTri(int p0, int p1, int p2)
{
	m_Index->push_back(p0);
	m_Index->push_back(p1);
	m_Index->push_back(p2);
	if (m_ePrimType == TRIANGLES)
	{
		DrawArrays *pDrawArrays = dynamic_cast<DrawArrays*>(m_pPrimSet.get());

		// OSG's "Count" is the number of indices
		pDrawArrays->setCount(m_Index->size());
	}
	else if (m_ePrimType == TRIANGLE_STRIP || m_ePrimType == TRIANGLE_FAN)
	{
		DrawArrayLengths *pDrawArrayLengths = dynamic_cast<DrawArrayLengths*>(m_pPrimSet.get());
		pDrawArrayLengths->push_back(3);
	}
}

/**
 * Add a triangle fan with up to 6 points (center + 5 points).  The first 3
 * arguments are required, the rest are optional.  A fan will be created
 * with as many point indices as you pass.
 */
void vtMesh::AddFan(int p0, int p1, int p2, int p3, int p4, int p5)
{
	DrawArrayLengths *pDrawArrayLengths = dynamic_cast<DrawArrayLengths*>(m_pPrimSet.get());
	int len = 2;

	m_Index->push_back(p0);
	m_Index->push_back(p1);

	if (p2 != -1) { m_Index->push_back(p2); len = 3; }
	if (p3 != -1) { m_Index->push_back(p3); len = 4; }
	if (p4 != -1) { m_Index->push_back(p4); len = 5; }
	if (p5 != -1) { m_Index->push_back(p5); len = 6; }

	pDrawArrayLengths->push_back(len);
}

/**
 * Add a triangle fan with any number of points.
 *	\param idx An array of vertex indices for the fan.
 *	\param iNVerts the number of vertices in the fan.
 */
void vtMesh::AddFan(int *idx, int iNVerts)
{
	DrawArrayLengths *pDrawArrayLengths = dynamic_cast<DrawArrayLengths*>(m_pPrimSet.get());
	if (!pDrawArrayLengths)
	{
		VTLOG("Error, calling AddFan when primtype is %d\n", m_ePrimType);
		return;
	}
	for (int i = 0; i < iNVerts; i++)
		m_Index->push_back(idx[i]);

	pDrawArrayLengths->push_back(iNVerts);
}

/**
 * Adds an indexed triangle strip to the mesh.
 *
 * \param iNVerts The number of vertices in the strip.
 * \param pIndices An array of the indices of the vertices in the strip.
 */
void vtMesh::AddStrip(int iNVerts, unsigned short *pIndices)
{
	DrawArrayLengths *pDrawArrayLengths = dynamic_cast<DrawArrayLengths*>(m_pPrimSet.get());
	if (!pDrawArrayLengths)
	{
		VTLOG("Error, calling AddStrip when primtype is %d\n", m_ePrimType);
		return;
	}
	for (int i = 0; i < iNVerts; i++)
		m_Index->push_back(pIndices[i]);

	pDrawArrayLengths->push_back(iNVerts);
}

/**
 * Add a single line primitive to a mesh.
 *	\param p0, p1	The indices of the two vertices of the line.
 */
void vtMesh::AddLine(int p0, int p1)
{
	m_Index->push_back(p0);
	m_Index->push_back(p1);

	if (m_ePrimType == LINES)
	{
		DrawArrays *pDrawArrays = dynamic_cast<DrawArrays*>(m_pPrimSet.get());
		pDrawArrays->setCount(m_Index->size());
	}
	else if (m_ePrimType == LINE_STRIP)
	{
		DrawArrayLengths *pDrawArrayLengths = dynamic_cast<DrawArrayLengths*>(m_pPrimSet.get());
		pDrawArrayLengths->push_back(2);
	}
}

/**
 * Add a single line primitive to a mesh.
 *	\param pos1, pos2	The positions of the two vertices of the line.
 *  \return The index of the first vertex added.
 */
int vtMesh::AddLine(const FPoint3 &pos1, const FPoint3 &pos2)
{
	int p0 = AddVertex(pos1);
	int p1 = AddVertex(pos2);
	AddLine(p0, p1);
	return p0;
}

/**
 * Add a triangle.
 *  p0, p1, p2 are the indices of the vertices of the triangle.
 */
void vtMesh::AddQuad(int p0, int p1, int p2, int p3)
{
	DrawArrays *pDrawArrays = dynamic_cast<DrawArrays*>(m_pPrimSet.get());
	if (!pDrawArrays)
	{
		VTLOG("Error, calling AddQuad when primtype is %d\n", m_ePrimType);
		return;
	}
	m_Index->push_back(p0);
	m_Index->push_back(p1);
	m_Index->push_back(p2);
	m_Index->push_back(p3);
	pDrawArrays->setCount(m_Index->size());
}

unsigned int vtMesh::GetNumVertices() const
{
	return m_Vert->size();
}

/**
 * Set the position of a vertex.
 *	\param i	Index of the vertex.
 *	\param p	The position.
 */
void vtMesh::SetVtxPos(unsigned int i, const FPoint3 &p)
{
	Vec3 s;
	v2s(p, s);

	if (i >= (int)m_Vert->size())
		m_Vert->resize(i + 1);

	m_Vert->at(i) = s;

	if (m_ePrimType == GL_POINTS)
	{
		if (i >= (int)m_Index->size())
			m_Index->resize(i + 1);

		m_Index->at(i) = i;
		DrawArrays *pDrawArrays = dynamic_cast<DrawArrays*>(m_pPrimSet.get());
		pDrawArrays->setCount(m_Vert->size());
	}
}

/**
 * Get the position of a vertex.
 */
FPoint3 vtMesh::GetVtxPos(unsigned int i) const
{
	FPoint3 p;
	s2v( (Vec3)m_Vert->at(i), p);
	return p;
}

/**
 * Set the normal of a vertex.  This is used for lighting, if the mesh
 *	is used with a material with lighting enabled.  Generally you will
 *	want to use a vector of unit length.
 *
 *	\param i	Index of the vertex.
 *	\param norm	The normal vector.
 */
void vtMesh::SetVtxNormal(unsigned int i, const FPoint3 &norm)
{
	Vec3 s;
	v2s(norm, s);

	if (i >= (int)m_Norm->size())
		m_Norm->resize(i + 1);

	m_Norm->at(i) = s;
}

/**
 * Get the normal of a vertex.
 */
FPoint3 vtMesh::GetVtxNormal(unsigned int i) const
{
	FPoint3 p;
	s2v( (Vec3)m_Norm->at(i), p);
	return p;
}

/**
 * Set the color of a vertex.  This color multiplies with the color of the
 *	material used with the mesh, so if you want the vertex color to be
 *	dominant, use a white material.
 *
 *	\param i		Index of the vertex.
 *	\param color	The color.
 */
void vtMesh::SetVtxColor(unsigned int i, const RGBAf &color)
{
	if (m_iVtxType & VT_Colors)
	{
		Vec4 s;
		v2s(color, s);

		if (i >= (int)m_Color->size())
			m_Color->resize(i + 1);

		m_Color->at(i) = s;
	}
}

/**
 * Get the color of a vertex.
 */
RGBAf vtMesh::GetVtxColor(unsigned int i) const
{
	if (m_iVtxType & VT_Colors)
	{
		RGBAf p;
		s2v( (Vec4)m_Color->at(i), p);
		return p;
	}
	return RGBf(0,0,0);
}

/**
 * Set the texture coordinates of a vertex.  Generally these values are
 *	in the range of 0 to 1, although you can use higher values if you want
 *	repeating tiling.  The components of the texture coordinates are
 *  usually called "u" and "v".
 *
 *	\param i	Index of the vertex.
 *	\param uv	The texture coordinate.
 */
void vtMesh::SetVtxTexCoord(unsigned int i, const FPoint2 &uv)
{
	if (m_iVtxType & VT_TexCoords)
	{
		Vec2 s;
		v2s(uv, s);

		// Not sure whether I need this
		if (i >= (int)m_Tex->size())
			m_Tex->resize(i + 1);

		(*m_Tex)[i] = s;
	}
}

/**
 * Get the texture coordinates of a vertex.
 */
FPoint2 vtMesh::GetVtxTexCoord(unsigned int i) const
{
	if (m_iVtxType & VT_TexCoords)
	{
		FPoint2 p;
		s2v( (Vec2)m_Tex->at(i), p);
		return p;
	}
	return FPoint2(0,0);
}

int vtMesh::GetNumPrims() const
{
	return m_pPrimSet->getNumPrimitives();
}

/**
 * Set whether to allow rendering optimization of this mesh.  With OpenGL,
 *	this optimization is called a "display list", which increases the speed
 *	of rendering by creating a special representation of the mesh the first
 *	time it is drawn.  The tradeoff is that subsequent changes to the mesh
 *	are not applied unless you call ReOptimize().
 *
 *	\param bAllow	True to allow optimization.  The default is true.
 */
void vtMesh::AllowOptimize(bool bAllow)
{
	m_pGeometry->setUseDisplayList(bAllow);
}

/**
 * For a mesh with rendering optimization enabled, forces an update of the
 *	optimized representation.
 */
void vtMesh::ReOptimize()
{
	m_pGeometry->dirtyDisplayList();
	m_pGeometry->dirtyBound();
}

/**
 * Set the line width, in pixels, for this mesh's geometry.
 *
 * You should call this method _after_ the mesh has been added to some
 *  geometry with vtGeom::AddMesh()
 * (this requirement was found with the OSG flavor of vtlib.)
 */
void vtMesh::SetLineWidth(float fWidth)
{
	osg::LineWidth *lws = new osg::LineWidth;
	osg::StateSet *ss = m_pGeometry->getOrCreateStateSet();

	lws->setWidth(fWidth);
	ss->setAttributeAndModes(lws,
		osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON);
}

/**
 * Set the normals of the vertices by combining the normals of the
 * surrounding faces.  This requires going through all the primitives
 * to average their contribution to each vertex.
 */
void vtMesh::SetNormalsFromPrimitives()
{
	if (m_ePrimType != vtMesh::TRIANGLES &&
		m_ePrimType != vtMesh::QUADS &&
		m_ePrimType != vtMesh::TRIANGLE_STRIP &&
		m_ePrimType != vtMesh::POLYGON)
		return;

	m_Norm->resize(m_Vert->size());
	Vec3Array::iterator itr;
	for (itr = m_Norm->begin(); itr != m_Norm->end(); itr++)
		itr->set(0, 0, 0);

	switch (m_ePrimType)
	{
	case vtMesh::POINTS:		 break;
	case vtMesh::LINES:			 break;
	case vtMesh::LINE_STRIP:	 break;
	case vtMesh::TRIANGLES:		 _AddTriangleNormals(); break;
	case vtMesh::TRIANGLE_STRIP: _AddStripNormals(); break;
	case vtMesh::TRIANGLE_FAN:	 break;
	case vtMesh::QUADS:			 _AddQuadNormals(); break;
	case vtMesh::POLYGON:		 _AddPolyNormals(); break;
	}

	for (itr = m_Norm->begin(); itr != m_Norm->end(); itr++)
		itr->normalize();
}

void vtMesh::_AddStripNormals()
{
	DrawArrayLengths *pDrawArrayLengths = dynamic_cast<DrawArrayLengths*>(m_pPrimSet.get());
	int prims = GetNumPrims();
	int i, j, len, idx;
	unsigned short v0 = 0, v1 = 0, v2 = 0;
	osg::Vec3 p0, p1, p2, d0, d1, norm;

	idx = 0;
	for (i = 0; i < prims; i++)
	{
		len = pDrawArrayLengths->at(i);
		for (j = 0; j < len; j++)
		{
			v0 = v1; p0 = p1;
			v1 = v2; p1 = p2;
			v2 = m_Index->at(idx);
			p2 = m_Vert->at(v2);
			if (j >= 2)
			{
				d0 = (p1 - p0);
				d1 = (p2 - p0);
				d0.normalize();
				d1.normalize();

				norm = d0^d1;

				m_Norm->at(v0) += norm;
				m_Norm->at(v1) += norm;
				m_Norm->at(v2) += norm;
			}
			idx++;
		}
	}
}

void vtMesh::_AddPolyNormals()
{
	DrawArrayLengths *pDrawArrayLengths = dynamic_cast<DrawArrayLengths*>(m_pPrimSet.get());
	if (!pDrawArrayLengths)
		return;

	int prims = GetNumPrims();
	int i, j, len, idx;
	unsigned short v0 = 0, v1 = 0, v2 = 0;
	osg::Vec3 p0, p1, p2, d0, d1, norm;

	idx = 0;
	for (i = 0; i < prims; i++)
	{
		len = pDrawArrayLengths->at(i);
		// ensure this poly has enough verts to define a surface
		if (len >= 3)
		{
			v0 = m_Index->at(idx);
			v1 = m_Index->at(idx+1);
			v2 = m_Index->at(idx+2);
			p0 = m_Vert->at(v0);
			p1 = m_Vert->at(v1);
			p2 = m_Vert->at(v2);

			d0 = (p1 - p0);
			d1 = (p2 - p0);
			d0.normalize();
			d1.normalize();

			norm = d0^d1;

			for (j = 0; j < len; j++)
			{
				int v = m_Index->at(idx + j);
				m_Norm->at(v) += norm;
			}
		}
		idx += len;
	}
}

void vtMesh::_AddTriangleNormals()
{
	int tris = GetNumPrims();
	unsigned short v0, v1, v2;
	osg::Vec3 p0, p1, p2, d0, d1, norm;

	for (int i = 0; i < tris; i++)
	{
		v0 = m_Index->at(i*3);
		v1 = m_Index->at(i*3+1);
		v2 = m_Index->at(i*3+2);
		p0 = m_Vert->at(v0);
		p1 = m_Vert->at(v1);
		p2 = m_Vert->at(v2);

		d0 = (p1 - p0);
		d1 = (p2 - p0);
		d0.normalize();
		d1.normalize();

		norm = d0^d1;

		m_Norm->at(v0) += norm;
		m_Norm->at(v1) += norm;
		m_Norm->at(v2) += norm;
	}
}

void vtMesh::_AddQuadNormals()
{
	int quads = GetNumPrims();
	unsigned short v0, v1, v2, v3;
	osg::Vec3 p0, p1, p2, d0, d1, norm;

	for (int i = 0; i < quads; i++)
	{
		v0 = m_Index->at(i*4);
		v1 = m_Index->at(i*4+1);
		v2 = m_Index->at(i*4+2);
		v3 = m_Index->at(i*4+3);
		p0 = m_Vert->at(v0);
		p1 = m_Vert->at(v1);
		p2 = m_Vert->at(v2);

		d0 = (p1 - p0);
		d1 = (p2 - p0);
		d0.normalize();
		d1.normalize();

		norm = d0^d1;

		m_Norm->at(v0) += norm;
		m_Norm->at(v1) += norm;
		m_Norm->at(v2) += norm;
		m_Norm->at(v3) += norm;
	}
}


/////////////////////////////////////////////////////////////////////////////
// Text

vtFont::vtFont()
{
}

vtFont::~vtFont()
{
	// no need to free m_pOsgFont, it is a ref_ptr
}

bool vtFont::LoadFont(const char *filename)
{
	// OSG 0.9.3
//	m_pOsgFont = new osgText::Font(filename, 24, 3);

	// OSG 0.9.4
	m_pOsgFont = osgText::readFontFile(filename);

	if (m_pOsgFont == NULL)
		return false;

	return true;
}

////

/**
 * Construct a TextMesh object.
 *
 * \param font The font that will be used to draw the text.
 * \param fSize Size (height) in world units of the text rectangle.
 * \param bCenter If true, the origin of the text rectangle is at
 *			it's bottom center.  Otherwise, bottom left.
 */
vtTextMesh::vtTextMesh(vtFont *font, float fSize, bool bCenter)
{
	ref();		// artficially set refcount to 1

	// OSG 0.9.3
//	m_pOsgText = new osgText::Text(font->m_pOsgFont.get());

	// OSG 0.9.4
	m_pOsgText = new osgText::Text;
	m_pOsgText->setFont(font->m_pOsgFont.get());

	// set backpointer so we can find ourselves later
	m_pOsgText->setUserData(this);

	// Set the Font reference width and height resolution in texels.
	m_pOsgText->setFontResolution(32,32);

	// Set the rendered character size in object coordinates.
	m_pOsgText->setCharacterSize(fSize);

	if (bCenter)
		m_pOsgText->setAlignment(osgText::Text::CENTER_BOTTOM);

	// We'd like to turn off lighting for the text, but we can't, because
	//  the OSG Text object fiddles with its own StateSet.  Instead, we do
	//  it in vtGeom::AddTextMesh().
}

vtTextMesh::~vtTextMesh()
{
}

void vtTextMesh::Release()
{
	unref();
	if (_refCount == 1)	// no more references except from m_pGeometry
		// explicit dereference. if Release is not called, this dereference should
		//  also occur implicitly in the destructor
		m_pOsgText = NULL;
}

// Override with ability to get OSG bounding box
void vtTextMesh::GetBoundBox(FBox3 &box) const
{
	const BoundingBox &osg_box = m_pOsgText->getBound();
	s2v(osg_box, box);
}


void vtTextMesh::SetText(const char *text)
{
	m_pOsgText->setText(text);
}

void vtTextMesh::SetText(const wchar_t *text)
{
	m_pOsgText->setText(text);
}

#if SUPPORT_WSTRING
void vtTextMesh::SetText(const std::wstring &text)
{
	m_pOsgText->setText(text.c_str());
}
#endif

void vtTextMesh::SetPosition(const FPoint3 &pos)
{
	Vec3 s;
	v2s(pos, s);
	m_pOsgText->setPosition(s);
}

void vtTextMesh::SetRotation(const FQuat &rot)
{
	Quat q(rot.x, rot.y, rot.z, rot.w);
	m_pOsgText->setRotation(q);
}

void vtTextMesh::SetAlignment(int align)
{
	osgText::Text::AxisAlignment osga;

	if (align == 0)
		osga = osgText::Text::XY_PLANE;
	else if (align == 1)
		osga = osgText::Text::XZ_PLANE;
	else if (align == 2)
		osga = osgText::Text::YZ_PLANE;
	else return;

	m_pOsgText->setAxisAlignment(osga);
}

void vtTextMesh::SetColor(const RGBAf &rgba)
{
	osg::Vec4 color = v2s(rgba);
	m_pOsgText->setColor(color);
}
