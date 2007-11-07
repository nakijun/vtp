//
// RoadLayer.cpp
//
// Copyright (c) 2001-2006 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "RoadLayer.h"
#include "NodeDlg.h"
#include "RoadDlg.h"
#include "Frame.h"
#include "Helper.h"
#include "BuilderView.h"
#include "vtui/Helper.h"

bool vtRoadLayer::m_bDrawNodes = false;
bool vtRoadLayer::m_bShowDirection = false;
bool vtRoadLayer::m_bDrawWidth = false;

//////////////////////////////////////////////////////////////////////

vtRoadLayer::vtRoadLayer() : vtLayer(LT_ROAD)
{
	wxString name = _("Untitled");
	name += _T(".rmf");
	SetLayerFilename(name);
}

vtRoadLayer::~vtRoadLayer()
{
}

///////////////////////////////////////////////////////////////////////
// Standard layer overrides
//

bool vtRoadLayer::GetExtent(DRECT &rect)
{
	if (NumNodes() == 0 && NumLinks() == 0)
		return false;

	rect = GetMapExtent();
	return true;
}

void vtRoadLayer::DrawLayer(wxDC* pDC, vtScaledView *pView)
{
	Draw(pDC, pView, m_bDrawNodes);
}

bool vtRoadLayer::OnSave()
{
	return WriteRMF(GetLayerFilename().mb_str(wxConvUTF8));
}

bool vtRoadLayer::OnLoad()
{
	bool success = ReadRMF(GetLayerFilename().mb_str(wxConvUTF8), true, true, true);
	if (!success)
		return false;

	// Set visual properties
	for (NodeEdit *pN = GetFirstNode(); pN; pN = pN->GetNext())
	{
		pN->DetermineVisualFromLinks();
	}

	// Pre-process some link attributes
	for (LinkEdit *pL = GetFirstLink(); pL; pL = pL->GetNext())
	{
		pL->m_fLength = pL->Length();

		//set the bounding box for the link
		pL->ComputeExtent();

		// clean up link direction info
		if ((pL->m_iFlags & (RF_FORWARD|RF_REVERSE)) == 0)
			pL->m_iFlags |= (RF_FORWARD|RF_REVERSE);
	}
	return true;
}

void vtRoadLayer::GetProjection(vtProjection &proj)
{
	proj = vtRoadMap::GetProjection();
}

bool vtRoadLayer::AppendDataFrom(vtLayer *pL)
{
	// safety check
	if (pL->GetType() != LT_ROAD)
		return false;

	vtRoadLayer *pFrom = (vtRoadLayer *)pL;

	// add nodes to our list
	TNode *n = pFrom->GetFirstNode();
	while (n)
	{
		TNode *next = n->m_pNext;
		n->m_pNext = m_pFirstNode;
		m_pFirstNode = n;
		n = next;
	}
	// add links to our list
	TLink *r = pFrom->GetFirstLink();
	while (r)
	{
		TLink *next = r->m_pNext;
		r->m_pNext = m_pFirstLink;
		m_pFirstLink = r;
		r = next;
	}
	pFrom->m_pFirstLink = NULL;
	pFrom->m_pFirstNode = NULL;

	ComputeExtents();

	return true;
}

///////////////////////////////////////////////////////////////////////

int vtRoadLayer::GetSelectedNodes()
{
	int count = 0;
	for (NodeEdit *n = GetFirstNode(); n; n = n->GetNext())
		if (n->IsSelected()) count++;
	return count;
}

int vtRoadLayer::GetSelectedLinks()
{
	int count = 0;
	for (LinkEdit *n = GetFirstLink(); n; n = n->GetNext())
		if (n->IsSelected()) count++;
	return count;
}

void vtRoadLayer::ToggleLinkDirection(LinkEdit *pLink)
{
	switch (pLink->m_iFlags & (RF_FORWARD|RF_REVERSE))
	{
		case RF_FORWARD:
			pLink->m_iFlags &= ~RF_FORWARD;
			pLink->m_iFlags |= RF_REVERSE;
			break;
		case RF_REVERSE:
			pLink->m_iFlags |= RF_FORWARD;
			break;
		case (RF_FORWARD|RF_REVERSE):
			pLink->m_iFlags &= ~RF_REVERSE;
			break;
	}
	SetModified(true);
}

void vtRoadLayer::MoveSelectedNodes(const DPoint2 &offset)
{
	for (NodeEdit *n = GetFirstNode(); n; n=n->GetNext())
	{
		if (n->IsSelected())
			n->Translate(offset);
	}
}

