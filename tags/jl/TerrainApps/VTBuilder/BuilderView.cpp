//
// BuilderView.cpp
//
// Copyright (c) 2001 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#include "wx/wxprec.h"
#include "vtdata/shapelib/shapefil.h"

#include "BuilderView.h"
#include "Frame.h"
#include "MenuEnum.h"
#include "Helper.h"
#include "VTBuilder_wdr.h"
// Layers
#include "ElevLayer.h"
#include "RawLayer.h"
#include "RoadLayer.h"
#include "StructLayer.h"

#include "cpl_error.h"

////////////////////////////////////////////////////////////////

#define BOUNDADJUST 5

BEGIN_EVENT_TABLE(BuilderView, vtScaledView)
	EVT_LEFT_DOWN(BuilderView::OnLeftDown)	
	EVT_LEFT_UP(BuilderView::OnLeftUp)
	EVT_LEFT_DCLICK(BuilderView::OnLeftDoubleClick)
	EVT_MIDDLE_DOWN(BuilderView::OnMiddleDown)
	EVT_MIDDLE_UP(BuilderView::OnMiddleUp)
	EVT_RIGHT_DOWN(BuilderView::OnRightDown)
	EVT_RIGHT_UP(BuilderView::OnRightUp)
	EVT_MOTION(BuilderView::OnMouseMove)

	EVT_CHAR(BuilderView::OnChar)
END_EVENT_TABLE()

/////////////////////////////////////////////////////////////////

BuilderView::BuilderView(wxWindow* parent, wxWindowID id, const wxPoint& pos,
						 const wxSize& size, const wxString& name) :
	vtScaledView(parent, id, pos, size, name ),
	cursorPan("cursors/panhand.cur", wxBITMAP_TYPE_CUR)
{
	m_bPanning = false;
	m_bBoxing = false;
	m_mode = LB_None;
	m_iDragSide = 0;
	m_bCrossSelect = false;
	m_bMouseMoved = false;
	m_bLMouseButton = m_bMMouseButton = m_bRMouseButton = false;
	m_bShowMap = true;
	m_bShowUTMBounds = false;
	m_bRubber = false;

	// road point editing
	m_pEditingRoad = NULL;
	m_iEditingPoint = -1;

	// Import world map SHP file
	WMPoly = NULL;
	WMPolyDraw = NULL;
	ImportWorldMap();
}

BuilderView::~BuilderView()
{
	delete [] WMPoly;
	delete [] WMPolyDraw;
	delete [] m_NoLines;
}

////////////////////////////////////////////////////////////
// Operations

void BuilderView::OnDraw(wxDC& dc)  // overridden to draw this view
{
	MainFrame *pFrame = GetMainFrame();
	vtLayerPtr lp;
	int i, iLayers = pFrame->m_Layers.GetSize();

	// Draw the world map SHP file of country outline polys in latlon
	if (m_bShowMap)
		DrawWorldMap(&dc, this);

	// Draw the solid layers first
	for (i = 0; i < iLayers; i++)
	{
		lp = pFrame->m_Layers.GetAt(i);
		if (!lp->GetVisible())
			continue;
		if (lp->GetType() != LT_IMAGE && lp->GetType() != LT_ELEVATION)
			continue;
		if (lp->GetVisible())
			lp->DrawLayer(&dc, this);
	}
	// Then the poly/vector/point layers
	for (i = 0; i < iLayers; i++)
	{
		lp = pFrame->m_Layers.GetAt(i);
		if (!lp->GetVisible())
			continue;
		if (lp->GetType() == LT_IMAGE || lp->GetType() == LT_ELEVATION)
			continue;
		if (lp->GetVisible())
			lp->DrawLayer(&dc, this);
	}
	if (pFrame->GetActiveElevLayer())
		HighlightTerrain(&dc, pFrame->GetActiveElevLayer());

	if (m_bShowUTMBounds)
		DrawUTMBounds(&dc);

	DrawArea(&dc);
}

void BuilderView::GetMouseLocation(double &x, double &y)
{
	x = ox(m_CurPoint.x);
	y = oy(m_CurPoint.y);
}

void BuilderView::SetMode(LBMode m)
{
	if (m_mode == LB_Dir && m != LB_Dir)
	{
		vtRoadLayer::SetShowDirection(false);
		Refresh();
	}

	m_mode = m;
	SetCorrectCursor();

	switch (m_mode)
	{
		case LB_Dir:
			vtRoadLayer::SetShowDirection(true);
			Refresh();
			break;
		case LB_Node:
			if (!vtRoadLayer::GetDrawNodes()) {
				vtRoadLayer::SetDrawNodes(true);
				Refresh();
			}
			break;
	}
	if (m_mode != LB_RoadEdit)
	{
		if (m_pEditingRoad)
		{
			m_pEditingRoad->m_bDrawPoints = false;
			RefreshRoad(m_pEditingRoad);
		}
		m_pEditingRoad = NULL;
	}
}

float BuilderView::BoundaryPixels()
{
	return (float) odx(BOUNDADJUST);
}

