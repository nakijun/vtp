//
// A raw data layer, suitable for storing and displaying the type of
// generic spatial data contained in a Shapefile.
//
// Copyright (c) 2001-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifndef RAWLAYERH
#define RAWLAYERH

#include "vtdata/Features.h"
#include "Layer.h"

class vtRawLayer : public vtLayer
{
public:
	vtRawLayer();
	virtual ~vtRawLayer();

	void SetGeomType(OGRwkbGeometryType type);
	OGRwkbGeometryType GetGeomType();
	vtFeatureSet *GetFeatureSet() { return m_pSet; }
	void SetFeatureSet(vtFeatureSet *pSet) { m_pSet = pSet; }

	// implementation of vtLayer methods
	bool GetExtent(DRECT &rect);
	void DrawLayer(wxDC *pDC, vtScaledView *pView);
	bool TransformCoords(vtProjection &proj);
	bool OnSave(bool progress_callback(int) = NULL);
	bool OnLoad();
	bool AppendDataFrom(vtLayer *pL);
	void GetProjection(vtProjection &proj);
	void SetProjection(const vtProjection &proj);
	void Offset(const DPoint2 &p);
	void GetPropertyText(wxString &strIn);
	void OnLeftDown(BuilderView *pView, UIContext &ui);

	void AddPoint(const DPoint2 &p2);
	bool CreateFromOGRLayer(OGRLayer *pOGRLayer);
	bool ReadFeaturesFromWFS(const char *szServerURL, const char *layername);
	bool LoadWithOGR(const char *filename, bool progress_callback(int) = NULL);
	void Scale(double factor);

	void SetDrawStyle(const DrawStyle &style) { m_DrawStyle = style; }
	DrawStyle GetDrawStyle() { return m_DrawStyle; }

	vtProjection *GetAtProjection()
	{
		if (m_pSet)
			return &(m_pSet->GetAtProjection());
		else
			return NULL;
	}
	wxString GetLayerFilename()
	{
		if (m_pSet)
			return wxString(m_pSet->GetFilename(), wxConvUTF8);
		else
			return vtLayer::GetLayerFilename();
	}
	void SetLayerFilename(const wxString &fname)
	{
		if (m_pSet)
			m_pSet->SetFilename((const char *) fname.mb_str(wxConvUTF8));
		vtLayer::SetLayerFilename(fname);
	}

	void ReadGeoURL();
	bool ImportFromXML(const char *fname);

	// speed optimization
	void CreateIndex(int iSize);
	void FreeIndex();

protected:
	vtFeatureSet	*m_pSet;
	DrawStyle	m_DrawStyle;
	bool m_bExtentComputed;
	DRECT m_Extents;
};

#endif	// RAWLAYERH