bool vtRoadLayer::TransformCoords(vtProjection &proj_new)
{
	// Create conversion object
	vtProjection Source;
	GetProjection(Source);

	OCT *trans = CreateCoordTransform(&Source, &proj_new);
	if (!trans)
		return false;		// inconvertible projections

	LinkEdit *l;
	NodeEdit *n;
	for (l = GetFirstLink(); l; l=l->GetNext())
	{
		for (unsigned int i = 0; i < l->GetSize(); i++)
			trans->Transform(1, &(l->GetAt(i).x), &(l->GetAt(i).y));
	}
	for (n = GetFirstNode(); n; n=n->GetNext())
		trans->Transform(1, &(n->m_p.x), &(n->m_p.y));

	delete trans;

	// recompute link extents
	for (l = GetFirstLink(); l; l=l->GetNext())
	{
		l->ComputeExtent();
		l->m_bSidesComputed = false;
	}

	// set the vtRoadMap projection
	m_proj = proj_new;

	m_bValidExtents = false;
	return true;
}

void vtRoadLayer::SetProjection(const vtProjection &proj)
{
	if (m_proj == proj)
		return;

	m_proj = proj;

	// Extents are still valid, but we should recompute things like displayed
	// link widths, which may have different values in another projection.
	for (LinkEdit *link = GetFirstLink(); link; link = link->GetNext())
		link->m_bSidesComputed = false;

	SetModified(true);
}

void vtRoadLayer::Offset(const DPoint2 &p)
{
	bool bSelLinks = (NumSelectedLinks() > 0);
	bool bSelNodes = (NumSelectedNodes() > 0);
	bool bSelected = bSelLinks || bSelNodes;
	for (LinkEdit *link = GetFirstLink(); link; link=link->GetNext())
	{
		for (unsigned int i = 0; i < link->GetSize(); i++)
		{
			if (bSelected && !link->IsSelected())
				continue;
			link->GetAt(i) += p;
		}
		link->m_bSidesComputed = false;
		if (bSelLinks && !bSelNodes)
		{
			link->GetNode(0)->m_p += p;
			link->GetNode(1)->m_p += p;
		}
	}
	for (NodeEdit *node = GetFirstNode(); node; node=node->GetNext())
	{
		if (bSelected && !node->IsSelected())
			continue;
		node->m_p += p;
		if (!bSelLinks && bSelNodes)
		{
			for (int i = 0; i < node->m_iLinks; i++)
			{
				TLink *l1 = node->GetLink(i);
				if (l1->GetNode(0) == node)
					l1->SetAt(0, node->m_p);
				else
					l1->SetAt(l1->GetSize()-1, node->m_p);
			}
		}
	}

	// recompute link extents
	for (LinkEdit *r2 = GetFirstLink(); r2; r2=r2->GetNext())
		r2->ComputeExtent();

	m_bValidExtents = false;
}

void vtRoadLayer::GetPropertyText(wxString &strIn)
{
	strIn += _("Network of links.\n");

	wxString str;
	str.Printf(_("Nodes: %d, selected: %d\n"), NumNodes(), NumSelectedNodes());
	strIn += str;
	str.Printf(_("Links: %d, selected: %d\n"), NumLinks(), NumSelectedLinks());
	strIn += str;
}

void vtRoadLayer::OnLeftDown(BuilderView *pView, UIContext &ui)
{
	if (ui.mode == LB_LinkEdit && ui.m_pEditingRoad)
	{
		double closest = 1E8;
		int closest_i=-1;
		for (unsigned int i = 0; i < ui.m_pEditingRoad->GetSize(); i++)
		{
			DPoint2 diff = ui.m_DownLocation - ui.m_pEditingRoad->GetAt(i);
			double dist = diff.Length();
			if (dist < closest)
			{
				closest = dist;
				closest_i = i;
			}
		}
		int pixels = pView->sdx(closest);
		if (pixels < 8)
		{
			// begin dragging point
			ui.m_iEditingPoint = closest_i;
			return;
		}
		else
			ui.m_iEditingPoint = -1;
	}
	if (ui.mode == LB_Dir)
	{
		LinkEdit *pLink = FindLink(ui.m_DownLocation, pView->odx(5));
		if (pLink)
		{
			ToggleLinkDirection(pLink);
			pView->RefreshRoad(pLink);
		}
	}
	if (ui.mode == LB_LinkEdit)
	{
		// see if there is a link or node at m_DownPoint
		float error = pView->odx(5);

		LinkEdit *pLink = FindLink(ui.m_DownLocation, error);
		if (pLink != ui.m_pEditingRoad)
		{
			if (ui.m_pEditingRoad)
			{
				pView->RefreshRoad(ui.m_pEditingRoad);
				ui.m_pEditingRoad->m_bDrawPoints = false;
			}
			ui.m_pEditingRoad = pLink;
			if (ui.m_pEditingRoad)
			{
				pView->RefreshRoad(ui.m_pEditingRoad);
				ui.m_pEditingRoad->m_bDrawPoints = true;
			}
		}
	}
	if (ui.mode == LB_LinkExtend)
	{
		pView->OnLButtonClickElement(this);
	}
}

