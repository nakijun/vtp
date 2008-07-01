//
// SceneOSG.h
//
// Copyright (c) 2001-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifndef VTOSG_SCENEH
#define VTOSG_SCENEH

#include <osgUtil/SceneView>
#include <osg/Timer>

#if OLD_OSG_SHADOWS
class CStructureShadowsOSG;
#endif

/** \addtogroup sg */
/*@{*/

/**
 * A Scene is the all-encompassing container for all 3D objects
 * that are to be managed and drawn by the scene graph / graphics
 * pipeline functionality of vtlib.
 *
 * A Scene currently encapsulates:
	- A scene graph
	- A set of engines (vtEngine)
	- A window
	- A current camera (vtCamera)
 */
class vtScene : public vtSceneBase
{
public:
	vtScene();
	~vtScene();

	/// Set the root node, the top node of the scene graph.
	void SetRoot(vtGroup *pRoot);

	/// Set global wireframe, which will force all objects to be drawn wireframe.
	void SetGlobalWireframe(bool bWire);
	/// Get the global wireframe state.
	bool GetGlobalWireframe();

	/// Call this method once before calling any other vtlib methods.
	bool Init(bool bStereo = false, int iStereoMode = 0);

	/// Call this method after all other vtlib methods, to free memory.
	void Shutdown();

	void TimerRunning(bool bRun);
	void UpdateBegin();
	void UpdateEngines();
	void UpdateWindow(vtWindow *window);
	void PostDrawEngines();

	// backward compatibility
	void DoUpdate();

	/// Return the instantaneous framerate in frames per seconds estimated.
	float GetFrameRate()
	{
		return 1.0 / m_fLastFrameTime;
	}
	void DrawFrameRateChart();

	/// Time in seconds since the scene began.
	float GetTime()
	{
		return _timer.delta_s(_initialTick,_frameTick);
	}
	/// Time in seconds between the start of the previous frame and the current frame.
	float GetFrameTime()
	{
		return m_fLastFrameTime;
	}

	// View methods
	bool CameraRay(const IPoint2 &win, FPoint3 &pos, FPoint3 &dir, vtWindow *pWindow = NULL);
	void WorldToScreen(const FPoint3 &point, IPoint2 &result);
	FPlane *GetCullPlanes() { return m_cullPlanes; }

#if OLD_OSG_SHADOWS
	// Experimental:
	// Object-terrain shadow casting, only for OSG
	void SetShadowedNode(vtTransform *pLight, vtNode *pShadowerNode,
		vtNode *pShadowed, int iRez, float fDarkness, int iTextureUnit,
		const FSphere &ShadowSphere);
	void UnsetShadowedNode(vtTransform *pTransform);
	void UpdateShadowLightDirection(vtTransform *pLight);
	void SetShadowDarkness(float fDarkness);
	void SetShadowSphere(const FSphere &ShadowSphere, bool bForceRedraw);
	void ShadowVisibleNode(vtNode *node, bool bVis);
	bool IsShadowVisibleNode(vtNode *node);
	void ComputeShadows();
#endif

	void SetHUD(vtHUD *hud) { m_pHUD = hud; }
	vtHUD *GetHUD() { return m_pHUD; }
	void SetWindowSize(int w, int h, vtWindow *pWindow = NULL);

	// For backward compatibility
	void SetBgColor(const RGBf &color) {
		if (GetWindow(0))
			GetWindow(0)->SetBgColor(color);
	}

	bool IsStereo() const;;
	void SetStereoSeparation(float fSep);
	float GetStereoSeparation() const;
	void SetStereoFusionDistance(float fDist);
	float GetStereoFusionDistance();

	void ComputeViewMatrix(FMatrix4 &mat);

	// OSG access
	osgUtil::SceneView *getSceneView() { return m_pOsgSceneView.get(); }

protected:
	// OSG-specific implementation
	osg::ref_ptr<osgUtil::SceneView>	m_pOsgSceneView;

	// for culling
	void CalcCullPlanes();
	FPlane		m_cullPlanes[6];

	osg::ref_ptr<osg::Group>	m_pOsgSceneRoot;

	osg::Timer   _timer;
	osg::Timer_t _initialTick;
	osg::Timer_t _lastFrameTick;
	osg::Timer_t _lastRunningTick;
	osg::Timer_t _frameTick;
	double	m_fAccumulatedFrameTime, m_fLastFrameTime;

	bool	m_bWinInfo;
	bool	m_bInitialized;
	bool	m_bWireframe;
#if OLD_OSG_SHADOWS
	osg::ref_ptr<CStructureShadowsOSG> m_pStructureShadowsOSG;
#endif
	vtHUD	*m_pHUD;
};

// global
vtScene *vtGetScene();
float vtGetTime();
float vtGetFrameTime();
int vtGetMaxTextureSize();

/*@}*/	// Group sg

#endif	// VTOSG_SCENEH