void BuilderView::DrawUTMBounds(wxDC *pDC)
{
	wxPen orange(wxColor(255,128,0), 1, PS_SOLID);
	pDC->SetLogicalFunction(wxCOPY);
	pDC->SetPen(orange);

	vtProjection proj;
	GetMainFrame()->GetProjection(proj);

	int width, height;
	GetClientSize(&width, &height);

	DPoint2 ll, utm;
	wxPoint sp, array[4000];
	int zone, j;

	if (proj.IsGeographic())
	{
		for (zone = 0; zone < 60; zone++)
		{
			ll.x = -180.0f + zone * 6.0;

			ll.y = -70.0;
			screen(ll, array[0]);
			ll.y = +70.0;
			screen(ll, array[1]);

			pDC->DrawLines(2, array);
		}
	}
	else
	{
		int zone_start = 0;
		int zone_end = 60;
		int zone;
		DPoint2 proj_point;

		vtProjection geo;
		CreateSimilarGeographicProjection(proj, geo);

		OCT *trans = OGRCreateCoordinateTransformation(&proj, &geo);

		// try to speed up a bit by avoiding zones off the screen
		object(wxPoint(0, height/2), proj_point);
		trans->Transform(1, &proj_point.x, &proj_point.y);
		zone = GuessZoneFromLongitude(proj_point.x);
		if (zone-1 > zone_start) zone_start = zone-1;

		object(wxPoint(width, height/2), proj_point);
		trans->Transform(1, &proj_point.x, &proj_point.y);
		zone = GuessZoneFromLongitude(proj_point.x);
		if (zone+1 < zone_end) zone_end = zone+1;

		delete trans;

		// Now convert the longitude lines between the UTM zones to the current projection
		trans = OGRCreateCoordinateTransformation(&geo, &proj);

		for (zone = zone_start; zone < zone_end; zone++)
		{
			ll.x = -180.0f + zone * 6.0;
			j = 0;
			for (ll.y = -70.0; ll.y <= 70.0; ll.y += 0.1)
			{
				proj_point = ll;
				trans->Transform(1, &proj_point.x, &proj_point.y);
				screen(proj_point, sp);
				if (sp.y < -8000 || sp.y > 8000)
					continue;
				array[j++] = sp;
			}
			pDC->DrawLines(j, array);
		}
		delete trans;
	}
}

//////////////////////////////////////////////////////////
// Added capability to read & display world map, poly SHP file.

// Get data out of SHP into WMPolys
void BuilderView::ImportWorldMap()
{
	SHPHandle	hSHP;
	int			nShapeType;
	double		adfMinBound[4], adfMaxBound[4];
	int			i, j;

	// Open SHP file
	hSHP = SHPOpen( "WorldMap/gnv19.shp", "rb");
	if (hSHP == NULL) return;

	SHPGetInfo(hSHP, &m_iEntities, &nShapeType, adfMinBound, adfMaxBound);
	if (nShapeType != SHPT_POLYGON) return;

	// Copy SHP data into World Map Poly data
	WMPoly = new DLine2[m_iEntities];
	WMPolyDraw = new DLine2[m_iEntities];
	m_NoLines = new BoolArray[m_iEntities];
	m_bHidden = false;

	for (i = 0; i < m_iEntities; i++)
	{
		DLine2 &current = WMPoly[i];
		BoolArray &noline = m_NoLines[i];

		DPoint2 p;

		SHPObject	*psShape;
		psShape = SHPReadObject(hSHP, i);

		int points = psShape->nVertices;

		current.SetMaxSize(points);
		noline.SetMaxSize(points);

		for (j = 0; j < points; j++)
		{
			p.x = psShape->padfX[j];
			p.y = psShape->padfY[j];
			current.Append(p);
		}
		SHPDestroyObject(psShape);

		for (j = 0; j < points; j++)
			noline[j] = false;
	}

	// Close SHP file
	SHPClose(hSHP);

	// Initialize the drawn World Map WMPolyDraw to original (latlon)
	for (i = 0; i < m_iEntities; i++)
		WMPolyDraw[i] = WMPoly[i];
}

//
// post-process WMPoly to determine lines not to draw 
//
void BuilderView::HideWorldMapEdges()
{
	int i, points;

	for (i = 0; i < m_iEntities; i++)
	{
		DLine2 &current = WMPoly[i];
		BoolArray &noline = m_NoLines[i];
		points = current.GetSize();
		// must have at least 8 points to worry about internal edges
		if (points < 8 || points > 8000)
			continue;

		// detect and turn off internal edges
		int a, b;
		for (a = 0; a < points-2; a++)
		{
			for (b = a+1; b < points-1; b++)
			{
				if (current[a] == current[b])
				{
					// This is an internal edge
					noline[a+1] = true;
					noline[b] = true;
					break;
				}
			}
		}
	}
	m_bHidden = true;
}


void myErrorHandler(CPLErr err, int i, const char*str)
{
	int foo = err;
	wxLogDebug(str);
}

void BuilderView::SetWMProj(vtProjection &proj)
{
	int i, j;

	const char *proj_name = proj.GetProjectionNameShort();
	if (!strcmp(proj_name, "Geo"))
	{
		// the data is already in latlon so just use WMPoly
		for (i = 0; i < m_iEntities; i++)
			WMPolyDraw[i] = WMPoly[i];
		return;
	}

	// Otherwise, must convert from Geo to whatever project is desired
	vtProjection Source;
	CreateSimilarGeographicProjection(proj, Source);

#if DEBUG
	// Check projection text
	char *str1, *str2;
	Source.exportToWkt(&str1);
	proj.exportToWkt(&str2);
	wxLogDebug(str1);
	wxLogDebug(str2);

	// Check texts in PROJ4
	char *str3, *str4;
	Source.exportToProj4(&str3);
	wxLogDebug(str3);
	proj.exportToProj4(&str4);
	wxLogDebug(str4);
#endif

	CPLPushErrorHandler(myErrorHandler);
	// Create conversion object
	OCT *trans = OGRCreateCoordinateTransformation(&Source, &proj);
	CPLPopErrorHandler();

	if (!trans)
	{
		m_bShowMap = false;
		return;
	}

	DPoint2 point;
	for (i = 0; i < m_iEntities; i++)
	{
		for (j = 0; j < WMPoly[i].GetSize(); j++)
		{
			point = WMPoly[i][j];
			trans->Transform(1, &point.x, &point.y);
			WMPolyDraw[i][j] = point;
		}
	}
}


#define MAXPOINTS 8000
static wxPoint wmbuf[MAXPOINTS];
// All poly have less than 8000 points except ocean (the one we don't want
// to draw)

