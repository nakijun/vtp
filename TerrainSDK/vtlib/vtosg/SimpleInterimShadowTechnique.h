//
// SimpleInterimShadowTechnique.h
//
// It's simple because it is.
// It's interim because it should be replaced with something better.
// It's a shadow technique.
//
// Copyright (c) 2001-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifndef VTOSG_SIMPLEINTERIMSHADOWTECHNIQUEH
#define VTOSG_SIMPLEINTERIMSHADOWTECHNIQUEH

#include <osg/Camera>
#include <osg/Material>
#include <osgShadow/ShadowTechnique>

class vtHeightField3d;

/** CSimpleInterimShadowTechnique provides an implementation of a depth map shadow tuned to the needs of VTP.*/
class CSimpleInterimShadowTechnique : public osgShadow::ShadowTechnique
{
public:
	CSimpleInterimShadowTechnique();
	CSimpleInterimShadowTechnique(const CSimpleInterimShadowTechnique& es, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
	META_Object(osgShadow, CSimpleInterimShadowTechnique);
	void SetShadowTextureUnit(const unsigned int Unit);
	void SetShadowTextureResolution(const unsigned int ShadowTextureResolution) { m_ShadowTextureResolution = ShadowTextureResolution; }
	unsigned int GetShadowTextureResolution() { return m_ShadowTextureResolution; }
	void SetRecalculateEveryFrame(const bool RecalculateEveryFrame) { m_RecalculateEveryFrame = RecalculateEveryFrame; }
	bool GetRecalculateEveryFrame() { return m_RecalculateEveryFrame; }
	void SetShadowDarkness(const float Darkness);
	float GetShadowDarkness() { return m_ShadowDarkness; }
	void SetShadowSphereRadius(const float ShadowSphereRadius) { m_ShadowSphereRadius = ShadowSphereRadius; }
	float GetShadowSphereRadius() { return m_ShadowSphereRadius; }
	void SetHeightField3d(vtHeightField3d *pHeightField3d) { m_pHeightField3d = pHeightField3d; }
	void AddMainSceneTextureUnit(const unsigned int Unit, const unsigned int Mode);
	void RemoveMainSceneTextureUnit(const unsigned int Unit);
	void ForceShadowUpdate();

	virtual void init();
	virtual void update(osg::NodeVisitor& nv);
	virtual void cull(osgUtil::CullVisitor& cv);
	virtual void cleanSceneGraph();

	osg::ref_ptr<osg::Camera> makeDebugHUD();

protected :

	virtual ~CSimpleInterimShadowTechnique() {}

	std::string GenerateFragmentShaderSource();

	osg::ref_ptr<osg::Camera>		m_pCamera;
	osg::ref_ptr<osg::TexGen>		m_pTexgen;
	osg::ref_ptr<osg::Texture2D>	m_pTexture;
	osg::ref_ptr<osg::StateSet>		m_pStateset;
	osg::ref_ptr<osg::Material>		m_pMaterial;
	unsigned int					m_ShadowTextureUnit;// Must be set before init is called
	unsigned int					m_ShadowTextureResolution; // Must be set before init is called
	bool							m_RecalculateEveryFrame; // Can be set any time
	float							m_PolygonOffsetFactor; // Must be set before init is called
	float							m_PolygonOffsetUnits; // Must be set before init is called
	float							m_ShadowDarkness; // Must be set before init is called
	float							m_ShadowSphereRadius; // Must be set before init is called
	std::map<unsigned int, unsigned int>	m_MainSceneTextureUnits;
	vtHeightField3d					*m_pHeightField3d;
	osg::Vec3						m_OldBoundingSphereCentre;
	osg::Vec3						m_OldSunPos;
};
#endif
