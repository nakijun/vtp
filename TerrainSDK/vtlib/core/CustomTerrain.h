//
// CustomTerrain class : Dynamically rendering terrain
//
// Copyright (c) 2001-2007 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifndef CUSTOMTERRAINH
#define CUSTOMTERRAINH

/** \addtogroup dynterr */
/*@{*/

#include "DynTerrain.h"

#ifdef USE_OSGEARTH
namespace VTP {
#endif

/**
 * This class provides an example of how to add a CLOD implementation to
 * the vtlib library.  It contains the bare skeleton of a terrain rendering
 * algorithm, taking an elevation grid as input and rendering all the
 * geometry via OpenGL each frame.
 *
 * To add, or wrap, your own terrain rendering algorithm, simply make a
 * copy of CustomTerrain and rename it, then fill in the methods with your
 * own functionality.
 */
class CustomTerrain : public vtDynTerrainGeom
{
public:
	CustomTerrain();

	// initialization
	DTErr Init(const vtElevationGrid *pGrid, float fZScale);

	// overrides
	void DoRender();
	void DoCulling(const vtCamera *pCam);
	float GetElevation(int iX, int iZ, bool bTrue = false) const;
	void GetWorldLocation(int iX, int iZ, FPoint3 &p, bool bTrue = false) const;
	float GetVerticalExag() const { return m_fZScale; }

protected:
	virtual ~CustomTerrain();

	// rendering
	void RenderSurface();
	void RenderPass();
	void LoadSingleMaterial();

private:
	float *m_pData;			// the elevation height array
	float m_fZScale;
};

#ifdef USE_OSGEARTH
}
#endif

/*@}*/	// Group dynterr

#endif