void BuilderView::DrawWorldMap(wxDC* pDC, vtScaledView *pView)
{
	// Set pen options
	wxPen WMPen(wxColor(0,0,0), 1, PS_SOLID);  //solid black pen
	pDC->SetLogicalFunction(wxCOPY);
	pDC->SetPen(WMPen);

	if (!m_bHidden)
	{
		int x, y;
		GetClientSize(&x, &y);
		pDC->DrawText("Preparing World Map...", x * 4 / 10, y / 2);
		HideWorldMapEdges();
		Refresh();
	}

	// Draw each poly in WMPolyDraw
	for (int i = 0; i < m_iEntities; i++)
	{
		int wmbuflen = 0;
		if (WMPolyDraw[i].GetSize() > MAXPOINTS)
			continue;

		for (int j = 0; j < WMPolyDraw[i].GetSize(); j++)
		{
			wmbuf[j].x = pView->sx(WMPolyDraw[i][j].x);
			wmbuf[j].y = pView->sy(WMPolyDraw[i][j].y);
			wmbuflen += 1;
			if (m_NoLines[i][j] == true)
			{
				if (wmbuflen > 2) pDC->DrawLines(wmbuflen - 1, wmbuf);
				wmbuflen = 0;
			}
		}
		if (wmbuflen > 1) pDC->DrawLines(wmbuflen, wmbuf);
		wmbuflen = 0;
	}
}


//////////////////////////////////////////////////////////
// Pan handlers

void BuilderView::BeginPan()
{
	m_bPanning = true;
	SetCursor(cursorPan);
}

void BuilderView::EndPan()
{
	m_bPanning = false;
	SetCorrectCursor();
}

void BuilderView::DoPan(wxPoint point)
{
	wxPoint diff;
	diff = point - m_DownClient;
	m_DownClient = point;

	// update picture to reflect the changes
	Scroll(m_xScrollPosition - diff.x,
		   m_yScrollPosition - diff.y);
}


//////////////////////////////////////////////////////////
// Box handlers

void BuilderView::InvertRect(wxDC *pDC, wxRect &r, bool bDashed)
{
	InvertRect(pDC, wxPoint(r.x, r.y),
		wxPoint(r.x + r.width, r.y + r.height), bDashed);
}

void BuilderView::InvertRect(wxDC *pDC, wxPoint &one, wxPoint &two,
							 bool bDashed)
{
	wxPen pen(*wxBLACK_PEN);
	if (bDashed)
		pen.SetStyle(wxSHORT_DASH);
	pDC->SetPen(pen);
	pDC->SetLogicalFunction(wxINVERT);

	wxPoint points[5];
	points[0].x = one.x;
	points[0].y = one.y;
	points[1].x = one.x;
	points[1].y = two.y;
	points[2].x = two.x;
	points[2].y = two.y;
	points[3].x = two.x;
	points[3].y = one.y;
	points[4].x = one.x;
	points[4].y = one.y;

	pDC->DrawLines(5, points);
}

void BuilderView::BeginBox()
{
	m_bBoxing = true;
}

void BuilderView::EndBox(wxMouseEvent& event)
{
	m_bBoxing = false;

	if (!m_bMouseMoved)
		return;

	wxClientDC dc(this);
	PrepareDC(dc);

	InvertRect(&dc, m_DownPoint, m_LastPoint);

	wxRect rect = PointsToRect(m_DownPoint, m_LastPoint);
	DRECT world_rect = CanvasToWorld(rect);
	switch (m_mode)
	{
	case LB_Mag:
		if (event.AltDown())
			ZoomOutToRect(world_rect);
		else
			ZoomToRect(world_rect, 0.0f);
		break;
	case LB_Box:
		DrawArea(&dc);
		GetMainFrame()->m_area = world_rect;
		DrawArea(&dc);
		break;
	case LB_Path:
		//do not allow click and drag selection.
		//ignore action
		break;
	case LB_Node:
	case LB_Road:
		{
			//select everything in the highlighted box.
			vtRoadLayer *pRL = GetMainFrame()->GetActiveRoadLayer();
			if (pRL->SelectArea(world_rect, (m_mode == LB_Node),
				m_bCrossSelect))
			{
				rect = WorldToWindow(world_rect);
				IncreaseRect(rect, BOUNDADJUST);
				if (m_bCrossSelect)
					Refresh();
				else
					Refresh(TRUE, &rect);
			} else {
				DeselectAll();
			}
		}
		break;
	case LB_Move:
		Refresh();
		break;
	case LB_FSelect:
		{
			vtStructureLayer *pSL = GetMainFrame()->GetActiveStructureLayer();
			int selected = 0;
			for (int i = 0; i < pSL->GetSize(); i++)
			{
				vtStructure *str = pSL->GetAt(i);
				vtBuilding *bld = str->GetBuilding();
				if (!bld)
					continue;
				bool bSelect = world_rect.ContainsPoint(bld->GetLocation());
				str->Select(bSelect);
				if (bSelect) selected++;
			}
			wxString msg;
			msg.Printf("Selected %d entit%s", selected, selected == 1 ? "y" : "ies");
			GetMainFrame()->SetStatusText(msg);
			Refresh(FALSE);
		}
	}
}

void BuilderView::DoBox(wxPoint point)
{
	wxClientDC dc(this);
	PrepareDC(dc);
	InvertRect(&dc, m_DownPoint, m_LastPoint);
	InvertRect(&dc, m_DownPoint, point);
}

void BuilderView::DrawArea(wxDC *pDC)
{
	DRECT area = GetMainFrame()->m_area;
	if (!area.IsEmpty())
	{
		int d = 3;
		wxRect r = WorldToCanvas(area);
		InvertRect(pDC, r, true);
		InvertRect(pDC, wxPoint(r.x-d, r.y-d), wxPoint(r.x+d, r.y+d));
		InvertRect(pDC, wxPoint(r.x+r.width-d, r.y-d), wxPoint(r.x+r.width+d, r.y+d));
		InvertRect(pDC, wxPoint(r.x-d, r.y+r.height-d), wxPoint(r.x+d, r.y+r.height+d));
		InvertRect(pDC, wxPoint(r.x+r.width-d, r.y+r.height-d), wxPoint(r.x+r.width+d, r.y+r.height+d));
	}
}

void BuilderView::BeginLine()
{
}

////////////////////////////////////////////////////////////
// Elevation