void vtRoadLayer::OnLeftUp(BuilderView *pView, UIContext &ui)
{
	if (ui.mode != LB_LinkEdit)
		return;

	if (ui.m_pEditingRoad != NULL && ui.m_iEditingPoint >= 0)
	{
		LinkEdit *le = ui.m_pEditingRoad;
		pView->RefreshRoad(le);	// erase where it was
		DPoint2 p = le->GetAt(ui.m_iEditingPoint);
		p += (ui.m_CurLocation - ui.m_DownLocation);
		le->SetAt(ui.m_iEditingPoint, p);
		le->Dirtied();
		pView->RefreshRoad(le);	// draw where it is now

		// see if we changed the first or last point, affects some node
		int num_points = le->GetSize();
		NodeEdit *node = NULL;
		if (ui.m_iEditingPoint == 0)
			node = le->GetNode(0);
		if (ui.m_iEditingPoint == num_points-1)
			node = le->GetNode(1);
		if (node)
		{
			node->m_p = p;
			for (int i = 0; i < node->m_iLinks; i++)
			{
				LinkEdit *link = node->GetLink(i);
				if (link->GetNode(0) == node)
					link->SetAt(0, p);
				if (link->GetNode(1) == node)
					link->SetAt(link->GetSize()-1, p);
				link->Dirtied();
			}
			pView->Refresh();
		}
		// We have changed the layer
		SetModified(true);
	}
	ui.m_iEditingPoint = -1;
}

void vtRoadLayer::OnRightUp(BuilderView *pView, UIContext &ui)
{
	//if we are not clicked close to a single item, edit all selected items.
	bool status;
	if (ui.mode == LB_Node)
		status = EditNodesProperties();
	else
		status = EditLinksProperties();
	if (status)
	{
		SetModified(true);
		pView->Refresh();
		GetMainFrame()->RefreshTreeStatus();
	}
}

void vtRoadLayer::OnLeftDoubleClick(BuilderView *pView, UIContext &ui)
{
	DRECT world_bound, bound2;

	// epsilon is how close to the link/node can we be off by?
	float epsilon = pView->odx(5);
	bool bRefresh = false;

	if (ui.mode == LB_Node)
	{
		SelectNode(ui.m_DownLocation, epsilon, bound2);
		EditNodeProperties(ui.m_DownLocation, epsilon, world_bound);
		bRefresh = true;
	}
	else if (ui.mode == LB_Link)
	{
		SelectLink(ui.m_DownLocation, epsilon, bound2);
		EditLinkProperties(ui.m_DownLocation, epsilon, world_bound);
		bRefresh = true;
	}
	if (bRefresh)
	{
		wxRect screen_bound = pView->WorldToWindow(world_bound);
		IncreaseRect(screen_bound, 5);
		pView->Refresh(TRUE, &screen_bound);
	}
}

bool vtRoadLayer::EditNodeProperties(const DPoint2 &point, float epsilon,
									 DRECT &bound)
{
	NodeEdit *node = (NodeEdit *) FindNodeAtPoint(point, epsilon);
	if (node)
	{
		DPoint2 p = node->m_p;
		bound.SetRect(p.x-epsilon, p.y+epsilon, p.x+epsilon, p.y-epsilon);
		return node->EditProperties(this);
	}
	return false;
}

bool vtRoadLayer::EditLinkProperties(const DPoint2 &point, float error,
									 DRECT &bound)
{
	LinkEdit* bestRSoFar = NULL;
	double dist = error;
	double b;
	bool RFound = false;

	DRECT target(point.x-error, point.y+error, point.x+error, point.y-error);

	for (LinkEdit* curLink = GetFirstLink(); curLink; curLink = curLink->GetNext())
	{
		if (curLink->WithinExtent(target))
		{
			b = curLink->DistanceToPoint(point);
			if (b < dist)
			{
				bestRSoFar = curLink;
				dist = b;
				RFound = true;
			}
		}
	}

	if (RFound)
	{
		bound = bestRSoFar->m_extent;
		return bestRSoFar->EditProperties(this);
	}

	return false;
}

