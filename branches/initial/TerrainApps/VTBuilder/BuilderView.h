//
// BuilderView.h
//
// Copyright (c) 2001 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifndef BUILDERVIEWH
#define BUILDERVIEWH

#include "ScaledView.h"
#include "vtdata/Projections.h"
#include "vtdata/Building.h"

class vtLayer;
class vtRoadLayer;
class vtElevLayer;
class vtStructureLayer;
class RoadEdit;

//
// Left-button modes
//
enum LBMode {
	LB_None,	// none
	LB_Road,	// select/edit roads
	LB_Node,	// select/edit nodes
	LB_Move,	// move selected nodes
	LB_Pan,		// pan the view
	LB_Dist,	// measure distance
	LB_Mag,		// zoom into rectangle
	LB_Path,	// pick points on a path
	LB_Dir,		// show/change road direction
	LB_RoadEdit,	// edit the points of a road centerline
	LB_RoadExtend,  //extend a road selection,
	LB_TSelect,		// select elevation layer
	LB_Box,			// set area box
	LB_FSelect,		// select feature
	LB_BldEdit,		// edit building
	LB_AddLinear,	// structures: add linear features
	LB_AddPoints	// add raw points
};

// A useful class to contain an array of bools
class BoolArray : public Array<bool> {};


class BuilderView : public vtScaledView
{
public:
	BuilderView(wxWindow* parent, wxWindowID id = -1, const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, const wxString& name = "");
	virtual ~BuilderView();

	virtual void OnDraw(wxDC& dc); // overridden to draw this view

	// Cursor
	void SetCorrectCursor();

	// Mouse
	void SetMode(LBMode m);
	LBMode GetMode() { return m_mode; }
	void GetMouseLocation(double &x, double &y);

	// UTM zone boundary display
	void DrawUTMBounds(wxDC *pDC);

	// World Map
	void SetWMProj(vtProjection &p);

	// Key handler
	void OnChar(wxKeyEvent& event);

	// More public methods
	void AreaStretch();
	void DeselectAll();
	void DeleteSelected(vtRoadLayer *pRL);
	void SetActiveLayer(vtLayer *lp);
	void MatchZoomToElev(vtElevLayer *pEL);

	bool	m_bCrossSelect;
	bool	m_bShowMap;
	bool	m_bShowUTMBounds;

protected:
	// Edit
	void OnLeftDownEditShape(wxMouseEvent& event);
	void OnLeftDownAddPoint(wxMouseEvent& event);
	void OnLeftDownAddLinear(wxMouseEvent& event);
	void UpdateMove();
	void UpdateResizeScale();
	void UpdateRotate();
	void OnDragDistance();
	void OnDragRoadEdit();

	// Elevation
	void CheckForTerrainSelect(DPoint2 loc);
	void HighlightTerrain(wxDC* pDC, vtElevLayer *t);

	// Pan handlers
	void BeginPan();
	void EndPan();
	void DoPan(wxPoint point);

	// Box handlers
	void BeginBox();
	void EndBox(wxMouseEvent& event);
	void DoBox(wxPoint point);
	void BeginArea();
	void DoArea(wxPoint point);

	// Line handlers
	void BeginLine();

	// Mouse handlers
	void OnLeftDown(wxMouseEvent& event);	
	void OnLeftUp(wxMouseEvent& event);
	void OnLeftDoubleClick(wxMouseEvent& event);
	void OnMiddleDown(wxMouseEvent& event);
	void OnMiddleUp(wxMouseEvent& event);
	void OnRightDown(wxMouseEvent& event);
	void OnRightUp(wxMouseEvent& event);

	void OnLButtonClick();
	void OnLButtonDragRelease(wxMouseEvent& event);
	void OnLButtonClickElement(vtRoadLayer *pRL);
	void OnLButtonClickDirection(vtRoadLayer *pRL);
	void OnLButtonClickRoadEdit(vtRoadLayer *pRL);
	void OnLButtonClickFeature(vtLayer *pL);
	void OnDblClickElement(vtRoadLayer *pRL, DPoint2 &point);
	void OnDblClickElement(vtStructureLayer *pSL, DPoint2 &point);
	void OnRightUp(vtRoadLayer *pRL);
	void OnRightUp(vtStructureLayer *pBL);
	void OnLeftDownRoadEdit();

	void OnMouseMove(wxMouseEvent& event);
	void OnMouseMoveLButton(wxPoint &point);

	void InvertRect(wxDC *pDC, wxRect &r, bool bDashed = false);
	void InvertRect(wxDC *pDC, wxPoint &one, wxPoint &two, bool bDashed = false);
	void DrawArea(wxDC *pDC);
	float BoundaryPixels();

	// in canvas coords
	wxPoint	m_DownPoint;
	wxPoint m_CurPoint;		// current position of mouse
	wxPoint m_LastPoint;	// last position of mouse

	// in widow coords
	wxPoint m_DownClient;

	// in world coordinates
	DPoint2 m_DownLocation;
	DPoint2 m_CurLocation;

	// Left Mouse Button Mode
	LBMode	m_mode;

	// Used while mouse button is down
	bool	m_bMouseMoved;
	bool	m_bPanning;		// currently panning
	bool	m_bBoxing;		// currently drawing a rubber box
	int		m_iDragSide;	// which side of the area box being dragged

	bool	m_bLMouseButton;
	bool	m_bMMouseButton;
	bool	m_bRMouseButton;

	// Used while editing buildings
	vtBuilding	*m_pCurBuilding, m_EditBuilding;
	int		m_iCurCorner;
	bool	m_bDragCenter;
	bool	m_bRotate;
	bool	m_bControl;
	bool	m_bShift;
	bool	m_bRubber;

	// Used while editing roads
	RoadEdit *m_pEditingRoad;
	void RefreshRoad(RoadEdit *pRoad);
	int		m_iEditingPoint;

	wxCursor	cursorPan;

	// World Map
	DLine2	*WMPoly;	 // Original data from SHP file
	DLine2	*WMPolyDraw; // This is WM that is drawn
	BoolArray *m_NoLines;
	int		m_iEntities;
	bool	m_bHidden;

	void ImportWorldMap();
	void DrawWorldMap(wxDC* pDC, vtScaledView *pView);
	void HideWorldMapEdges();

	DECLARE_EVENT_TABLE()
};

#endif