void BuilderView::CheckForTerrainSelect(DPoint2 loc)
{
	MainFrame *pFrame = GetMainFrame();

	// perhaps the user clicked on a terrain
	bool bFound = false;
	for (int l = 0; l < pFrame->m_Layers.GetSize(); l++)
	{
		vtLayerPtr lp = pFrame->m_Layers.GetAt(l);
		if (lp->GetType() != LT_ELEVATION) continue;
		vtElevLayer *t = (vtElevLayer *)lp;

		if (t->GetExtents().ContainsPoint(loc))
		{
			SetActiveLayer(t);
			bFound = true;
			break;
		}
	}

#if 0
	// deselect if no terrain selected (questionable logic)
	if (!bFound)
		SetActiveLayer(NULL);
#endif
}

//
// The view needs to be notified of the new active layer to update
// the selection marks drawn around the active layer.
//
void BuilderView::SetActiveLayer(vtLayerPtr lp)
{
	MainFrame *pFrame = GetMainFrame();
	vtElevLayer *last = pFrame->GetActiveElevLayer();
	if (lp != last)
	{
		wxClientDC DC(this), *pDC = &DC;
		PrepareDC(DC);

		if (last)
			HighlightTerrain(pDC, last);

		pFrame->SetActiveLayer(lp, true);

		if (pFrame->GetActiveElevLayer())
			HighlightTerrain(pDC, pFrame->GetActiveElevLayer());
	}
}

void BuilderView::HighlightTerrain(wxDC* pDC, vtElevLayer *t)
{
	wxPen bgPen(wxDOT, 3, RGB(0xff, 0xff, 0xff));
	pDC->SetPen(bgPen);
	pDC->SetLogicalFunction(wxINVERT);

	wxRect sr = WorldToCanvas(t->GetExtents());
	int s = sr.width / 3;
	int left = sr.x, right = sr.x+sr.width,
		top = sr.y, bottom = sr.y+sr.height;
	int d=3,e=6;

	//
	pDC->DrawLine(left - e, top - d,
				  left - e, top + s);

	pDC->DrawLine(left - d, top - e,
				  left + s, top - e);

	//
	pDC->DrawLine(right - s, top - e,
				  right + e, top - e);

	pDC->DrawLine(right + e, top - d,
				  right + e, top + s);

	//
	pDC->DrawLine(right + e, bottom - s,
				  right + e, bottom + d);

	pDC->DrawLine(right - s, bottom + e,
				  right + e, bottom + e);

	//
	pDC->DrawLine(left - e, bottom - s,
				  left - e, bottom + d);

	pDC->DrawLine(left + s, bottom + e,
				  left - e, bottom + e);
}

////////////////////////////////////////////////////////////
// Cursor

void BuilderView::SetCorrectCursor()
{
	switch (m_mode)
	{
	case LB_None:	// none
	case LB_Road:	// select/edit roads
	case LB_Node:	// select/edit nodes
	case LB_Move:	// move selected nodes
		SetCursor(wxCURSOR_ARROW); break;
	case LB_Pan:	// pan the view
		SetCursor(cursorPan); break;
	case LB_Dist:	// measure distance
		SetCursor(wxCURSOR_CROSS); break;
	case LB_Mag:	// zoom into rectangle
		SetCursor(wxCURSOR_MAGNIFIER); break;
	case LB_Path:	// pick points on a path
	case LB_Dir:		// show/change road direction
	case LB_RoadEdit:	// edit road points
	case LB_RoadExtend: //extend a road selection
	case LB_TSelect:
	case LB_Box:
	default:
		SetCursor(wxCURSOR_ARROW); break;
	}
}


/////////////////////////////////////////////////////////////

void BuilderView::BeginArea()	// in canvas coordinates
{
	DRECT area = GetMainFrame()->m_area;
	if (area.IsEmpty())
	{
		BeginBox();
		return;
	}

	// check to see if they've clicked near one of the sides of the area
	m_iDragSide = 0;
	int eps = 10;	// epsilon in pixels
	wxRect r = WorldToCanvas(area);
	int diff;
	diff = abs(m_CurPoint.x - r.x);
	if (diff < eps)
		m_iDragSide |= 1;
	diff = abs(m_CurPoint.x - (r.x+r.width));
	if (diff < eps)
		m_iDragSide |= 2;
	diff = abs(m_CurPoint.y - r.y);
	if (diff < eps)
		m_iDragSide |= 4;
	diff = abs(m_CurPoint.y - (r.y+r.height));
	if (diff < eps)
		m_iDragSide |= 8;

	// if they didn't click near the box, start a new one
	if (!m_iDragSide)
		BeginBox();
}

void BuilderView::DoArea(wxPoint point)	// in canvas coordinates
{
	wxClientDC dc(this);
	PrepareDC(dc);
	DrawArea(&dc);
	if (m_iDragSide & 1)
		GetMainFrame()->m_area.left = ox(point.x);
	if (m_iDragSide & 2)
		GetMainFrame()->m_area.right = ox(point.x);
	if (m_iDragSide & 4)
		GetMainFrame()->m_area.top = oy(point.y);
	if (m_iDragSide & 8)
		GetMainFrame()->m_area.bottom = oy(point.y);
	DrawArea(&dc);
}

void BuilderView::AreaStretch()
{
	wxClientDC dc(this);
	PrepareDC(dc);
	DrawArea(&dc);
	GetMainFrame()->StretchArea();
	DrawArea(&dc);
}

void BuilderView::DeselectAll()
{
	vtRoadLayer *pRL = GetMainFrame()->GetActiveRoadLayer();
	if (!pRL)
		return;

	DRECT* world_bounds;
	wxRect bound;
	int n;

	// caller is responsible for deleting the array returned from DeselectAll
	world_bounds = pRL->DeSelectAll(n);
	if (n > 100)
	{
		// too many deleted for quick refresh
		Refresh(TRUE);
	}
	else
	{
		n = n -1;
		while (n >=0) {
			bound = WorldToWindow(world_bounds[n]);
			IncreaseRect(bound, BOUNDADJUST);
			Refresh(TRUE, &bound);
			n--;
		}
	}
	delete world_bounds;
}

