//
// Contours.h
//
// Copyright (c) 2004-2006 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifndef CONTOURSH
#define CONTOURSH

#if SUPPORT_QUIKGRID

#include "Terrain.h"
class SurfaceGrid;

/** \defgroup utility Utility classes
 */
/*@{*/

/**
 * This class provides the ability to easily construct contour lines
 * on a terrain.  It does so by calling the QuikGrid library to generate
 * contour vectors, then converts those vectors into 3D line geometry
 * draped on the terrain.
 *
 * \par Here is an example of how to use it:
	\code
	vtContourConverter cc;
	cc.Setup(pTerrain, RGBf(1,1,0), 10);
	cc.GenerateContours(100);
	cc.Finish();
	\endcode
 *
 * \par Or, you can generate specific contour lines:
	\code
	vtContourConverter cc;
	cc.Setup(pTerrain, RGBf(1,1,0), 10);
	cc.GenerateContour(75);
	cc.GenerateContour(125);
	cc.GenerateContour(250);
	cc.Finish();
	\endcode
 *
 * \par If you keep a pointer to the geometry, you can toggle or delete it later:
	\code
	vtContourConverter cc;
	vtGeom *geom = cc.Setup(pTerrain, RGBf(1,1,0), 10);
	[...]
	geom->SetEnabled(bool);	// set visibility
	[...]
	pTerrain->GetScaledFeatures()->RemoveChild(geom);
	geom->Release();		// delete
	\endcode
 */
class vtContourConverter
{
public:
	vtContourConverter();
	~vtContourConverter();

	vtGeom *Setup(vtTerrain *pTerr, const RGBf &color, float fHeight);
	void GenerateContour(float fAlt);
	void GenerateContours(float fAInterval);
	void Finish();
	void Coord(float x, float y, bool bStart);

protected:
	void Flush();

	SurfaceGrid *m_pGrid;

	vtTerrain *m_pTerrain;
	vtHeightFieldGrid3d *m_pHF;
	DRECT m_ext;
	DPoint2 m_spacing;
	float m_fHeight;

	DLine2	m_line;
	vtGeom *m_pGeom;
	vtMeshFactory *m_pMF;
};

/*@}*/  // utility

#endif // SUPPORT_QUIKGRID
#endif // CONTOURSH
