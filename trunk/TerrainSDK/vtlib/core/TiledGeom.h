//
// vtTiledGeom : Renders tiled heightfields using Roettger's libMini library
//
// Copyright (c) 2005-2007 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifndef TILEDGEOMH
#define TILEDGEOMH

#include "vtdata/HeightField.h"
#include "vtdata/MiniDatabuf.h"
#include <map>

#define TILEDGEOM_RESOLUTION_MIN 8000.0f
#define TILEDGEOM_RESOLUTION_MAX 8000000.0f

/** \addtogroup dynterr */
/*@{*/

#ifndef DOXYGEN_SHOULD_SKIP_THIS
class TiledDatasetDescription
{
public:
	TiledDatasetDescription();
	bool Read(const char *ini_fname);
	bool GetCorners(DLine2 &line, bool bGeo) const;

	int cols, rows;
	int lod0size;
	DRECT earthextents;
	float minheight, maxheight;	// for elevation tilesets only
	vtProjection proj;
	LODMap lodmap;
};
#endif

// Simple cache of tiles loaded from disk
typedef unsigned char *ucharptr;
class databuf;
struct CacheEntry { databuf *buf; int framestamp; };
typedef std::map<std::string, CacheEntry> TileCache;

/**
 * This class represents a tiled textured terrain heightfield, which is drawn
 * using the tiled paging capabilities of Roettger's libMini.  It is rendered
 * directly using OpenGL, instead of going through whichever scene graph vtlib is
 * built on.
 */
class vtTiledGeom : public vtDynGeom, public vtHeightField3d
{
public:
	vtTiledGeom();
	~vtTiledGeom();

	bool ReadTileList(const char *dataset_fname_elev,
		const char *dataset_fname_image, bool bThreading, bool bGradual);
	void SetVerticalExag(float fExag);
	float GetVerticalExag() const { return m_fDrawScale; }
	void SetVertexTarget(int iVertices);
	int GetVertexTarget() { return m_iVertexTarget; }
	void SetTileCacheSize(int iBytes) { m_iMaxCacheSize = iBytes; }
	int GetTileCacheSize() { return m_iMaxCacheSize; }
	int GetTileCacheUsed() { return m_iCacheSize; }
	FPoint2 GetWorldSpacingAtPoint(const DPoint2 &p);

	// overrides for vtDynGeom
	void DoRender();
	void DoCalcBoundBox(FBox3 &box);
	void DoCull(const vtCamera *pCam);

	// overrides for vtHeightField
	bool FindAltitudeOnEarth(const DPoint2 &p, float &fAltitude, bool bTrue = false) const;

	// overrides for vtHeightField3d
	bool FindAltitudeAtPoint(const FPoint3 &p3, float &fAltitude,
		bool bTrue = false, int iCultureFlags = 0,
		FPoint3 *vNormal = NULL) const;
	bool CastRayToSurface(const FPoint3 &point, const FPoint3 &dir,
		FPoint3 &result) const;

	// Tile cache methods
	databuf FetchAndCacheTile(const char *fname);
	void EmptyCache();

	// CRS of this tileset
	vtProjection m_proj;

	// detail level and vertex target
	float m_fResolution;
	float m_fHResolution;
	float m_fLResolution;
	int m_iVertexTarget;
	int m_iVertexCount;

	// Tile cache in host RAM, to reduce loading from disk
	TileCache m_Cache;
	int	m_iCacheSize;		// In bytes
	int	m_iMaxCacheSize;	// In bytes
	int m_iFrame;
	int m_iTileLoads;
	int m_iCacheHits;

	// Size of base texture LOD
	int image_lod0size;

	// Values used to initialize miniload
	int cols, rows;
	float coldim, rowdim;
	FPoint3 center;
	ucharptr *hfields, *textures;

	class miniload *GetMiniLoad() { return m_pMiniLoad; }
	class minitile *GetMiniTile() { return m_pMiniTile; }
	class datacloud *GetDataCloud() { return m_pDataCloud; }

	// information about all the tiles LODs which exist
	bool CheckMapFile(const char *mapfile, bool bIsTexture);
	vtString m_folder_elev;
	vtString m_folder_image;
	TiledDatasetDescription m_elev_info, m_image_info;

protected:
	// a vtlib material
	vtMaterial *m_pPlainMaterial;

	// Values used to render each frame
	IPoint2 m_window_size;
	FPoint3 m_eyepos_ogl;
	float m_fFOVY;
	float m_fAspect;
	float m_fNear, m_fFar;
	FPoint3 eye_up, eye_forward;
	bool m_bNeedResolutionAdjust;

	// vertical scale (exaggeration)
	float m_fMaximumScale;
	float m_fHeightScale;
	float m_fDrawScale;

	// the libMini objects
	class miniload *m_pMiniLoad;
	class minitile *m_pMiniTile;
	class minicache *m_pMiniCache;	// This is cache of OpenGL primitives to be rendered
	class datacloud *m_pDataCloud;

	void SetupMiniLoad(bool bThreading, bool bGradual);
};

/*@}*/	// Group dynterr

#endif // TILEDGEOMH