void BuilderView::DeleteSelected(vtRoadLayer *pRL)
{
	int n;

	// delete the items, which returns an array of extents,
	// one for each deleted item
	DRECT *world_bounds = pRL->DeleteSelected(n);

	if (pRL->RemoveUnusedNodes() != 0)
		pRL->ComputeExtents();

	if (n > 100)
	{
		// too many deleted for quick refresh
		Refresh(TRUE);
	}
	else if (n > 0  && world_bounds != NULL)
	{
		wxRect bound;
		for (n = n - 1; n >= 0; n--)
		{
			bound = WorldToWindow(world_bounds[n]);
			IncreaseRect(bound, BOUNDADJUST);
			Refresh(TRUE, &bound);
		}
		pRL->SetModified(true);
	}
	delete world_bounds;
}

void BuilderView::MatchZoomToElev(vtElevLayer *pEL)
{
	DPoint2 spacing = pEL->m_pGrid->GetSpacing();
	SetScale(1.0f / spacing.x);

	FPoint2 center;
	DRECT area = pEL->GetExtents();
	center.x = (float) (area.left + area.right) / 2;
	center.y = (float) (area.bottom + area.top) / 2;
	ZoomToPoint(center);

	Refresh();
}

/////////////////////////////////////////////////////////////
// Mouse handlers

void BuilderView::OnLeftDown(wxMouseEvent& event)
{
	m_bLMouseButton = true;
	m_bMouseMoved = false;	

	// save the point where the user clicked
	m_DownClient = event.GetPosition();
	GetCanvasPosition(event, m_DownPoint);

	m_CurPoint = m_DownPoint;
	m_LastPoint = m_DownPoint;

	// "points" are in window pixels, "locations" are in current projection
	object(m_DownPoint, m_DownLocation);

	CaptureMouse();			// capture mouse

	switch (m_mode)
	{
	case LB_TSelect:
		CheckForTerrainSelect(m_DownLocation);
		break;

	case LB_Pan:
		BeginPan();
		break;

	case LB_Mag:
	case LB_Node:
	case LB_Road:
	case LB_FSelect:
		BeginBox();
		break;

	case LB_Box:
		BeginArea();
		break;

	case LB_Dist:
		BeginLine();
		break;

	case LB_RoadEdit:
		OnLeftDownRoadEdit();
		break;

	case LB_BldEdit:
		OnLeftDownEditShape(event);
		break;

	case LB_AddPoints:
		OnLeftDownAddPoint(event);
		break;

	case LB_AddLinear:
		OnLeftDownAddLinear(event);
		break;
	}
}

void BuilderView::OnLeftDownRoadEdit()
{
	vtRoadLayer *pRL = GetMainFrame()->GetActiveRoadLayer();
	if (!pRL)
		return;
	if (!m_pEditingRoad)
		return;

	double closest = 1E8;
	int closest_i;
	for (int i = 0; i < m_pEditingRoad->GetSize(); i++)
	{
		DPoint2 diff = m_DownLocation - m_pEditingRoad->GetAt(i);
		double dist = diff.Length();
		if (dist < closest)
		{
			closest = dist;
			closest_i = i;
		}
	}
	int pixels = sdx(closest);
	if (pixels < 8)
	{
		// begin dragging point
		m_iEditingPoint = closest_i;
	}
	else
		m_iEditingPoint = -1;
}

void BuilderView::OnLeftDownEditShape(wxMouseEvent& event)
{
	vtStructureLayer *pSL = GetMainFrame()->GetActiveStructureLayer();
	if (!pSL)
		return;

	double error = sdx(6);  //calculate what 6 pixels is as world coord

	int building1, building2, corner;
	double dist1, dist2;

	bool found1 = pSL->FindClosestBuildingCenter(m_DownLocation, error, building1, dist1);
	bool found2 = pSL->FindClosestBuildingCorner(m_DownLocation, error, building2, corner, dist2);

	if (found1 && found2)
	{
		// which was closer?
		if (dist1 < dist2)
			found2 = false;
		else
			found1 = false;
	}
	if (found1)
	{
		// closest point is a building center
		m_pCurBuilding = pSL->GetAt(building1)->GetBuilding();
		m_bDragCenter = true;
	}
	if (found2)
	{
		// closest point is a building corner
		m_pCurBuilding = pSL->GetAt(building2)->GetBuilding();
		m_bDragCenter = false;
		m_iCurCorner = corner;

		m_bShift = event.ShiftDown();
		m_bControl = event.ControlDown();

		m_bRotate = m_bControl;
	}
	if (found1 || found2)
	{
		m_bRubber = true;

		// make a copy of the building, to edit and diplay while dragging
		m_EditBuilding = *m_pCurBuilding;
	}
}

void BuilderView::OnLeftDownAddPoint(wxMouseEvent &event)
{
	vtRawLayer *pRL = GetMainFrame()->GetActiveRawLayer();
	pRL->AddPoint(m_DownLocation);
	Refresh();
}

void BuilderView::OnLeftDownAddLinear(wxMouseEvent &event)
{
	vtStructureLayer *pSL = GetMainFrame()->GetActiveStructureLayer();
//	pRL->AddPoint(m_DownLocation); // TODO
	Refresh();
}

void BuilderView::OnLeftUp(wxMouseEvent& event)
{
	ReleaseMouse();

	if (m_mode == LB_BldEdit)
	{
		if (m_bRubber)
		{
			DRECT extent_old, extent_new;
			m_pCurBuilding->GetExtents(extent_old);
			m_EditBuilding.GetExtents(extent_new);
			wxRect screen_old = WorldToWindow(extent_old);
			wxRect screen_new = WorldToWindow(extent_new);
//			screen_old.InflateRect(1, 1);
//			screen_new.InflateRect(1, 1);

			Refresh(TRUE, &screen_old);
			Refresh(TRUE, &screen_new);

			// copy back from temp building to real building
			*m_pCurBuilding = m_EditBuilding;
			m_bRubber = false;
			GetMainFrame()->GetActiveLayer()->SetModified(true);
		}
	}
	else
	{
		if (!m_bMouseMoved)
			OnLButtonClick();
		OnLButtonDragRelease(event);
	}

	m_bLMouseButton = false;	// left mouse button no longer down
}

