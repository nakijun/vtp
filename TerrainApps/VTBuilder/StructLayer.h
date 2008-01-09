//
// StructureLayer.h
//
// Copyright (c) 2001-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifndef STRUCTLAYER_H
#define STRUCTLAYER_H

#include "vtdata/shapelib/shapefil.h"
#include "vtdata/Features.h"
#include "vtdata/StructArray.h"
#include "vtdata/Features.h"
#include "Layer.h"

class vtDLGFile;
class vtElevLayer;

//////////////////////////////////////////////////////////

class vtStructureLayer : public vtLayer, public vtStructureArray
{
public:
	vtStructureLayer();

	// Implement vtLayer methods
	bool GetExtent(DRECT &rect);
	void DrawLayer(wxDC* pDC, vtScaledView *pView);
	bool TransformCoords(vtProjection &proj);
	bool OnSave();
	bool OnLoad();
	void GetProjection(vtProjection &proj);
	void SetProjection(const vtProjection &proj);
	bool AppendDataFrom(vtLayer *pL);
	void Offset(const DPoint2 &p);
	void GetPropertyText(wxString &str);
	bool AskForSaveFilename();

	wxString GetLayerFilename() { return wxString(GetFilename(), wxConvUTF8); }
	void SetLayerFilename(const wxString &fname)
	{
		SetFilename((const char *) fname.mb_str(wxConvUTF8));
		vtLayer::SetLayerFilename(fname);
	}

	// UI event handlers
	void OnLeftDown(BuilderView *pView, UIContext &ui);
	void OnLeftUp(BuilderView *pView, UIContext &ui);
	void OnRightDown(BuilderView *pView, UIContext &ui);
	void OnMouseMove(BuilderView *pView, UIContext &ui);

	void OnLeftDownEditBuilding(BuilderView *pView, UIContext &ui);
	void OnLeftDownBldAddPoints(BuilderView *pView, UIContext &ui);
	void OnLeftDownBldDeletePoints(BuilderView *pView, UIContext &ui);
	void OnLeftDownEditLinear(BuilderView *pView, UIContext &ui);
	void OnLeftDownAddInstance(BuilderView *pView, UIContext &ui);
	void UpdateMove(UIContext &ui);
	void UpdateRotate(UIContext &ui);
	void UpdateResizeScale(BuilderView *pView, UIContext &ui);

	void DrawBuildingHighlight(wxDC* pDC, vtScaledView *pView);
	bool AddElementsFromSHP(const wxString &filename, const vtProjection &proj, DRECT rect);
	void AddElementsFromDLG(vtDLGFile *pDlg);

	bool EditBuildingProperties();
	void AddFoundations(vtElevLayer *pEL);

	void DrawBuilding(wxDC* pDC, vtScaledView *pView, vtBuilding *bld);
	void DrawLinear(wxDC* pDC, vtScaledView *pView, vtFence *fen);

	// inverts selection values on all structures.
	void InvertSelection();
	void DeselectAll();
	int DoBoxSelect(const DRECT &rect, SelectionType st);

	// override to catch edit hightlighting
	virtual void SetEditedEdge(vtBuilding *bld, int lev, int edge);

	void ResolveInstancesOfItems();

protected:
	void DrawStructures(wxDC* pDC, vtScaledView *pView, bool bOnlySelected);

	int		m_size;	// size in pixels of the small crosshair at building center
	bool	m_bPreferGZip;	// user wants their elevation treated as a .gz file
};

#endif	// STRUCTLAYER_H