bool vtRoadLayer::EditNodesProperties()
{
	int count = 0;
	NodeEdit *node=NULL;

	for (NodeEdit* n = GetFirstNode(); n; n = n->GetNext())
	{
		if (!n->IsSelected())
			continue;
		count++;
		node = n;
	}
	if (count == 0)
		return false;

	NodeDlg dlg(NULL, -1, _("Node Properties"));
	if (count == 1)
		dlg.SetNode(node, this);
	else
		dlg.SetNode(NULL, this);
	return (dlg.ShowModal() == wxID_OK);
}

bool vtRoadLayer::EditLinksProperties()
{
	int count = 0;
	LinkEdit *link=NULL;

	//only bring up dialog is there is a selected link.
	for (LinkEdit* r = GetFirstLink(); r; r = r->GetNext())
	{
		if (!r->IsSelected())
			continue;
		count++;
		link = r;
	}
	if (count == 0)
		return false;

	RoadDlg dlg(NULL, -1, _("Link Properties"));
	if (count == 1)
		dlg.SetRoad(link, this);	//only one link found
	else
		dlg.SetRoad(NULL, this);
	return (dlg.ShowModal() == wxID_OK);
}

bool vtRoadLayer::SelectArea(const DRECT &box, bool nodemode, bool crossSelect)
{
	bool ret = false;
	int selected;
	if (nodemode)
	{
		selected = SelectNodes(box, true);
		wxString str = wxString::Format(_("Selected %d nodes"), selected);
		if (selected) SetMessageText(str);
		ret = (selected != 0);
	}
	else
	{
		if (crossSelect)
			selected = CrossSelectLinks(box, true);
		else
			selected = SelectLinks(box, true);

		wxString str = wxString::Format(_("Selected %d links"), selected);
		if (selected) SetMessageText(str);
		ret = (selected != 0);
	}
	return ret;
}

#include "ElevLayer.h"
#include "vtdata/ElevationGrid.h"

void vtRoadLayer::CarveRoadway(vtElevLayer *pElev, float margin)
{
	vtElevationGrid	*grid = pElev->m_pGrid;

	if (!pElev || !grid)
		return;

	// how many units to flatten on either side of the roadway, past the
	//  physical edge of the link surface
	float shoulder = margin;
	float fade = margin;

	OpenProgressDialog(_("Scanning Grid against Roads"));

	float half;
	LinkEdit *pLink;
	for (pLink = GetFirstLink(); pLink; pLink = pLink->GetNext())
	{
		pLink->ComputeExtent();
		half = pLink->m_fWidth / 2 + shoulder + fade;
		pLink->m_extent.Grow(half, half);
	}

	int altered_heixels = 0;
	float height;
	int linkpoint;
	float fractional;
	double a, b, total;
	DPoint3 loc;
	int i, j;
	int xsize, ysize;
	grid->GetDimensions(xsize, ysize);
	for (i = 0; i < xsize; i++)
	{
		UpdateProgressDialog(i*100/xsize);
		for (j = 0; j < ysize; j++)
		{
			grid->GetEarthLocation(i, j, loc);
			DPoint2 p2(loc.x, loc.y);

			for (pLink = GetFirstLink(); pLink; pLink = pLink->GetNext())
			{
				if (!pLink->WithinExtent(p2))
					continue;

				// Find position in link coordinates.
				// These factors (a,b) are similar to what Pete Willemsen calls
				//  Curvilinear Coordinates: distance and offset.
				DPoint2 closest;
				total = pLink->GetLinearCoordinates(p2, a, b, closest,
					linkpoint, fractional, false);
				half = pLink->m_fWidth / 2 + shoulder + fade;

				// Check if the point is actually on the link
				if (a < 0 || a > total || b < -half || b > half)
					continue;

				// Don't use the height of the ground at the middle of the link.
				// That assumes the link is draped perfectly.  In reality,
				//  it's draped based only on the height at each vertex of
				//  the link.  Just use those.
				float alt1, alt2;
				grid->FindAltitudeOnEarth(pLink->GetAt(linkpoint), alt1);
				grid->FindAltitudeOnEarth(pLink->GetAt(linkpoint+1), alt2);
				height = alt1 + (alt2 - alt1) * fractional;

				// If the point falls in the 'fade' region, interpolate
				//  the offset from 1 to 0 across the region.
				if (half - fabs(b) > fade)
					grid->SetFValue(i, j, height);
				else
				{
					float amount = (half - fabs(b)) / fade;
					float current = grid->GetFValue(i, j);
					float diff = height - current;
					grid->SetFValue(i, j, current + amount * diff);
				}
				altered_heixels++;
				break;
			}
		}
	}
	if (altered_heixels)
	{
		grid->ComputeHeightExtents();
		pElev->SetModified(true);
		pElev->ReRender();
	}
	CloseProgressDialog();
}