void BuilderView::OnLeftDoubleClick(wxMouseEvent& event)
{
	GetCanvasPosition(event, m_DownPoint);
	m_CurPoint = m_LastPoint = m_DownPoint;
	DPoint2 point(ox(m_CurPoint.x), oy(m_CurPoint.y));

	vtLayerPtr pL = GetMainFrame()->GetActiveLayer();
	if (!pL) return;

	switch (pL->GetType())
	{
	case LT_ROAD:
		{
			if (m_mode == LB_Road || m_mode == LB_Node)
			{
				vtRoadLayer *pRL = (vtRoadLayer *)pL;
				OnDblClickElement(pRL, point);
			}
			else
				OnLeftDown(event);
		}
		break;
	case LT_STRUCTURE:
		{
			vtStructureLayer *pSL = (vtStructureLayer *)pL;
			OnDblClickElement(pSL, point);
		}
		break;
	}
}

void BuilderView::OnDblClickElement(vtRoadLayer *pRL, DPoint2 &point)
{
	DRECT world_bound, bound2;

	//error is how close to the road/node can we be off by?
	float error = BoundaryPixels();

	//undo the select caused by double clicking
	if (m_mode == LB_Node)
	{
		pRL->SelectNode(point, error, bound2);
		pRL->EditNodeProperties(point, error, world_bound);
	}
	else
	{
		pRL->SelectRoad(point, error, bound2);
		pRL->EditRoadProperties(point, error, world_bound);
	}
	wxRect screen_bound = WorldToWindow(world_bound);
	IncreaseRect(screen_bound, BOUNDADJUST);
	Refresh(TRUE, &screen_bound);
}

void BuilderView::OnDblClickElement(vtStructureLayer *pSL, DPoint2 &point)
{
}


void BuilderView::OnLButtonClick()
{
	vtLayerPtr pL = GetMainFrame()->GetActiveLayer();
	if (!pL) return;

	if (pL->GetType() == LT_ROAD)
	{
		switch (m_mode)
		{
		case LB_Path:
//			OnLButtonClickPath();
			break;
		case LB_Road:
		case LB_Node:
		case LB_RoadExtend:
			OnLButtonClickElement((vtRoadLayer *)pL);
			break;
		case LB_Dir:
			OnLButtonClickDirection((vtRoadLayer *)pL);
			break;
		case LB_RoadEdit:
			OnLButtonClickRoadEdit((vtRoadLayer *)pL);
			break;
		}
	}
	if (m_mode == LB_FSelect)
		OnLButtonClickFeature(pL);
}

void BuilderView::OnLButtonDragRelease(wxMouseEvent& event)
{
	if (m_bPanning)
		EndPan();
	if (m_bBoxing)
		EndBox(event);

	if (m_iDragSide)
	{
		GetMainFrame()->m_area.Sort();
		m_iDragSide = 0;
	}

	if (m_mode == LB_Dist)
		OnDragDistance();
	if (m_mode == LB_RoadEdit)
		OnDragRoadEdit();
}

void BuilderView::OnDragDistance()
{
	vtProjection proj;
	GetMainFrame()->GetProjection(proj);

	bool bUTM = proj.IsUTM();

	DPoint2 p1, p2;
	p1.x = ox(m_DownPoint.x);
	p1.y = oy(m_DownPoint.y);
	p2.x = ox(m_LastPoint.x);
	p2.y = oy(m_LastPoint.y);
	DPoint2 diff_projected;
	DPoint2 diff_degrees;
	DPoint2 diff_meters;

	if (proj.IsGeographic())
	{
		diff_degrees = p2 - p1;
		diff_meters.Set(0, 0);		// TODO: expand this assumption
	}
	else
	{
		diff_projected = p2 - p1;
		diff_meters = diff_projected;	// TODO: expand this assumption

		vtProjection geo;
		CreateSimilarGeographicProjection(proj, geo);

		OCT *trans = OGRCreateCoordinateTransformation(&proj, &geo);

		DPoint2 geo1 = p1, geo2 = p2;
		trans->Transform(1, &geo1.x, &geo1.y);
		trans->Transform(1, &geo2.x, &geo2.y);

		diff_degrees = geo2 - geo1;

		delete trans;
	}

	wxString str1, str2, str3;
	str1.Printf("%s, %s",
		FormatCoord(false, diff_meters.x),
		FormatCoord(false, diff_meters.y));
	str2.Printf("%s, %s",
		FormatCoord(true, diff_degrees.x), 
		FormatCoord(true, diff_degrees.y));
	str3.Printf("%s",
		FormatCoord(false, diff_meters.Length()));

	wxDialog dlg(NULL, -1, "Distance", wxDefaultPosition);
	DistanceDialogFunc(&dlg, TRUE ); 
	wxTextCtrl *ctr1 = (wxTextCtrl *) dlg.FindWindow(ID_XY);
	ctr1->SetValue(str1);

	wxTextCtrl *ctr2 = (wxTextCtrl *) dlg.FindWindow(ID_DUMMY);
	ctr2->SetValue(str2);

	wxTextCtrl *ctr3 = (wxTextCtrl *) dlg.FindWindow(ID_DIST);
	ctr3->SetValue(str3);

	dlg.ShowModal();
}

void BuilderView::OnDragRoadEdit()
{
	if (m_pEditingRoad != NULL && m_iEditingPoint >= 0)
	{
		RefreshRoad(m_pEditingRoad);
		DPoint2 p = m_pEditingRoad->GetAt(m_iEditingPoint);
		p += (m_CurLocation - m_DownLocation);
		m_pEditingRoad->SetAt(m_iEditingPoint, p);
		m_pEditingRoad->ComputeExtent();
		RefreshRoad(m_pEditingRoad);
	}
	m_iEditingPoint = -1;
}

void BuilderView::OnLButtonClickElement(vtRoadLayer *pRL)
{
	DRECT world_bound;

	// error is how close to the road/node can we be off by?
	float error = BoundaryPixels();

	bool returnVal = false;
	if (m_mode == LB_Node)
		returnVal = pRL->SelectNode(m_DownLocation, error, world_bound);
	else if (m_mode == LB_Road)
		returnVal = pRL->SelectRoad(m_DownLocation, error, world_bound);
	else if (m_mode == LB_RoadExtend)
		returnVal = pRL->SelectAndExtendRoad(m_DownLocation, error, world_bound);

	if (returnVal)
	{
		wxRect screen_bound = WorldToWindow(world_bound);
		IncreaseRect(screen_bound, BOUNDADJUST);
		Refresh(TRUE, &screen_bound);
		wxString str = wxString::Format("Selected 1 %s (%d total)",
			m_mode == LB_Node ? "Node" : "Road",
			m_mode == LB_Node ? pRL->GetSelectedNodes() : pRL->GetSelectedRoads());
		GetMainFrame()->SetStatusText(str);
	}
	else
	{
		DeselectAll();
		GetMainFrame()->SetStatusText("Deselected all");
	}
}


void BuilderView::OnLButtonClickDirection(vtRoadLayer *pRL)
{
	// see if there is a road or node at m_DownPoint
	// error is how close to the road can we be off by?
	float error = BoundaryPixels();

	RoadEdit *pRoad = pRL->FindRoad(m_DownLocation, error);
	if (pRoad)
	{
		pRL->ToggleRoadDirection(pRoad);
		RefreshRoad(pRoad);
	}
}

void BuilderView::OnLButtonClickRoadEdit(vtRoadLayer *pRL)
{
	// see if there is a road or node at m_DownPoint
	float error = BoundaryPixels();
	bool redraw = false;

	RoadEdit *pRoad = pRL->FindRoad(m_DownLocation, error);
	if (pRoad != m_pEditingRoad)
	{
		if (m_pEditingRoad)
		{
			RefreshRoad(m_pEditingRoad);
			m_pEditingRoad->m_bDrawPoints = false;
		}
		m_pEditingRoad = pRoad;
		if (m_pEditingRoad)
		{
			RefreshRoad(m_pEditingRoad);
			m_pEditingRoad->m_bDrawPoints = true;
		}
	}
}

void BuilderView::RefreshRoad(RoadEdit *pRoad)
{
	DRECT world_bound = pRoad->m_extent;
	wxRect screen_bound = WorldToWindow(world_bound);
	IncreaseRect(screen_bound, BOUNDADJUST);
	Refresh(TRUE, &screen_bound);
}

void BuilderView::OnLButtonClickFeature(vtLayerPtr pL)
{
	if (pL->GetType() == LT_STRUCTURE)
	{
		vtStructureLayer *pSL = (vtStructureLayer *)pL;

		// first do a deselect-all
		pSL->DeselectAll();

		// see if there is a building at m_DownPoint
		vtStructure *str = pSL->FindBuilding(m_DownLocation, odx(12)); // 12 pixel margin
		if (str)
			str->Select(!str->IsSelected());

		Refresh(FALSE);
	}
}

////////////////

void BuilderView::OnMiddleDown(wxMouseEvent& event)
{
	m_bMMouseButton = true;
	m_bMouseMoved = false;	

	// save the point where the user clicked
	m_DownClient = event.GetPosition();

	GetCanvasPosition(event, m_DownPoint);
	m_CurPoint = m_DownPoint;
	CaptureMouse();			// capture mouse

	BeginPan();
}

void BuilderView::OnMiddleUp(wxMouseEvent& event)
{
	if (m_bPanning)
		EndPan();

	ReleaseMouse();
}

void BuilderView::OnRightDown(wxMouseEvent& event)
{
	m_bRMouseButton = true;
	CaptureMouse();			// capture mouse
}

void BuilderView::OnRightUp(wxMouseEvent& event)
{
	m_bRMouseButton = false;	//right mouse button no longer down
	ReleaseMouse();

	vtLayerPtr pL = GetMainFrame()->GetActiveLayer();
	if (!pL)
		return;

	switch (pL->GetType())
	{
	case LT_ROAD:
		OnRightUp((vtRoadLayer *)pL);
		break;
	case LT_STRUCTURE:
		OnRightUp((vtStructureLayer *)pL);
		break;
	}
}

void BuilderView::OnRightUp(vtRoadLayer *pRL) 
{
	//if we are not clicked close to a single item, edit all selected items.
	bool status;
	if (m_mode == LB_Node)
		status = pRL->EditNodesProperties();
	else
		status = pRL->EditRoadsProperties();
	if (status)
		Refresh();
}

void BuilderView::OnRightUp(vtStructureLayer *pSL) 
{
	bool status = pSL->EditBuildingProperties();
	if (status)
		Refresh();
}

void BuilderView::OnMouseMove(wxMouseEvent& event)
{
	wxPoint point = event.GetPosition();
	static wxPoint lastpoint;

	if (point == lastpoint)
		return;
	GetMainFrame()->SetStatusText("Ready");
	lastpoint = point;

	GetCanvasPosition(event, m_CurPoint);
	object(m_CurPoint, m_CurLocation);

	if (m_bLMouseButton || m_bMMouseButton || m_bRMouseButton)
	{
		wxPoint diff = m_CurPoint - m_DownPoint;
		int mag = abs(diff.x) + abs(diff.y);
		if (mag > 5 && !m_bMouseMoved)
			m_bMouseMoved = true;
	}

	if (m_bPanning)
		DoPan(point);

	// left button click and drag selects/deselects multiple (or zooms)
	if (m_bLMouseButton)
	{
		if (m_bBoxing)
			DoBox(m_CurPoint);
		if (m_iDragSide)
			DoArea(m_CurPoint);
		if (m_mode == LB_Dist)
		{
			wxClientDC dc(this);
			PrepareDC(dc);
			wxPen pen(*wxBLACK_PEN);
			dc.SetPen(pen);
			dc.SetLogicalFunction(wxINVERT);

			dc.DrawLine(m_DownPoint, m_LastPoint);
			dc.DrawLine(m_DownPoint, m_CurPoint);
		}
		else if (m_mode == LB_BldEdit && m_bRubber)
		{
			wxClientDC dc(this);
			PrepareDC(dc);
			wxPen pen(*wxBLACK_PEN);
			dc.SetPen(pen);
			dc.SetLogicalFunction(wxINVERT);

			vtStructureLayer *pSL = GetMainFrame()->GetActiveStructureLayer();
			pSL->DrawBuilding(&dc, this, &m_EditBuilding);

			if (m_bDragCenter)
				UpdateMove();
			else if (m_bRotate)
				UpdateRotate();
			else
				UpdateResizeScale();

			pSL->DrawBuilding(&dc, this, &m_EditBuilding);
		}
	}

	// middle button click and drag moves viewpoint
	else if (m_bMMouseButton)
	{
		if (m_bPanning)
			DoPan(point);
	}
	GetMainFrame()->RefreshStatusBar();

	m_LastPoint = m_CurPoint;
}

void BuilderView::UpdateMove()
{
	DPoint2 p;
	DPoint2 moved_by = m_CurLocation - m_DownLocation;

	if (m_EditBuilding.GetShape() == SHAPE_POLY)
	{
		DLine2 dl = m_pCurBuilding->GetFootprint();
		for (int i = 0; i < dl.GetSize(); i++)
		{
			p = dl.GetAt(i);
			p += moved_by;
			dl.SetAt(i, p);
		}
		m_EditBuilding.SetFootprint(dl);
	}
	p = m_pCurBuilding->GetLocation();
	p += moved_by;
	m_EditBuilding.SetLocation(p.x, p.y);

	if (m_EditBuilding.GetShape() == SHAPE_RECTANGLE)
		m_EditBuilding.RectToPoly();
}

void BuilderView::UpdateResizeScale()
{
	DPoint2 moved_by = m_CurLocation - m_DownLocation;

	if (m_bShift)
		int foo = 1;

	DPoint2 origin = m_pCurBuilding->GetLocation();
	DPoint2 diff1 = m_DownLocation - origin;
	DPoint2 diff2 = m_CurLocation - origin;
	float fScale = diff2.Length() / diff1.Length();

	if (m_EditBuilding.GetShape() == SHAPE_RECTANGLE)
	{
		float fRotation;
		m_pCurBuilding->GetRotation(fRotation);
		if (fRotation == -1.0f) fRotation = 0.0f;
		diff1.Rotate(-fRotation);
		diff2.Rotate(-fRotation);

		DPoint2 ratio;
		if (m_bShift)
			// Scale evenly
			ratio.x = ratio.y = fScale;
		else
			// Resize
			ratio.Set(diff2.x / diff1.x, diff2.y / diff1.y);

		float fWidth, fDepth;
		m_pCurBuilding->GetRectangle(fWidth, fDepth);
		fWidth *= (float) ratio.x;
		fDepth *= (float) ratio.y;

		// stay positive
		if (fWidth < 0.0f) fWidth = -fWidth;
		if (fDepth < 0.0f) fDepth = -fDepth;
		m_EditBuilding.SetRectangle(fWidth, fDepth);

		m_EditBuilding.RectToPoly();
	}

	if (m_EditBuilding.GetShape() == SHAPE_POLY)
	{
		DLine2 dl = m_pCurBuilding->GetFootprint();
		DPoint2 p;
		if (m_bShift)
		{
			// Scale evenly
			for (int i = 0; i < dl.GetSize(); i++)
			{
				p = dl.GetAt(i);
				p -= origin;
				p *= fScale;
				p += origin;
				dl.SetAt(i, p);
			}
		}
		else
		{
			// drag individual corner points
			p = dl.GetAt(m_iCurCorner);
			p += moved_by;
			dl.SetAt(m_iCurCorner, p);
		}
		m_EditBuilding.SetFootprint(dl);
	}
}

void BuilderView::UpdateRotate()
{
	DPoint2 origin = m_pCurBuilding->GetLocation();
	DPoint2 original_vector = m_DownLocation - origin;
	double length1 = original_vector.Length();
	double angle1 = atan2(original_vector.y, original_vector.x);

	DPoint2 cur_vector = m_CurLocation - origin;
	double length2 = cur_vector.Length();
	double angle2 = atan2(cur_vector.y, cur_vector.x);

//	double length_diff = length2 / length1;
	double angle_diff = angle2 - angle1;

	if (m_EditBuilding.GetShape() == SHAPE_POLY)
	{
		DLine2 dl = m_pCurBuilding->GetFootprint();
		DPoint2 p;
		for (int i = 0; i < dl.GetSize(); i++)
		{
			p = dl.GetAt(i);
			p -= origin;
//			p *= length_diff;
			p.Rotate(angle_diff);
			p += origin;
			dl.SetAt(i, p);
		}
		m_EditBuilding.SetFootprint(dl);
	}

	if (m_EditBuilding.GetShape() == SHAPE_RECTANGLE)
	{
//		float fWidth, fHeight;
//		m_pCurBuilding->GetRectangle(fWidth, fHeight);
//		m_EditBuilding.SetRectangle(fWidth * length_diff, fHeight * length_diff);

		float original_angle;
		m_pCurBuilding->GetRotation(original_angle);
		if (original_angle == -1.0f) original_angle = 0.0f;
		m_EditBuilding.SetRotation(original_angle + angle_diff);

		m_EditBuilding.RectToPoly();
	}
}

//////////////////
// Keyboard shortcuts

void BuilderView::OnChar(wxKeyEvent& event)
{
	bool ctrl = event.ControlDown();
	int code = event.KeyCode();

	if (code == ' ')
	{
		SetMode(LB_Pan);
		SetCorrectCursor();
	}
	else if (code == WXK_ADD && ctrl)
	{
		SetScale(GetScale() * 1.2);
	}
	else if (code == WXK_SUBTRACT && ctrl)
	{
		SetScale(GetScale() / 1.2);
	}
	else
		vtScaledView::OnChar(event);
}