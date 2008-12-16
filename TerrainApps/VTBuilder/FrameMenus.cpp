//
//  The menus functions of the main Frame window of the VTBuilder application.
//
// Copyright (c) 2001-2008 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/progdlg.h>
#include <wx/choicdlg.h>
#include <wx/colordlg.h>

#include "vtdata/config_vtdata.h"
#include "vtdata/ChunkLOD.h"
#include "vtdata/ElevationGrid.h"
#include "vtdata/Icosa.h"
#include "vtdata/TripDub.h"
#include "vtdata/vtDIB.h"
#include "vtdata/vtLog.h"
#include "vtdata/WFSClient.h"
#include "vtui/Helper.h"
#include "vtui/ProfileDlg.h"
#include "vtui/ProjectionDlg.h"

#include "gdal_priv.h"

#include "App.h"
#include "Frame.h"
#include "MenuEnum.h"
#include "BuilderView.h"
#include "TreeView.h"
#include "Helper.h"
#include "vtBitmap.h"
#include "vtImage.h"
#include "FileFilters.h"
#include "Options.h"
// Layers
#include "ElevLayer.h"
#include "ImageLayer.h"
#include "RawLayer.h"
#include "RoadLayer.h"
#include "StructLayer.h"
#include "UtilityLayer.h"
#include "VegLayer.h"
#include "WaterLayer.h"
// Dialogs
#include "DistanceDlg2d.h"
#include "DistribVegDlg.h"
#include "ExtentDlg.h"
#include "FeatInfoDlg.h"
#include "GenGridDlg.h"
#include "GeocodeDlg.h"
#include "ImageMapDlg.h"
#include "LayerPropDlg.h"
#include "MapServerDlg.h"
#include "MatchDlg.h"
#include "PrefDlg.h"
#include "RenderDlg.h"
#include "SelectDlg.h"
#include "TSDlg.h"
#include "VegDlg.h"

DECLARE_APP(BuilderApp)

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_MENU(ID_FILE_NEW,		MainFrame::OnProjectNew)
EVT_MENU(ID_FILE_OPEN,		MainFrame::OnProjectOpen)
EVT_MENU(ID_FILE_SAVE,		MainFrame::OnProjectSave)
EVT_MENU(ID_FILE_PREFS,		MainFrame::OnProjectPrefs)
EVT_MENU(ID_SPECIAL_DYMAX_TEXTURES,	MainFrame::OnDymaxTexture)
EVT_MENU(ID_SPECIAL_DYMAX_MAP,	MainFrame::OnDymaxMap)
EVT_MENU(ID_SPECIAL_PROCESS_BILLBOARD,	MainFrame::OnProcessBillboard)
EVT_MENU(ID_SPECIAL_GEOCODE,	MainFrame::OnGeocode)
EVT_MENU(ID_FILE_EXIT,		MainFrame::OnQuit)

EVT_MENU(ID_EDIT_DELETE, MainFrame::OnEditDelete)
EVT_MENU(ID_EDIT_DESELECTALL, MainFrame::OnEditDeselectAll)
EVT_MENU(ID_EDIT_INVERTSELECTION, MainFrame::OnEditInvertSelection)
EVT_MENU(ID_EDIT_CROSSINGSELECTION, MainFrame::OnEditCrossingSelection)

EVT_UPDATE_UI(ID_EDIT_DELETE,	MainFrame::OnUpdateEditDelete)
EVT_UPDATE_UI(ID_EDIT_CROSSINGSELECTION,	MainFrame::OnUpdateCrossingSelection)

EVT_MENU(ID_LAYER_NEW,			MainFrame::OnLayerNew)
EVT_MENU(ID_LAYER_OPEN,			MainFrame::OnLayerOpen)
EVT_MENU(ID_LAYER_SAVE,			MainFrame::OnLayerSave)
EVT_MENU(ID_LAYER_SAVE_AS,		MainFrame::OnLayerSaveAs)
EVT_MENU(ID_LAYER_IMPORT,		MainFrame::OnLayerImport)
EVT_MENU(ID_LAYER_IMPORTTIGER,	MainFrame::OnLayerImportTIGER)
EVT_MENU(ID_LAYER_IMPORTNTF,	MainFrame::OnLayerImportNTF)
EVT_MENU(ID_LAYER_IMPORTUTIL,	MainFrame::OnLayerImportUtil)
EVT_MENU(ID_LAYER_IMPORT_MS,	MainFrame::OnLayerImportMapSource)
EVT_MENU(ID_LAYER_IMPORT_POINT,	MainFrame::OnLayerImportPoint)
EVT_MENU(ID_LAYER_IMPORT_XML,	MainFrame::OnLayerImportXML)
EVT_MENU(ID_LAYER_IMPORT_DXF,	MainFrame::OnLayerImportDXF)
EVT_MENU(ID_LAYER_PROPS,		MainFrame::OnLayerProperties)
EVT_MENU(ID_LAYER_CONVERTPROJ,	MainFrame::OnLayerConvert)
EVT_MENU(ID_LAYER_SETPROJ,		MainFrame::OnLayerSetProjection)
EVT_MENU(ID_LAYER_FLATTEN,		MainFrame::OnLayerFlatten)
EVT_MENU(ID_EDIT_OFFSET,		MainFrame::OnEditOffset)

EVT_UPDATE_UI(ID_LAYER_SAVE,	MainFrame::OnUpdateLayerSave)
EVT_UPDATE_UI(ID_LAYER_SAVE_AS,	MainFrame::OnUpdateLayerSaveAs)
EVT_UPDATE_UI(ID_LAYER_PROPS,	MainFrame::OnUpdateLayerProperties)
EVT_UPDATE_UI(ID_LAYER_FLATTEN,	MainFrame::OnUpdateLayerFlatten)
EVT_UPDATE_UI(ID_EDIT_OFFSET,	MainFrame::OnUpdateEditOffset)

EVT_MENU(ID_VIEW_SHOWLAYER,		MainFrame::OnLayerShow)
EVT_MENU(ID_VIEW_LAYER_UP,		MainFrame::OnLayerUp)
EVT_MENU(ID_VIEW_LAYER_DOWN,	MainFrame::OnLayerDown)
EVT_MENU(ID_VIEW_ZOOMIN,		MainFrame::OnViewZoomIn)
EVT_MENU(ID_VIEW_ZOOMOUT,		MainFrame::OnViewZoomOut)
EVT_MENU(ID_VIEW_ZOOMALL,		MainFrame::OnViewZoomAll)
EVT_MENU(ID_VIEW_ZOOM_LAYER,	MainFrame::OnViewZoomToLayer)
EVT_MENU(ID_VIEW_FULLVIEW,		MainFrame::OnViewFull)
EVT_MENU(ID_VIEW_ZOOM_AREA,		MainFrame::OnViewZoomArea)
EVT_MENU(ID_VIEW_TOOLBAR,		MainFrame::OnViewToolbar)
EVT_MENU(ID_VIEW_LAYERS,		MainFrame::OnViewLayers)
EVT_MENU(ID_VIEW_MAGNIFIER,		MainFrame::OnViewMagnifier)
EVT_MENU(ID_VIEW_PAN,			MainFrame::OnViewPan)
EVT_MENU(ID_VIEW_DISTANCE,		MainFrame::OnViewDistance)
EVT_MENU(ID_VIEW_SETAREA,		MainFrame::OnViewSetArea)
EVT_MENU(ID_VIEW_WORLDMAP,		MainFrame::OnViewWorldMap)
EVT_MENU(ID_VIEW_SHOWUTM,		MainFrame::OnViewUTMBounds)
EVT_MENU(ID_VIEW_PROFILE,		MainFrame::OnViewProfile)
EVT_MENU(ID_VIEW_SCALE_BAR,		MainFrame::OnViewScaleBar)
EVT_MENU(ID_VIEW_OPTIONS,		MainFrame::OnViewOptions)

EVT_UPDATE_UI(ID_VIEW_SHOWLAYER,	MainFrame::OnUpdateLayerShow)
EVT_UPDATE_UI(ID_VIEW_LAYER_UP,		MainFrame::OnUpdateLayerUp)
EVT_UPDATE_UI(ID_VIEW_LAYER_DOWN,	MainFrame::OnUpdateLayerDown)
EVT_UPDATE_UI(ID_VIEW_MAGNIFIER,	MainFrame::OnUpdateMagnifier)
EVT_UPDATE_UI(ID_VIEW_PAN,			MainFrame::OnUpdatePan)
EVT_UPDATE_UI(ID_VIEW_DISTANCE,		MainFrame::OnUpdateDistance)
EVT_UPDATE_UI(ID_VIEW_ZOOM_LAYER,	MainFrame::OnUpdateViewZoomToLayer)
EVT_UPDATE_UI(ID_VIEW_FULLVIEW,		MainFrame::OnUpdateViewFull)
EVT_UPDATE_UI(ID_VIEW_ZOOM_AREA,	MainFrame::OnUpdateViewZoomArea)
EVT_UPDATE_UI(ID_VIEW_TOOLBAR,		MainFrame::OnUpdateViewToolbar)
EVT_UPDATE_UI(ID_VIEW_LAYERS,		MainFrame::OnUpdateViewLayers)
EVT_UPDATE_UI(ID_VIEW_SETAREA,		MainFrame::OnUpdateViewSetArea)
EVT_UPDATE_UI(ID_VIEW_WORLDMAP,		MainFrame::OnUpdateWorldMap)
EVT_UPDATE_UI(ID_VIEW_SHOWUTM,		MainFrame::OnUpdateUTMBounds)
EVT_UPDATE_UI(ID_VIEW_PROFILE,		MainFrame::OnUpdateViewProfile)
EVT_UPDATE_UI(ID_VIEW_SCALE_BAR,	MainFrame::OnUpdateViewScaleBar)

EVT_MENU(ID_ROAD_SELECTROAD,	MainFrame::OnSelectLink)
EVT_MENU(ID_ROAD_SELECTNODE,	MainFrame::OnSelectNode)
EVT_MENU(ID_ROAD_SELECTWHOLE,	MainFrame::OnSelectWhole)
EVT_MENU(ID_ROAD_DIRECTION,		MainFrame::OnDirection)
EVT_MENU(ID_ROAD_EDIT,			MainFrame::OnRoadEdit)
EVT_MENU(ID_ROAD_SHOWNODES,		MainFrame::OnRoadShowNodes)
EVT_MENU(ID_ROAD_SELECTHWY,		MainFrame::OnSelectHwy)
EVT_MENU(ID_ROAD_CLEAN,			MainFrame::OnRoadClean)
EVT_MENU(ID_ROAD_GUESS,			MainFrame::OnRoadGuess)
EVT_MENU(ID_ROAD_FLATTEN,		MainFrame::OnRoadFlatten)

EVT_UPDATE_UI(ID_ROAD_SELECTROAD,	MainFrame::OnUpdateSelectLink)
EVT_UPDATE_UI(ID_ROAD_SELECTNODE,	MainFrame::OnUpdateSelectNode)
EVT_UPDATE_UI(ID_ROAD_SELECTWHOLE,	MainFrame::OnUpdateSelectWhole)
EVT_UPDATE_UI(ID_ROAD_DIRECTION,	MainFrame::OnUpdateDirection)
EVT_UPDATE_UI(ID_ROAD_EDIT,			MainFrame::OnUpdateRoadEdit)
EVT_UPDATE_UI(ID_ROAD_SHOWNODES,	MainFrame::OnUpdateRoadShowNodes)
EVT_UPDATE_UI(ID_ROAD_FLATTEN,		MainFrame::OnUpdateRoadFlatten)

EVT_MENU(ID_ELEV_SELECT,			MainFrame::OnElevSelect)
EVT_MENU(ID_ELEV_REMOVERANGE,		MainFrame::OnRemoveElevRange)
EVT_MENU(ID_ELEV_SETUNKNOWN,		MainFrame::OnElevSetUnknown)
EVT_MENU(ID_ELEV_FILL_FAST,			MainFrame::OnFillFast)
EVT_MENU(ID_ELEV_FILL_SLOW,			MainFrame::OnFillSlow)
EVT_MENU(ID_ELEV_FILL_REGIONS,		MainFrame::OnFillRegions)
EVT_MENU(ID_ELEV_SCALE,				MainFrame::OnScaleElevation)
EVT_MENU(ID_ELEV_VERT_OFFSET,		MainFrame::OnVertOffsetElevation)
EVT_MENU(ID_ELEV_EXPORT,			MainFrame::OnElevExport)
EVT_MENU(ID_ELEV_EXPORT_TILES,		MainFrame::OnElevExportTiles)
EVT_MENU(ID_ELEV_COPY,				MainFrame::OnElevCopy)
EVT_MENU(ID_ELEV_PASTE_NEW,			MainFrame::OnElevPasteNew)
EVT_MENU(ID_ELEV_BITMAP,			MainFrame::OnElevExportBitmap)
EVT_MENU(ID_ELEV_TOTIN,				MainFrame::OnElevToTin)
EVT_MENU(ID_ELEV_MERGETIN,			MainFrame::OnElevMergeTin)
EVT_MENU(ID_ELEV_TRIMTIN,			MainFrame::OnElevTrimTin)

EVT_UPDATE_UI(ID_ELEV_SELECT,		MainFrame::OnUpdateElevSelect)
EVT_UPDATE_UI(ID_ELEV_REMOVERANGE,	MainFrame::OnUpdateIsGrid)
EVT_UPDATE_UI(ID_ELEV_SETUNKNOWN,	MainFrame::OnUpdateIsGrid)
EVT_UPDATE_UI(ID_ELEV_FILL_FAST,	MainFrame::OnUpdateIsGrid)
EVT_UPDATE_UI(ID_ELEV_FILL_SLOW,	MainFrame::OnUpdateIsGrid)
EVT_UPDATE_UI(ID_ELEV_FILL_REGIONS,	MainFrame::OnUpdateIsGrid)
EVT_UPDATE_UI(ID_ELEV_SCALE,		MainFrame::OnUpdateIsElevation)
EVT_UPDATE_UI(ID_ELEV_VERT_OFFSET,	MainFrame::OnUpdateIsElevation)
EVT_UPDATE_UI(ID_ELEV_EXPORT,		MainFrame::OnUpdateIsGrid)
EVT_UPDATE_UI(ID_ELEV_EXPORT_TILES,	MainFrame::OnUpdateIsGrid)
EVT_UPDATE_UI(ID_ELEV_COPY,			MainFrame::OnUpdateIsGrid)
EVT_UPDATE_UI(ID_ELEV_BITMAP,		MainFrame::OnUpdateIsGrid)
EVT_UPDATE_UI(ID_ELEV_TOTIN,		MainFrame::OnUpdateIsGrid)
EVT_UPDATE_UI(ID_ELEV_MERGETIN,		MainFrame::OnUpdateElevMergeTin)
EVT_UPDATE_UI(ID_ELEV_TRIMTIN,		MainFrame::OnUpdateElevTrimTin)

EVT_MENU(ID_IMAGE_REPLACE_RGB,		MainFrame::OnImageReplaceRGB)
EVT_MENU(ID_IMAGE_CREATE_OVERVIEWS,	MainFrame::OnImageCreateOverviews)
EVT_MENU(ID_IMAGE_CREATE_OVER_ALL,	MainFrame::OnImageCreateOverviewsAll)
EVT_MENU(ID_IMAGE_CREATE_MIPMAPS,	MainFrame::OnImageCreateMipMaps)
EVT_MENU(ID_IMAGE_EXPORT_TILES,		MainFrame::OnImageExportTiles)
EVT_MENU(ID_IMAGE_EXPORT_PPM,		MainFrame::OnImageExportPPM)

EVT_UPDATE_UI(ID_IMAGE_REPLACE_RGB,	MainFrame::OnUpdateHaveImageLayer)
EVT_UPDATE_UI(ID_IMAGE_CREATE_OVERVIEWS, MainFrame::OnUpdateHaveImageLayer)
EVT_UPDATE_UI(ID_IMAGE_CREATE_OVER_ALL, MainFrame::OnUpdateHaveImageLayer)
EVT_UPDATE_UI(ID_IMAGE_CREATE_MIPMAPS,	MainFrame::OnUpdateHaveImageLayer)
EVT_UPDATE_UI(ID_IMAGE_EXPORT_TILES,MainFrame::OnUpdateHaveImageLayer)
EVT_UPDATE_UI(ID_IMAGE_EXPORT_PPM,	MainFrame::OnUpdateHaveImageLayer)

EVT_MENU(ID_TOWER_ADD,				MainFrame::OnTowerAdd)
EVT_MENU(ID_TOWER_SELECT,			MainFrame::OnTowerSelect)
EVT_MENU(ID_TOWER_EDIT,				MainFrame::OnTowerEdit)

EVT_UPDATE_UI(ID_TOWER_ADD,			MainFrame::OnUpdateTowerAdd)
EVT_UPDATE_UI(ID_TOWER_SELECT,		MainFrame::OnUpdateTowerSelect)
EVT_UPDATE_UI(ID_TOWER_EDIT,		MainFrame::OnUpdateTowerEdit)

EVT_MENU(ID_VEG_PLANTS,				MainFrame::OnVegPlants)
EVT_MENU(ID_VEG_BIOREGIONS,			MainFrame::OnVegBioregions)
EVT_MENU(ID_VEG_REMAP,				MainFrame::OnVegRemap)
EVT_MENU(ID_VEG_EXPORTSHP,			MainFrame::OnVegExportSHP)

EVT_UPDATE_UI(ID_VEG_REMAP,			MainFrame::OnUpdateVegExportSHP)
EVT_UPDATE_UI(ID_VEG_EXPORTSHP,		MainFrame::OnUpdateVegExportSHP)

EVT_MENU(ID_FEATURE_SELECT,			MainFrame::OnFeatureSelect)
EVT_MENU(ID_FEATURE_PICK,			MainFrame::OnFeaturePick)
EVT_MENU(ID_FEATURE_TABLE,			MainFrame::OnFeatureTable)
EVT_MENU(ID_STRUCTURE_EDIT_BLD,		MainFrame::OnBuildingEdit)
EVT_MENU(ID_STRUCTURE_ADD_POINTS,	MainFrame::OnBuildingAddPoints)
EVT_MENU(ID_STRUCTURE_DELETE_POINTS, MainFrame::OnBuildingDeletePoints)
EVT_MENU(ID_STRUCTURE_ADD_LINEAR,	MainFrame::OnStructureAddLinear)
EVT_MENU(ID_STRUCTURE_EDIT_LINEAR,	MainFrame::OnStructureEditLinear)
EVT_MENU(ID_STRUCTURE_ADD_INST,		MainFrame::OnStructureAddInstances)
EVT_MENU(ID_STRUCTURE_ADD_FOUNDATION, MainFrame::OnStructureAddFoundation)
EVT_MENU(ID_STRUCTURE_CONSTRAIN,	MainFrame::OnStructureConstrain)
EVT_MENU(ID_STRUCTURE_SELECT_USING_POLYGONS, MainFrame::OnStructureSelectUsingPolygons)
EVT_MENU(ID_STRUCTURE_COLOUR_SELECTED_ROOFS, MainFrame::OnStructureColourSelectedRoofs)
EVT_MENU(ID_STRUCTURE_CLEAN_FOOTPRINTS, MainFrame::OnStructureCleanFootprints)
EVT_MENU(ID_STRUCTURE_EXPORT_FOOTPRINTS, MainFrame::OnStructureExportFootprints)
EVT_MENU(ID_STRUCTURE_EXPORT_CANOMA, MainFrame::OnStructureExportCanoma)

EVT_UPDATE_UI(ID_FEATURE_SELECT,		MainFrame::OnUpdateFeatureSelect)
EVT_UPDATE_UI(ID_FEATURE_PICK,			MainFrame::OnUpdateFeaturePick)
EVT_UPDATE_UI(ID_FEATURE_TABLE,			MainFrame::OnUpdateFeatureTable)
EVT_UPDATE_UI(ID_STRUCTURE_EDIT_BLD,	MainFrame::OnUpdateBuildingEdit)
EVT_UPDATE_UI(ID_STRUCTURE_ADD_POINTS,	MainFrame::OnUpdateBuildingAddPoints)
EVT_UPDATE_UI(ID_STRUCTURE_DELETE_POINTS,	MainFrame::OnUpdateBuildingDeletePoints)
EVT_UPDATE_UI(ID_STRUCTURE_ADD_LINEAR,	MainFrame::OnUpdateStructureAddLinear)
EVT_UPDATE_UI(ID_STRUCTURE_EDIT_LINEAR,	MainFrame::OnUpdateStructureEditLinear)
EVT_UPDATE_UI(ID_STRUCTURE_ADD_INST,	MainFrame::OnUpdateStructureAddInstances)
EVT_UPDATE_UI(ID_STRUCTURE_ADD_FOUNDATION,	MainFrame::OnUpdateStructureAddFoundation)
EVT_UPDATE_UI(ID_STRUCTURE_CONSTRAIN,	MainFrame::OnUpdateStructureConstrain)
EVT_UPDATE_UI(ID_STRUCTURE_SELECT_USING_POLYGONS, MainFrame::OnUpdateStructureSelectUsingPolygons)
EVT_UPDATE_UI(ID_STRUCTURE_COLOUR_SELECTED_ROOFS, MainFrame::OnUpdateStructureColourSelectedRoofs)
EVT_UPDATE_UI(ID_STRUCTURE_EXPORT_FOOTPRINTS, MainFrame::OnUpdateStructureExportFootprints)
EVT_UPDATE_UI(ID_STRUCTURE_EXPORT_CANOMA, MainFrame::OnUpdateStructureExportFootprints)

EVT_MENU(ID_RAW_SETTYPE,			MainFrame::OnRawSetType)
EVT_MENU(ID_RAW_ADDPOINTS,			MainFrame::OnRawAddPoints)
EVT_MENU(ID_RAW_ADDPOINT_TEXT,		MainFrame::OnRawAddPointText)
EVT_MENU(ID_RAW_ADDPOINTS_GPS,		MainFrame::OnRawAddPointsGPS)
EVT_MENU(ID_RAW_SELECTCONDITION,	MainFrame::OnRawSelectCondition)
EVT_MENU(ID_RAW_CONVERT_TOTIN,		MainFrame::OnRawConvertToTIN)
EVT_MENU(ID_RAW_EXPORT_IMAGEMAP,	MainFrame::OnRawExportImageMap)
EVT_MENU(ID_RAW_EXPORT_KML,			MainFrame::OnRawExportKML)
EVT_MENU(ID_RAW_GENERATE_ELEVATION,	MainFrame::OnRawGenElevation)
EVT_MENU(ID_RAW_STYLE,				MainFrame::OnRawStyle)
EVT_MENU(ID_RAW_SCALE,				MainFrame::OnRawScale)

EVT_UPDATE_UI(ID_RAW_SETTYPE,			MainFrame::OnUpdateRawSetType)
EVT_UPDATE_UI(ID_RAW_ADDPOINTS,			MainFrame::OnUpdateRawAddPoints)
EVT_UPDATE_UI(ID_RAW_ADDPOINT_TEXT,		MainFrame::OnUpdateRawAddPointText)
EVT_UPDATE_UI(ID_RAW_ADDPOINTS_GPS,		MainFrame::OnUpdateRawAddPointsGPS)
EVT_UPDATE_UI(ID_RAW_SELECTCONDITION,	MainFrame::OnUpdateRawIsActive)
EVT_UPDATE_UI(ID_RAW_CONVERT_TOTIN,		MainFrame::OnUpdateRawIsActive)
EVT_UPDATE_UI(ID_RAW_EXPORT_IMAGEMAP,	MainFrame::OnUpdateRawIsActive)
EVT_UPDATE_UI(ID_RAW_EXPORT_KML,		MainFrame::OnUpdateRawIsActive)
EVT_UPDATE_UI(ID_RAW_GENERATE_ELEVATION,MainFrame::OnUpdateRawGenElevation)
EVT_UPDATE_UI(ID_RAW_STYLE,				MainFrame::OnUpdateRawIsActive)
EVT_UPDATE_UI(ID_RAW_SCALE,				MainFrame::OnUpdateRawIsActive)

EVT_MENU(ID_AREA_CLEAR,				MainFrame::OnAreaClear)
EVT_MENU(ID_AREA_ZOOM_ALL,			MainFrame::OnAreaZoomAll)
EVT_MENU(ID_AREA_ZOOM_LAYER,		MainFrame::OnAreaZoomLayer)
EVT_MENU(ID_AREA_TYPEIN,			MainFrame::OnAreaTypeIn)
EVT_MENU(ID_AREA_MATCH,				MainFrame::OnAreaMatch)
EVT_MENU(ID_AREA_EXPORT_ELEV,		MainFrame::OnAreaExportElev)
EVT_MENU(ID_AREA_EXPORT_IMAGE,		MainFrame::OnAreaExportImage)
EVT_MENU(ID_AREA_EXPORT_ELEV_SPARSE,MainFrame::OnAreaOptimizedElevTileset)
EVT_MENU(ID_AREA_EXPORT_IMAGE_OPT,	MainFrame::OnAreaOptimizedImageTileset)
EVT_MENU(ID_AREA_GENERATE_VEG,		MainFrame::OnAreaGenerateVeg)
EVT_MENU(ID_AREA_VEG_DENSITY,		MainFrame::OnAreaVegDensity)
EVT_MENU(ID_AREA_REQUEST_WFS,		MainFrame::OnAreaRequestWFS)
EVT_MENU(ID_AREA_REQUEST_WMS,		MainFrame::OnAreaRequestWMS)
EVT_MENU(ID_AREA_REQUEST_TSERVE,	MainFrame::OnAreaRequestTServe)

EVT_UPDATE_UI(ID_AREA_ZOOM_ALL,		MainFrame::OnUpdateAreaZoomAll)
EVT_UPDATE_UI(ID_AREA_ZOOM_LAYER,	MainFrame::OnUpdateAreaZoomLayer)
EVT_UPDATE_UI(ID_AREA_MATCH,		MainFrame::OnUpdateAreaMatch)
EVT_UPDATE_UI(ID_AREA_EXPORT_ELEV,	MainFrame::OnUpdateAreaExportElev)
EVT_UPDATE_UI(ID_AREA_EXPORT_ELEV_SPARSE,MainFrame::OnUpdateAreaExportElev)
EVT_UPDATE_UI(ID_AREA_EXPORT_IMAGE_OPT,MainFrame::OnUpdateAreaExportImage)
EVT_UPDATE_UI(ID_AREA_EXPORT_IMAGE,	MainFrame::OnUpdateAreaExportImage)
EVT_UPDATE_UI(ID_AREA_GENERATE_VEG,	MainFrame::OnUpdateAreaGenerateVeg)
EVT_UPDATE_UI(ID_AREA_VEG_DENSITY,	MainFrame::OnUpdateAreaVegDensity)
EVT_UPDATE_UI(ID_AREA_REQUEST_WFS,	MainFrame::OnUpdateAreaRequestWMS)
EVT_UPDATE_UI(ID_AREA_REQUEST_WMS,	MainFrame::OnUpdateAreaRequestWMS)

EVT_MENU(wxID_HELP,				MainFrame::OnHelpAbout)
EVT_MENU(ID_HELP_DOC_LOCAL,		MainFrame::OnHelpDocLocal)
EVT_MENU(ID_HELP_DOC_ONLINE,	MainFrame::OnHelpDocOnline)

// Popup menu items
EVT_MENU(ID_DISTANCE_CLEAR,		MainFrame::OnDistanceClear)
EVT_MENU(ID_POPUP_SHOWALL,		MainFrame::OnShowAll)
EVT_MENU(ID_POPUP_HIDEALL,		MainFrame::OnHideAll)
EVT_MENU(ID_POPUP_PROPS,		MainFrame::OnLayerPropsPopup)
EVT_MENU(ID_POPUP_TO_TOP,		MainFrame::OnLayerToTop)
EVT_MENU(ID_POPUP_TO_BOTTOM,	MainFrame::OnLayerToBottom)
EVT_MENU(ID_POPUP_OVR_DISK,		MainFrame::OnLayerOverviewDisk)
EVT_MENU(ID_POPUP_OVR_MEM,		MainFrame::OnLayerOverviewMem)

EVT_CHAR(MainFrame::OnChar)
EVT_KEY_DOWN(MainFrame::OnKeyDown)
EVT_MOUSEWHEEL(MainFrame::OnMouseWheel)
EVT_CLOSE(MainFrame::OnClose)

END_EVENT_TABLE()


void MainFrame::CreateMenus()
{
	int menu_num = 0;

	m_pMenuBar = new wxMenuBar;

	// Project menu
	fileMenu = new wxMenu;
	fileMenu->Append(ID_FILE_NEW, _("&New\tCtrl+N"), _("New Project"));
	fileMenu->Append(ID_FILE_OPEN, _("Open Project\tCtrl+O"), _("Open Project"));
	fileMenu->Append(ID_FILE_SAVE, _("Save Project\tCtrl+S"), _("Save Project As"));
	fileMenu->AppendSeparator();
	wxMenu *specialMenu = new wxMenu;
	specialMenu->Append(ID_SPECIAL_DYMAX_TEXTURES, _("Create Dymaxion Textures"));
	specialMenu->Append(ID_SPECIAL_DYMAX_MAP, _("Create Dymaxion Map"));
	specialMenu->Append(ID_SPECIAL_PROCESS_BILLBOARD, _("Process Billboard Texture"));
	specialMenu->Append(ID_SPECIAL_GEOCODE, _("Geocode"));
	specialMenu->Append(ID_ELEV_COPY, _("Copy Elevation Layer to Clipboard"));
	specialMenu->Append(ID_ELEV_PASTE_NEW, _("New Elevation Layer from Clipboard"));
	fileMenu->Append(0, _("Special"), specialMenu);
	fileMenu->AppendSeparator();
	fileMenu->Append(ID_FILE_PREFS, _("Preferences"));
	fileMenu->AppendSeparator();
	fileMenu->Append(ID_FILE_EXIT, _("E&xit\tAlt-X"), _("Exit"));
#ifdef __WXMAC__
	wxApp::s_macPreferencesMenuItemId = ID_FILE_PREFS;
	wxApp::s_macExitMenuItemId = ID_FILE_EXIT;
#endif
	m_pMenuBar->Append(fileMenu, _("&Project"));
	menu_num++;

	// Edit
	editMenu = new wxMenu;
	editMenu->Append(ID_EDIT_DELETE, _("Delete\tDEL"), _("Delete"));
	editMenu->AppendSeparator();
	editMenu->Append(ID_EDIT_DESELECTALL, _("Deselect All"), _("Clears selection"));
	editMenu->Append(ID_EDIT_INVERTSELECTION, _("Invert Selection"));
	editMenu->AppendCheckItem(ID_EDIT_CROSSINGSELECTION, _("Crossing Selection"));
	m_pMenuBar->Append(editMenu, _("&Edit"));
	menu_num++;

	// Layer
	layerMenu = new wxMenu;
	layerMenu->Append(ID_LAYER_NEW, _("&New Layer"), _("Create New Layer"));
	layerMenu->Append(ID_LAYER_OPEN, _("Open Layer"), _("Open Existing Layer"));
	layerMenu->Append(ID_LAYER_SAVE, _("Save Layer"), _("Save Active Layer"));
	layerMenu->Append(ID_LAYER_SAVE_AS, _("Save Layer As..."), _("Save Active Layer As"));
	layerMenu->Append(ID_LAYER_IMPORT, _("Import Data\tCtrl+I"), _("Import Data"));
	layerMenu->Append(ID_LAYER_IMPORTTIGER, _("Import Data From TIGER"), _("Import Data From TIGER"));
	layerMenu->Append(ID_LAYER_IMPORTNTF, _("Import Data From NTF"), _("Import Data From TIGER"));
	//layerMenu->Append(ID_LAYER_IMPORTUTIL, _("Import Utilities From SHP"), _("Import Utilities From SHP"));
	layerMenu->Append(ID_LAYER_IMPORT_MS, _("Import From MapSource File"));
	layerMenu->Append(ID_LAYER_IMPORT_POINT, _("Import Point Data From Table"));
	layerMenu->Append(ID_LAYER_IMPORT_XML, _("Import Point Data From XML"));
	layerMenu->Append(ID_LAYER_IMPORT_DXF, _("Import Raw Layers from DXF"));
	layerMenu->AppendSeparator();
	layerMenu->Append(ID_LAYER_PROPS, _("Layer Properties"), _("Layer Properties"));
	layerMenu->Append(ID_EDIT_OFFSET, _("Offset Coordinates"), _("Offset"));
	layerMenu->AppendSeparator();
	layerMenu->Append(ID_LAYER_FLATTEN, _("&Flatten Layers"), _("Flatten"));
	layerMenu->AppendSeparator();
	layerMenu->Append(ID_LAYER_CONVERTPROJ, _("Convert Projection"), _("Convert"));
	layerMenu->Append(ID_LAYER_SETPROJ, _("Set Projection"), _("Set Projection"));
	m_pMenuBar->Append(layerMenu, _("&Layer"));
	menu_num++;

	// View
	viewMenu = new wxMenu;
	viewMenu->AppendCheckItem(ID_VIEW_SHOWLAYER, _("Current Layer &Visible"),
		_("Toggle Visibility of the current Layer"));
	viewMenu->Append(ID_VIEW_LAYER_UP, _("Move Layer &Up"));
	viewMenu->Append(ID_VIEW_LAYER_DOWN, _("Move Layer &Down"));
	viewMenu->AppendSeparator();
	viewMenu->Append(ID_VIEW_ZOOMIN, _("Zoom &In\tCtrl++"));
	viewMenu->Append(ID_VIEW_ZOOMOUT, _("Zoom Out\tCtrl+-"));
	viewMenu->Append(ID_VIEW_ZOOMALL, _("Zoom &All"));
	viewMenu->Append(ID_VIEW_ZOOM_LAYER, _("Zoom to Current &Layer"));
	viewMenu->Append(ID_VIEW_FULLVIEW, _("Zoom to &Full Res (1:1)"));
	viewMenu->Append(ID_VIEW_ZOOM_AREA, _("Zoom to Area Tool"));
	viewMenu->AppendSeparator();
	viewMenu->AppendCheckItem(ID_VIEW_TOOLBAR, _("Toolbar"));
	viewMenu->AppendCheckItem(ID_VIEW_LAYERS, _("Layers"));
	viewMenu->AppendSeparator();
	viewMenu->AppendCheckItem(ID_VIEW_MAGNIFIER, _("&Magnifier\tZ"));
	viewMenu->AppendCheckItem(ID_VIEW_PAN, _("&Pan\tSPACE"));
	viewMenu->AppendCheckItem(ID_VIEW_DISTANCE, _("Obtain &Distance"));
	viewMenu->AppendCheckItem(ID_VIEW_SETAREA, _("Area &Tool"));
	viewMenu->AppendSeparator();
	viewMenu->AppendCheckItem(ID_VIEW_WORLDMAP, _("&World Map"), _("Show/Hide World Map"));
	viewMenu->AppendCheckItem(ID_VIEW_SHOWUTM, _("Show &UTM Boundaries"));
//	viewMenu->AppendCheckItem(ID_VIEW_SHOWGRID, _("Show 7.5\" Grid"), _("Show 7.5\" Grid"), true);
	viewMenu->AppendCheckItem(ID_VIEW_PROFILE, _("Elevation Profile"));
	viewMenu->AppendCheckItem(ID_VIEW_SCALE_BAR, _("Scale Bar"));
	viewMenu->AppendSeparator();
	viewMenu->Append(ID_VIEW_OPTIONS, _("&Options"));
	m_pMenuBar->Append(viewMenu, _("&View"));
	menu_num++;

	// Roads
	roadMenu = new wxMenu;
	roadMenu->AppendCheckItem(ID_ROAD_SELECTROAD, _("Select/Modify Roads"));
	roadMenu->AppendCheckItem(ID_ROAD_SELECTNODE, _("Select/Modify Nodes"));
	roadMenu->AppendCheckItem(ID_ROAD_SELECTWHOLE, _("Select Whole Roads"));
	roadMenu->AppendCheckItem(ID_ROAD_DIRECTION, _("Set Road Direction"));
	roadMenu->AppendCheckItem(ID_ROAD_EDIT, _("Edit Road Points"));
	roadMenu->AppendSeparator();
	roadMenu->AppendCheckItem(ID_ROAD_SHOWNODES, _("Show Nodes"));
	roadMenu->AppendCheckItem(ID_ROAD_SELECTHWY, _("Select by Highway Number"));
	roadMenu->AppendSeparator();
	roadMenu->Append(ID_ROAD_CLEAN, _("Clean RoadMap"), _("Clean"));
	roadMenu->Append(ID_ROAD_GUESS, _("Guess Intersection Types"));
	roadMenu->Append(ID_ROAD_FLATTEN, _("Flatten Elevation Grid Under Roads"));
	m_pMenuBar->Append(roadMenu, _("&Roads"));
	m_iLayerMenu[LT_ROAD] = menu_num;
	menu_num++;

	// Utilities
	utilityMenu = new wxMenu;
	utilityMenu->AppendCheckItem(ID_TOWER_ADD, _("Add a Transmission Tower"));
	utilityMenu->AppendSeparator();
	utilityMenu->AppendCheckItem(ID_TOWER_SELECT, _("Select Utility Layer"));
	utilityMenu->AppendCheckItem(ID_TOWER_EDIT, _("Edit Transmission Towers"));
	m_pMenuBar->Append(utilityMenu, _("Util&ities"));
	m_iLayerMenu[LT_UTILITY] = menu_num;
	menu_num++;

	// Elevation
	elevMenu = new wxMenu;
	elevMenu->AppendCheckItem(ID_ELEV_SELECT, _("Se&lect Elevation Layer"));
	elevMenu->AppendSeparator();
	elevMenu->Append(ID_ELEV_SCALE, _("Sc&ale Elevation"));
	elevMenu->Append(ID_ELEV_VERT_OFFSET, _("Offset Elevation Vertically"));
	elevMenu->Append(ID_ELEV_REMOVERANGE, _("&Remove Elevation Range..."));

	wxMenu *fillMenu = new wxMenu;
	fillMenu->Append(ID_ELEV_FILL_FAST, _("Fast"));
	fillMenu->Append(ID_ELEV_FILL_SLOW, _("Slow and smooth"));
	fillMenu->Append(ID_ELEV_FILL_REGIONS, _("Extrapolation via partial derivatives"));

	elevMenu->Append(0, _("&Fill In Unknown Areas"), fillMenu);

	elevMenu->Append(ID_ELEV_SETUNKNOWN, _("&Set Unknown Areas"));
	elevMenu->AppendSeparator();
	elevMenu->Append(ID_ELEV_EXPORT, _("E&xport To..."));
	elevMenu->Append(ID_ELEV_EXPORT_TILES, _("Export to Tiles..."));
	elevMenu->Append(ID_ELEV_BITMAP, _("Re&nder to Bitmap..."));
	elevMenu->Append(ID_ELEV_TOTIN, _("Convert Grid to TIN"));
	elevMenu->AppendSeparator();
	elevMenu->Append(ID_ELEV_MERGETIN, _("&Merge shared TIN vertices"));
	elevMenu->AppendCheckItem(ID_ELEV_TRIMTIN, _("Trim TIN triangles by line segment"));
	m_pMenuBar->Append(elevMenu, _("Elev&ation"));
	m_iLayerMenu[LT_ELEVATION] = menu_num;
	menu_num++;

	// Imagery
	imgMenu = new wxMenu;
	imgMenu->Append(ID_IMAGE_REPLACE_RGB, _("Replace RGB..."));
	imgMenu->Append(ID_IMAGE_CREATE_OVERVIEWS, _("Create Overviews on Disk"));
	imgMenu->Append(ID_IMAGE_CREATE_OVER_ALL, _("Create Overviews on Disk for All Images"));
	imgMenu->Append(ID_IMAGE_CREATE_MIPMAPS, _("Create Overviews in Memory"));
	imgMenu->AppendSeparator();
	imgMenu->Append(ID_IMAGE_EXPORT_TILES, _("Export to Tiles..."));
	imgMenu->Append(ID_IMAGE_EXPORT_PPM, _("Export to PPM"));
	m_pMenuBar->Append(imgMenu, _("Imagery"));
	m_iLayerMenu[LT_IMAGE] = menu_num;
	menu_num++;

	// Vegetation
	vegMenu = new wxMenu;
	vegMenu->Append(ID_VEG_PLANTS, _("Species List"), _("View/Edit list of available plant species"));
	vegMenu->Append(ID_VEG_BIOREGIONS, _("BioRegions"), _("View/Edit list of species & density for each BioRegion"));
	vegMenu->AppendSeparator();
	vegMenu->Append(ID_VEG_REMAP, _("Remap Species"));
	vegMenu->Append(ID_VEG_EXPORTSHP, _("Export SHP"));
	m_pMenuBar->Append(vegMenu, _("Veg&etation"));
	m_iLayerMenu[LT_VEG] = menu_num;
	menu_num++;

	// Structures
	bldMenu = new wxMenu;
	bldMenu->AppendCheckItem(ID_FEATURE_SELECT, _("Select Features"));
	bldMenu->AppendCheckItem(ID_STRUCTURE_EDIT_BLD, _("Edit Buildings"));
	bldMenu->AppendCheckItem(ID_STRUCTURE_ADD_POINTS, _("Add points to building footprints"), _T(""));
	bldMenu->AppendCheckItem(ID_STRUCTURE_DELETE_POINTS, _("Delete points from building footprints"), _T(""));
	bldMenu->AppendCheckItem(ID_STRUCTURE_ADD_LINEAR, _("Add Linear Structures"));
	bldMenu->AppendCheckItem(ID_STRUCTURE_EDIT_LINEAR, _("Edit Linear Structures"));
	bldMenu->AppendCheckItem(ID_STRUCTURE_ADD_INST, _("Add Instances"));
	bldMenu->AppendSeparator();
	bldMenu->Append(ID_STRUCTURE_ADD_FOUNDATION, _("Add Foundation Levels to Buildings"), _T(""));
	bldMenu->Append(ID_STRUCTURE_SELECT_USING_POLYGONS, _("Select Using Polygons"), _("Select buildings using selected raw layer polygons"));
	bldMenu->Append(ID_STRUCTURE_COLOUR_SELECTED_ROOFS, _("Colour Selected Roofs"), _("Set roof colour on selected buildings"));
	bldMenu->Append(ID_STRUCTURE_CLEAN_FOOTPRINTS, _("Clean Footprints"), _("Clean up degenerate footprint geometry"));
	bldMenu->AppendSeparator();
	bldMenu->Append(ID_STRUCTURE_EXPORT_FOOTPRINTS, _("Export footprints to SHP"));
	bldMenu->Append(ID_STRUCTURE_EXPORT_CANOMA, _("Export footprints to Canoma3DV"));

	bldMenu->AppendSeparator();
	bldMenu->AppendCheckItem(ID_STRUCTURE_CONSTRAIN, _("Constrain angles on footprint edit"));
	m_pMenuBar->Append(bldMenu, _("&Structures"));
	m_iLayerMenu[LT_STRUCTURE] = menu_num;
	menu_num++;

	// Raw
	rawMenu = new wxMenu;
	rawMenu->AppendCheckItem(ID_FEATURE_SELECT, _("Select Features"));
	rawMenu->AppendCheckItem(ID_FEATURE_PICK, _("Pick Features"));
	rawMenu->AppendCheckItem(ID_FEATURE_TABLE, _("Show Attribute Table"));
	rawMenu->AppendSeparator();
	rawMenu->Append(ID_RAW_SETTYPE, _("Set Entity Type"), _("Set Entity Type"));
	rawMenu->AppendCheckItem(ID_RAW_ADDPOINTS, _("Add Points with Mouse"));
	rawMenu->Append(ID_RAW_ADDPOINT_TEXT, _("Add Point with Text\tCtrl+T"), _("Add point"));
	rawMenu->Append(ID_RAW_ADDPOINTS_GPS, _("Add Points with GPS"), _("Add points with GPS"));
	rawMenu->Append(ID_RAW_STYLE, _("Style..."));
	rawMenu->Append(ID_RAW_SCALE, _("Scale"));
	rawMenu->AppendSeparator();
	rawMenu->Append(ID_RAW_SELECTCONDITION, _("Select Features by Condition"));
	rawMenu->Append(ID_RAW_CONVERT_TOTIN, _("Convert 3D points to TIN"));
	rawMenu->Append(ID_RAW_EXPORT_IMAGEMAP, _("Export as HTML ImageMap"));
	rawMenu->Append(ID_RAW_EXPORT_KML, _("Export as KML"));
	rawMenu->Append(ID_RAW_GENERATE_ELEVATION, _("Generate Grid from 3D Points"));
	m_pMenuBar->Append(rawMenu, _("Ra&w"));
	m_iLayerMenu[LT_RAW] = menu_num;
	menu_num++;

	// Area
	areaMenu = new wxMenu;
	areaMenu->Append(ID_AREA_CLEAR, _("Clear (Set to zero)"));
	areaMenu->Append(ID_AREA_ZOOM_ALL, _("Set to Full Extents"),
		_("Set the Area Tool rectangle to the combined extent of all layers."));
	areaMenu->Append(ID_AREA_ZOOM_LAYER, _("Set to Layer Extents"),
		_("Set the Area Tool rectangle to the extent of the active layer."));
	areaMenu->Append(ID_AREA_TYPEIN, _("Numeric Values"),
		_("Set the Area Tool rectangle by text entry of coordinates."));
	areaMenu->Append(ID_AREA_MATCH, _("Match Area and Tiling to Layer"),
		_("Set the Area Tool rectangle by matching the resolution of a layer."));
	areaMenu->AppendSeparator();
	areaMenu->Append(ID_AREA_EXPORT_ELEV, _("Merge && Resample &Elevation"),
		_("Sample all elevation data within the Area Tool to produce a single, new elevation."));
	areaMenu->Append(ID_AREA_EXPORT_IMAGE, _("Merge && Resample &Imagery"),
		_("Sample imagery within the Area Tool to produce a single, new image."));
	areaMenu->Append(ID_AREA_GENERATE_VEG, _("Generate Vegetation"),
		_("Generate Vegetation File (*.vf) containing plant distribution."));
	areaMenu->Append(ID_AREA_VEG_DENSITY, _("Compute Vegetation Density"),
		_("Compute and display the density of each species of vegetation in the given area."));
#if SUPPORT_CURL
	areaMenu->Append(ID_AREA_REQUEST_WFS, _("Request Layer from WFS"));
	areaMenu->Append(ID_AREA_REQUEST_WMS, _("Request Image from WMS"));
	areaMenu->Append(ID_AREA_REQUEST_TSERVE, _("Request Image from Terraserver"));
#endif // SUPPORT_CURL
	areaMenu->AppendSeparator();
	areaMenu->Append(ID_AREA_EXPORT_ELEV_SPARSE, _("Optimized Resample Elevation to Tileset"),
		_("Sample all elevation data within the Area Tool efficiently to produce an elevation tileset."));
	areaMenu->Append(ID_AREA_EXPORT_IMAGE_OPT, _("Optimized Resample Imagery to Tileset"),
		_("Sample all image data within the Area Tool efficiently to produce an image tileset."));
	m_pMenuBar->Append(areaMenu, _("&Area Tool"));
	menu_num++;

	// Help
	helpMenu = new wxMenu;
	wxString msg = _("About ");
	msg += wxString(APPNAME, wxConvUTF8);
#ifdef __WXMAC__
#endif
	helpMenu->Append(wxID_HELP, _("&About"), msg);
	helpMenu->Append(ID_HELP_DOC_LOCAL, _("Documentation (local)"), msg);
	helpMenu->Append(ID_HELP_DOC_ONLINE, _("Documentation (on the web)"), msg);
	m_pMenuBar->Append(helpMenu, _("&Help"));
#ifdef __WXMAC__
	wxApp::s_macAboutMenuItemId = wxID_HELP;
	wxApp::s_macHelpMenuTitleName = _("&Help");
#endif
	menu_num++;

	SetMenuBar(m_pMenuBar);
}

////////////////////////////////////////////////////////////////
// Project menu

void MainFrame::OnProjectNew(wxCommandEvent &event)
{
	SetActiveLayer(NULL);
	DeleteContents();
	m_area.SetRect(0.0, 0.0, 0.0, 0.0);
	m_pView->Refresh();
	Refresh();

	// reset veg too
	m_strSpeciesFilename = "";
	m_strBiotypesFilename = "";
	m_PlantList.Clear();
	m_BioRegion.Clear();

	RefreshTreeView();
	RefreshToolbars();

	vtProjection p;
	SetProjection(p);
}

wxString GetProjectFilter()
{
	wxString str(APPNAME, wxConvUTF8);
	str += _T(" ");
	str += _("Project Files (*.vtb)|*.vtb");
	return str;
}

void MainFrame::OnProjectOpen(wxCommandEvent &event)
{
	wxFileDialog loadFile(NULL, _("Load Project"), _T(""), _T(""),
		GetProjectFilter(), wxFD_OPEN);
	bool bResult = (loadFile.ShowModal() == wxID_OK);
	if (!bResult)
		return;
	LoadProject(loadFile.GetPath());
}

void MainFrame::OnProjectSave(wxCommandEvent &event)
{
	wxFileDialog saveFile(NULL, _("Save Project"), _T(""), _T(""),
		GetProjectFilter(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
	bool bResult = (saveFile.ShowModal() == wxID_OK);
	if (!bResult)
		return;
	wxString strPathName = saveFile.GetPath();

	SaveProject(strPathName);
}

void MainFrame::OnProjectPrefs(wxCommandEvent &event)
{
	PrefDlg dlg(this, wxID_ANY, _("Preferences"));
	dlg.b1 = (g_Options.GetValueBool(TAG_USE_CURRENT_CRS) == true);
	dlg.b2 = (g_Options.GetValueBool(TAG_USE_CURRENT_CRS) == false);

	dlg.b3 = g_Options.GetValueBool(TAG_LOAD_IMAGES_ALWAYS);
	dlg.b4 = g_Options.GetValueBool(TAG_LOAD_IMAGES_NEVER);
	dlg.b5 = (!g_Options.GetValueBool(TAG_LOAD_IMAGES_ALWAYS) &&
			  !g_Options.GetValueBool(TAG_LOAD_IMAGES_NEVER));

	dlg.b6 = g_Options.GetValueBool(TAG_REPRO_TO_FLOAT_NEVER);
	dlg.b7 = g_Options.GetValueBool(TAG_REPRO_TO_FLOAT_ALWAYS);
	dlg.b8 = (!g_Options.GetValueBool(TAG_REPRO_TO_FLOAT_ALWAYS) &&
			  !g_Options.GetValueBool(TAG_REPRO_TO_FLOAT_NEVER));

	dlg.b9 =  (g_Options.GetValueInt(TAG_GAP_FILL_METHOD) == 1);
	dlg.b10 = (g_Options.GetValueInt(TAG_GAP_FILL_METHOD) == 2);
	dlg.b11 = (g_Options.GetValueInt(TAG_GAP_FILL_METHOD) == 3);

	dlg.b12 = g_Options.GetValueBool(TAG_BLACK_TRANSP);
	dlg.b13 = g_Options.GetValueBool(TAG_TIFF_COMPRESS);
	dlg.b14 = g_Options.GetValueBool(TAG_DEFAULT_GZIP_BT);
	dlg.b15 = g_Options.GetValueBool(TAG_DELAY_LOAD_GRID);
	dlg.i1 =  g_Options.GetValueInt(TAG_SAMPLING_N);
	dlg.i2 =  g_Options.GetValueInt(TAG_MAX_MEGAPIXELS);
	dlg.i3 =  g_Options.GetValueInt(TAG_ELEV_MAX_SIZE);
	if (dlg.b15)
		dlg.i4 =  g_Options.GetValueInt(TAG_MAX_MEM_GRID);
	else
		dlg.i4 =  128;

	dlg.TransferDataToWindow();

	if (dlg.ShowModal() == wxID_OK)
	{
		g_Options.SetValueBool(TAG_USE_CURRENT_CRS, dlg.b1);
		g_Options.SetValueBool(TAG_LOAD_IMAGES_ALWAYS, dlg.b3);
		g_Options.SetValueBool(TAG_LOAD_IMAGES_NEVER, dlg.b4);
		g_Options.SetValueBool(TAG_REPRO_TO_FLOAT_ALWAYS, dlg.b7);
		g_Options.SetValueBool(TAG_REPRO_TO_FLOAT_NEVER, dlg.b6);

		if (dlg.b9)  g_Options.SetValueInt(TAG_GAP_FILL_METHOD, 1);
		if (dlg.b10) g_Options.SetValueInt(TAG_GAP_FILL_METHOD, 2);
		if (dlg.b11) g_Options.SetValueInt(TAG_GAP_FILL_METHOD, 3);

		g_Options.SetValueBool(TAG_BLACK_TRANSP, dlg.b12);
		g_Options.SetValueBool(TAG_TIFF_COMPRESS, dlg.b13);
		g_Options.SetValueBool(TAG_DEFAULT_GZIP_BT, dlg.b14);
		g_Options.SetValueBool(TAG_DELAY_LOAD_GRID, dlg.b15);
		g_Options.SetValueInt(TAG_SAMPLING_N, dlg.i1);
		g_Options.SetValueInt(TAG_MAX_MEGAPIXELS, dlg.i2);
		g_Options.SetValueInt(TAG_ELEV_MAX_SIZE, dlg.i3);
		g_Options.SetValueInt(TAG_MAX_MEM_GRID, dlg.i4);

		vtImage::bTreatBlackAsTransparent = dlg.b11;
		vtElevLayer::m_bDefaultGZip = dlg.b13;
		if (dlg.b15)
			vtElevLayer::m_iGridMemLimit = dlg.i4;
		else
			vtElevLayer::m_iGridMemLimit = -1;

		// safety checks
		CheckOptionBounds();
	}
}

void MainFrame::OnDymaxTexture(wxCommandEvent &event)
{
	DoDymaxTexture();
}

void MainFrame::OnDymaxMap(wxCommandEvent &event)
{
	DoDymaxMap();
}

void MainFrame::OnProcessBillboard(wxCommandEvent &event)
{
	DoProcessBillboard();
}

void MainFrame::OnGeocode(wxCommandEvent &event)
{
	DoGeocode();
}

void MainFrame::OnQuit(wxCommandEvent &event)
{
	Close(FALSE);
}

//////////////////////////////////////////////////
// Edit menu

void MainFrame::OnEditDelete(wxCommandEvent &event)
{
	vtRoadLayer *pRL = GetActiveRoadLayer();
	if (pRL && (pRL->NumSelectedNodes() != 0 || pRL->NumSelectedLinks() != 0))
	{
		wxString str;
		str.Printf(_("Deleting road selection: %d nodes and %d roads"),
			pRL->NumSelectedNodes(), pRL->NumSelectedLinks());
		SetStatusText(str);
		m_pView->DeleteSelected(pRL);
		pRL->SetModified(true);
		return;
	}
	vtStructureLayer *pSL = GetActiveStructureLayer();
	if (pSL && pSL->NumSelected() != 0)
	{
		pSL->DeleteSelected();
		pSL->SetModified(true);
		m_pView->Refresh();
		return;
	}
	vtRawLayer *pRawL = GetActiveRawLayer();
	if (pRawL)
	{
		vtFeatureSet *pSet = pRawL->GetFeatureSet();
		if (pSet && pSet->NumSelected() != 0)
		{
			pSet->DeleteSelected();
			pRawL->SetModified(true);
			m_pView->Refresh();
			OnSelectionChanged();
			return;
		}
	}

	vtLayer *pL = GetActiveLayer();
	if (pL)
	{
		int result = wxMessageBox(_("Are you sure you want to delete the current layer?"),
				_("Question"), wxYES_NO | wxICON_QUESTION, this);
		if (result == wxYES)
			RemoveLayer(pL);
	}
}

void MainFrame::OnUpdateEditDelete(wxUpdateUIEvent& event)
{
	event.Enable(GetActiveLayer() != NULL);
}

void MainFrame::OnEditDeselectAll(wxCommandEvent &event)
{
	m_pView->DeselectAll();
}

void MainFrame::OnEditInvertSelection(wxCommandEvent &event)
{
	vtRoadLayer *pRL = GetActiveRoadLayer();
	if (pRL) {
		pRL->InvertSelection();
		m_pView->Refresh(false);
	}
	vtStructureLayer *pSL = GetActiveStructureLayer();
	if (pSL) {
		pSL->InvertSelection();
		m_pView->Refresh(false);
	}
	vtRawLayer *pRawL = GetActiveRawLayer();
	if (pRawL) {
		pRawL->GetFeatureSet()->InvertSelection();
		m_pView->Refresh(false);
		OnSelectionChanged();
	}
}

void MainFrame::OnEditCrossingSelection(wxCommandEvent &event)
{
	m_pView->m_bCrossSelect = !m_pView->m_bCrossSelect;
}

void MainFrame::OnUpdateCrossingSelection(wxUpdateUIEvent& event)
{
	event.Check(m_pView->m_bCrossSelect);
}

void MainFrame::OnEditOffset(wxCommandEvent &event)
{
	wxTextEntryDialog dlg(this, _("Offset"),
		_("Please enter horizontal offset X, Y"), _T("0, 0"));
	if (dlg.ShowModal() != wxID_OK)
		return;

	DPoint2 offset;
	wxString str = dlg.GetValue();
	sscanf(str.mb_str(wxConvUTF8), "%lf, %lf", &offset.x, &offset.y);

	GetActiveLayer()->Offset(offset);
	GetActiveLayer()->SetModified(true);
	m_pView->Refresh();
}

void MainFrame::OnUpdateEditOffset(wxUpdateUIEvent& event)
{
	event.Enable(GetActiveLayer() != NULL);
}


//////////////////////////////////////////////////
// Layer menu

void MainFrame::OnLayerNew(wxCommandEvent &event)
{
	LayerType lt = AskLayerType();
	if (lt == LT_UNKNOWN)
		return;

	vtLayer *pL = vtLayer::CreateNewLayer(lt);
	if (!pL)
		return;

	if (lt == LT_ELEVATION)
	{
		vtElevLayer *pEL = (vtElevLayer *)pL;
		pEL->m_pGrid = new vtElevationGrid(m_area, 1025, 1025, false, m_proj);
		pEL->m_pGrid->FillWithSingleValue(1000);
	}
	else
	{
		pL->SetProjection(m_proj);
	}

	SetActiveLayer(pL);
	m_pView->SetActiveLayer(pL);
	AddLayer(pL);
	RefreshTreeView();
	RefreshToolbars();
	RefreshLayerInView(pL);
}

void MainFrame::OnLayerOpen(wxCommandEvent &event)
{
	wxString filter = _("Native Layer Formats|");

	AddType(filter, FSTRING_BT);	// elevation
	AddType(filter, FSTRING_BTGZ);	// compressed elevation
	AddType(filter, FSTRING_TIN);	// elevation
	AddType(filter, FSTRING_RMF);	// roads
	AddType(filter, FSTRING_GML);	// raw
	AddType(filter, FSTRING_UTL);	// utility towers
	AddType(filter, FSTRING_VTST);	// structures
	AddType(filter, FSTRING_VTSTGZ);// compressed structures
	AddType(filter, FSTRING_VF);	// vegetation files
	AddType(filter, FSTRING_TIF);	// image files
	AddType(filter, FSTRING_IMG);	// image or elevation file
	AddType(filter, FSTRING_SHP);	// raw files

	// ask the user for a filename, allow multiple select
	wxFileDialog loadFile(NULL, _("Open Layer"), _T(""), _T(""), filter,
		wxFD_OPEN | wxFD_MULTIPLE);
	bool bResult = (loadFile.ShowModal() == wxID_OK);
	if (!bResult)
		return;

	wxArrayString Paths;
	loadFile.GetPaths(Paths);

	for (size_t i = 0; i < Paths.GetCount(); i++)
		LoadLayer(Paths[i]);
}

void MainFrame::OnLayerSave(wxCommandEvent &event)
{
	vtLayer *lp = GetActiveLayer();
	if (lp->GetLayerFilename().Left(8).CmpNoCase(_("Untitled")) == 0)
	{
		if (!lp->AskForSaveFilename())
			return;
	}
	wxString msg = _("Saving layer to file ") + lp->GetLayerFilename();
	SetStatusText(msg);
	VTLOG(msg.mb_str(wxConvUTF8));
	VTLOG("\n");

	if (lp->Save())
		msg = _("Saved layer to file ") + lp->GetLayerFilename();
	else
		msg = _("Save failed.");
	SetStatusText(msg);
	VTLOG(msg.mb_str(wxConvUTF8));
	VTLOG("\n");
}

void MainFrame::OnUpdateLayerSave(wxUpdateUIEvent& event)
{
	vtLayer *lp = GetActiveLayer();
	event.Enable(lp != NULL && lp->GetModified() && lp->CanBeSaved());
}

void MainFrame::OnLayerSaveAs(wxCommandEvent &event)
{
	vtLayer *lp = GetActiveLayer();

	if (!lp->AskForSaveFilename())
		return;

	wxString msg = _("Saving layer to file as ") + lp->GetLayerFilename();
	SetStatusText(msg);

	VTLOG1(msg.mb_str(wxConvUTF8));
	VTLOG1("\n");

	bool success = lp->Save();
	if (success)
	{
		lp->SetModified(false);
		msg = _("Saved layer to file as ") + lp->GetLayerFilename();
	}
	else
	{
		msg = _("Failed to save layer to ") + lp->GetLayerFilename();
		wxMessageBox(msg, _("Problem"));
	}
	SetStatusText(msg);

	VTLOG1(msg.mb_str(wxConvUTF8));
	VTLOG1("\n");
}

void MainFrame::OnUpdateLayerSaveAs(wxUpdateUIEvent& event)
{
	vtLayer *lp = GetActiveLayer();
	event.Enable(lp != NULL && lp->CanBeSaved());
}

void MainFrame::OnUpdateLayerProperties(wxUpdateUIEvent& event)
{
	event.Enable(GetActiveLayer() != NULL);
}

void MainFrame::OnLayerImport(wxCommandEvent &event)
{
	LayerType lt;

	// first ask what kind of data layer
	lt = AskLayerType();
	if (lt == LT_UNKNOWN)
		return;

	ImportData(lt);
}

void MainFrame::OnLayerImportTIGER(wxCommandEvent &event)
{
	// ask the user for a directory
	wxDirDialog getDir(NULL, _("Import TIGER Data From Directory"));
	bool bResult = (getDir.ShowModal() == wxID_OK);
	if (!bResult)
		return;
	wxString strDirName = getDir.GetPath();

	ImportDataFromTIGER(strDirName);
}

void MainFrame::OnLayerImportNTF(wxCommandEvent &event)
{
	// Use file dialog to open plant list text file.
	wxFileDialog loadFile(NULL, _("Import Layers from NTF File"), _T(""), _T(""),
		_("NTF Files (*.ntf)|*.ntf"), wxFD_OPEN);

	if (loadFile.ShowModal() != wxID_OK)
		return;

	wxString str = loadFile.GetPath();
	ImportDataFromNTF(str);
}

void MainFrame::OnLayerImportUtil(wxCommandEvent &event)
{
	// ask the user for a directory
	wxDirDialog getDir(NULL, _("Import Utility Data from Directory of SHP Files"));
	bool bResult = (getDir.ShowModal() == wxID_OK);
	if (!bResult)
		return;
	wxString strDirName = getDir.GetPath();

//	dlg.m_strCaption = _T("Shapefiles do not contain projection information.  ")
//		_T("Please indicate the projection of this file:");
	// ask user for a projection
	ProjectionDlg dlg(NULL, -1, _("Indicate Projection"));
	dlg.SetProjection(m_proj);

	if (dlg.ShowModal() == wxID_CANCEL)
		return;
	vtProjection proj;
	dlg.GetProjection(proj);

	// create the new layers
	vtUtilityLayer *pUL = new vtUtilityLayer;
	if (pUL->ImportFromSHP(strDirName.mb_str(wxConvUTF8), proj))
	{
		pUL->SetLayerFilename(strDirName);
		pUL->SetModified(true);

		if (!AddLayerWithCheck(pUL, true))
			delete pUL;
	}
	else
		delete pUL;
}

//
// Import from a Garmin MapSource GPS export file (.txt)
//
void MainFrame::OnLayerImportMapSource(wxCommandEvent &event)
{
	wxFileDialog loadFile(NULL, _("Import MapSource File"), _T(""), _T(""),
		_("MapSource Export Files (*.txt)|*.txt"), wxFD_OPEN);

	if (loadFile.ShowModal() != wxID_OK)
		return;

	wxString str = loadFile.GetPath();
	ImportFromMapSource(str.mb_str(wxConvUTF8));
}

void MainFrame::OnLayerImportPoint(wxCommandEvent &event)
{
	wxString filter = _("Tabular Data Files|");

	AddType(filter, FSTRING_DBF);	// old-style database
	AddType(filter, FSTRING_CSV);	// comma-separated values
	AddType(filter, FSTRING_XYZ);	// space-separated X Y Z

	wxFileDialog loadFile(NULL, _("Import Point Data"), _T(""), _T(""),
		filter, wxFD_OPEN);

	if (loadFile.ShowModal() != wxID_OK)
		return;

	OpenProgressDialog(_T("Importing"));

	wxString str = loadFile.GetPath();
	ImportDataPointsFromTable(str.mb_str(wxConvUTF8), progress_callback);

	CloseProgressDialog();
}

void MainFrame::OnLayerImportXML(wxCommandEvent &event)
{
	wxFileDialog loadFile(NULL, _("Import XML Data"), _T(""), _T(""),
		_("XML files (*.xml)|*.xml"), wxFD_OPEN);

	if (loadFile.ShowModal() != wxID_OK)
		return;

	wxString str = loadFile.GetPath();
	vtRawLayer *pRL = new vtRawLayer;
	if (pRL->ImportFromXML(str.mb_str(wxConvUTF8)))
	{
		pRL->SetLayerFilename(str);
		pRL->SetModified(true);

		if (!AddLayerWithCheck(pRL, true))
			delete pRL;
	}
	else
		delete pRL;
}

void MainFrame::OnLayerImportDXF(wxCommandEvent &event)
{
	wxFileDialog loadFile(NULL, _("Import DXF Data"), _T(""), _T(""),
		_("DXF files (*.dxf)|*.dxf"), wxFD_OPEN);

	if (loadFile.ShowModal() != wxID_OK)
		return;

	wxString str = loadFile.GetPath();
	ImportDataFromDXF(str.mb_str(wxConvUTF8));
}

void MainFrame::OnLayerProperties(wxCommandEvent &event)
{
	vtLayer *lp = GetActiveLayer();
	if (lp)
		ShowLayerProperties(lp);
}

void MainFrame::ShowLayerProperties(vtLayer *lp)
{
	// All layers have some common properties, others are specific to the
	//  type of layer.
	LayerType ltype = lp->GetType();

	wxString title;
	title += vtLayer::LayerTypeNames[ltype];
	title += _(" Layer Properties");
	LayerPropDlg dlg(NULL, -1, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

	// Fill in initial values for the dialog
	DRECT rect, rect2;
	lp->GetExtent(rect);
	dlg.m_fLeft = rect.left;
	dlg.m_fTop = rect.top;
	dlg.m_fRight = rect.right;
	dlg.m_fBottom = rect.bottom;

	lp->GetPropertyText(dlg.m_strText);

	// For elevation and image layers, if the user changes the extents, apply.
	if (dlg.ShowModal() != wxID_OK)
		return;

	rect2.left = dlg.m_fLeft;
	rect2.top = dlg.m_fTop;
	rect2.right = dlg.m_fRight;
	rect2.bottom = dlg.m_fBottom;
	if (rect2 != rect)
	{
		// user changed the extents
		if (lp->SetExtent(rect2))
		{
			wxMessageBox(_("Changed extents."));
			m_pView->Refresh();
		}
		else
			wxMessageBox(_("Could not change extents."));
	}
}

void MainFrame::OnAreaExportElev(wxCommandEvent &event)
{
	MergeResampleElevation(m_pView);
}

void MainFrame::OnAreaOptimizedElevTileset(wxCommandEvent &event)
{
	ExportAreaOptimizedElevTileset(m_pView);
}

void MainFrame::OnAreaOptimizedImageTileset(wxCommandEvent &event)
{
	ExportAreaOptimizedImageTileset(m_pView);
}

void MainFrame::OnUpdateAreaExportElev(wxUpdateUIEvent& event)
{
	event.Enable(LayersOfType(LT_ELEVATION) > 0 && !m_area.IsEmpty());
}

void MainFrame::OnAreaExportImage(wxCommandEvent &event)
{
	MergeResampleImages(m_pView);
}

void MainFrame::OnUpdateAreaExportImage(wxUpdateUIEvent& event)
{
	event.Enable(LayersOfType(LT_IMAGE) > 0 && !m_area.IsEmpty());
}

void MainFrame::OnLayerConvert(wxCommandEvent &event)
{
	// ask for what projection to convert to
	ProjectionDlg dlg(NULL, 200, _("Convert to what projection?"));
	dlg.SetProjection(m_proj);

	// might switch to utm, help provide a good guess for UTM zone
	DPoint2 pos = EstimateGeoDataCenter();
	dlg.SetGeoRefPoint(pos);

	if (dlg.ShowModal() == wxID_CANCEL)
		return;
	vtProjection proj;
	dlg.GetProjection(proj);

	// count through the layer array, converting
	int layers = m_Layers.GetSize();
	int succeeded = 0;
	for (int i = 0; i < layers; i++)
	{
		vtLayer *lp = m_Layers.GetAt(i);

		OpenProgressDialog(_("Reprojecting"), false, this);
		bool success = lp->TransformCoords(proj);
		CloseProgressDialog();

		if (success)
			succeeded++;
	}
	if (succeeded < layers)
	{
		if (layers == 1)
			DisplayAndLog("Failed to convert.");
		else
			DisplayAndLog("Failed to convert %d of %d layers.",
				layers-succeeded, layers);
	}

	SetProjection(proj);
	ZoomAll();
	RefreshStatusBar();
}

void MainFrame::OnLayerSetProjection(wxCommandEvent &event)
{
	// Allow the user to directly specify the projection for all loaded
	// layers (override it, without reprojecting the layer's data)
	// ask for what projection to convert to
	ProjectionDlg dlg(NULL, -1, _("Set to what projection?"));
	dlg.SetProjection(m_proj);

	// might switch to utm, help provide a good guess for UTM zone
	DPoint2 pos = EstimateGeoDataCenter();
	dlg.SetGeoRefPoint(pos);

	if (dlg.ShowModal() == wxID_CANCEL)
		return;
	vtProjection proj;
	dlg.GetProjection(proj);

	// count through the layer array, converting
	int layers = m_Layers.GetSize();
	for (int i = 0; i < layers; i++)
	{
		vtLayer *lp = m_Layers.GetAt(i);
		lp->SetProjection(proj);
	}

	SetProjection(proj);
	ZoomAll();
	RefreshStatusBar();
}

void MainFrame::OnUpdateLayerConvert(wxUpdateUIEvent& event)
{
	event.Enable(m_Layers.GetSize() != 0);
}

void MainFrame::OnLayerFlatten(wxCommandEvent &event)
{
	vtLayer *pActive = GetActiveLayer();
	LayerType t = pActive->GetType();

	int layers_merged = 0;

	// count down through the layer array, flattening
	int layers = m_Layers.GetSize();
	for (int i = layers-1; i >= 0; i--)
	{
		vtLayer *pL = m_Layers.GetAt(i);
		if (pL == pActive) continue;
		if (pL->GetType() != t) continue;

//		TRACE("Merging layer %s/%x with %s/%x\n",
//			pL->GetFilename(), pL, pActive->GetFilename(), pActive);
		if (pActive->AppendDataFrom(pL))
		{
			// successfully merged contents, so second layer can be deleted
			RemoveLayer(pL);
			layers_merged++;
		}
	}

	if (layers_merged > 0)
	{
		wxString newname = _("Untitled");
		newname += pActive->GetFileExtension();
		pActive->SetLayerFilename(newname);
		pActive->SetModified(true);
	}
}

void MainFrame::OnUpdateLayerFlatten(wxUpdateUIEvent& event)
{
	vtLayer *lp = GetActiveLayer();
	event.Enable(lp &&
			(lp->GetType() == LT_ROAD ||
			 lp->GetType() == LT_VEG ||
			 lp->GetType() == LT_WATER ||
			 lp->GetType() == LT_STRUCTURE ||
			 lp->GetType() == LT_RAW));
}


////////////////////////////////////////////////////////////
// View menu

void MainFrame::OnLayerShow(wxCommandEvent &event)
{
	vtLayer *pLayer = GetActiveLayer();
	if (!pLayer)
		return;
	pLayer->SetVisible(!pLayer->GetVisible());
	RefreshLayerInView(pLayer);
	RefreshTreeStatus();
}

void MainFrame::OnUpdateLayerShow(wxUpdateUIEvent& event)
{
	vtLayer *pLayer = GetActiveLayer();

	event.Enable(pLayer != NULL);
	event.Check(pLayer && pLayer->GetVisible());
}

void MainFrame::OnLayerUp(wxCommandEvent &event)
{
	vtLayer *pLayer = GetActiveLayer();
	if (!pLayer)
		return;
	int num = LayerNum(pLayer);
	if (num < NumLayers()-1)
		SwapLayerOrder(num, num+1);

	RefreshLayerInView(pLayer);
	RefreshTreeView();
}

void MainFrame::OnUpdateLayerUp(wxUpdateUIEvent& event)
{
	vtLayer *pLayer = GetActiveLayer();
	event.Enable(pLayer != NULL && LayerNum(pLayer) < NumLayers()-1);
}

void MainFrame::OnLayerDown(wxCommandEvent &event)
{
	vtLayer *pLayer = GetActiveLayer();
	if (!pLayer)
		return;
	int num = LayerNum(pLayer);
	if (num > 0)
		SwapLayerOrder(num-1, num);

	RefreshLayerInView(pLayer);
	RefreshTreeView();
}

void MainFrame::OnUpdateLayerDown(wxUpdateUIEvent& event)
{
	vtLayer *pLayer = GetActiveLayer();
	event.Enable(pLayer != NULL && LayerNum(pLayer) > 0);
}

void MainFrame::OnViewMagnifier(wxCommandEvent &event)
{
	m_pView->SetMode(LB_Mag);
	m_pView->SetCorrectCursor();
}

void MainFrame::OnUpdateMagnifier(wxUpdateUIEvent& event)
{
	event.Check( m_pView->GetMode() == LB_Mag );
}

void MainFrame::OnViewPan(wxCommandEvent &event)
{
	m_pView->SetMode(LB_Pan);
	m_pView->SetCorrectCursor();
}

void MainFrame::OnUpdatePan(wxUpdateUIEvent& event)
{
	event.Check( m_pView->GetMode() == LB_Pan );
}

void MainFrame::OnViewDistance(wxCommandEvent &event)
{
	m_pView->SetMode(LB_Dist);
	m_pView->SetCorrectCursor();
	ShowDistanceDlg();
}

void MainFrame::OnUpdateDistance(wxUpdateUIEvent& event)
{
	event.Check( m_pView->GetMode() == LB_Dist );
}

void MainFrame::OnViewSetArea(wxCommandEvent& event)
{
	m_pView->SetMode(LB_Box);
}

void MainFrame::OnUpdateViewSetArea(wxUpdateUIEvent& event)
{
	event.Check(m_pView->GetMode() == LB_Box);
}

void MainFrame::OnViewZoomIn(wxCommandEvent &event)
{
	m_pView->SetScale(m_pView->GetScale() * sqrt(2.0));
	RefreshStatusBar();
}

void MainFrame::OnViewZoomOut(wxCommandEvent &event)
{
	m_pView->SetScale(m_pView->GetScale() / sqrt(2.0));
	RefreshStatusBar();
}

void MainFrame::OnViewZoomAll(wxCommandEvent &event)
{
	ZoomAll();
}

void MainFrame::OnViewZoomToLayer(wxCommandEvent &event)
{
	vtLayer *lp = GetActiveLayer();
	DRECT rect;
	if (lp->GetExtent(rect))
		m_pView->ZoomToRect(rect, 0.1f);
}

void MainFrame::OnUpdateViewZoomToLayer(wxUpdateUIEvent& event)
{
	event.Enable(GetActiveLayer() != NULL);
}

void MainFrame::OnViewFull(wxCommandEvent& event)
{
	vtElevLayer *pEL = GetActiveElevLayer();
	if (pEL)
		m_pView->MatchZoomToElev(pEL);
	vtImageLayer *pIL = GetActiveImageLayer();
	if (pIL)
		m_pView->MatchZoomToImage(pIL);
}

void MainFrame::OnUpdateViewFull(wxUpdateUIEvent& event)
{
	vtLayer *lp = GetActiveLayer();
	event.Enable(lp &&
			(lp->GetType() == LT_ELEVATION || lp->GetType() == LT_IMAGE));
}

void MainFrame::OnViewZoomArea(wxCommandEvent& event)
{
	m_pView->ZoomToRect(m_area, 0.1f);
}

void MainFrame::OnUpdateViewZoomArea(wxUpdateUIEvent& event)
{
	event.Enable(!m_area.IsEmpty());
}

void MainFrame::OnViewToolbar(wxCommandEvent& event)
{
	wxAuiPaneInfo &info = m_mgr.GetPane(m_pToolbar);
	info.Show(!info.IsShown());
	m_mgr.Update();
}

void MainFrame::OnUpdateViewToolbar(wxUpdateUIEvent& event)
{
	wxAuiPaneInfo &info = m_mgr.GetPane(m_pToolbar);
	event.Check(info.IsShown());
}

void MainFrame::OnViewLayers(wxCommandEvent& event)
{
	wxAuiPaneInfo &info = m_mgr.GetPane(m_pTree);
	info.Show(!info.IsShown());
	m_mgr.Update();
}

void MainFrame::OnUpdateViewLayers(wxUpdateUIEvent& event)
{
	wxAuiPaneInfo &info = m_mgr.GetPane(m_pTree);
	event.Check(info.IsShown());
}

void MainFrame::OnViewWorldMap(wxCommandEvent& event)
{
	m_pView->SetShowMap(!m_pView->GetShowMap());
	m_pView->Refresh();
}

void MainFrame::OnUpdateWorldMap(wxUpdateUIEvent& event)
{
	event.Check(m_pView->GetShowMap());
}

void MainFrame::OnViewUTMBounds(wxCommandEvent& event)
{
	m_pView->m_bShowUTMBounds = !m_pView->m_bShowUTMBounds;
	m_pView->Refresh();
}

void MainFrame::OnUpdateUTMBounds(wxUpdateUIEvent& event)
{
	event.Check(m_pView->m_bShowUTMBounds);
}

void MainFrame::OnViewProfile(wxCommandEvent& event)
{
	if (m_pProfileDlg && m_pProfileDlg->IsShown())
		m_pProfileDlg->Hide();
	else
		ShowProfileDlg();
}

void MainFrame::OnUpdateViewProfile(wxUpdateUIEvent& event)
{
	event.Check(m_pProfileDlg && m_pProfileDlg->IsShown());
	event.Enable(LayersOfType(LT_ELEVATION) > 0);
}

void MainFrame::OnViewScaleBar(wxCommandEvent& event)
{
	m_pView->SetShowScaleBar(!m_pView->GetShowScaleBar());
}

void MainFrame::OnUpdateViewScaleBar(wxUpdateUIEvent& event)
{
	event.Check(m_pView && m_pView->GetShowScaleBar());
}

void MainFrame::OnViewOptions(wxCommandEvent& event)
{
	ShowOptionsDialog();
}


//////////////////////////
// Road

void MainFrame::OnSelectLink(wxCommandEvent &event)
{
	m_pView->SetMode(LB_Link);
}

void MainFrame::OnUpdateSelectLink(wxUpdateUIEvent& event)
{
	event.Check( m_pView->GetMode() == LB_Link );
}

void MainFrame::OnSelectNode(wxCommandEvent &event)
{
	m_pView->SetMode(LB_Node);
}

void MainFrame::OnUpdateSelectNode(wxUpdateUIEvent& event)
{
	event.Check( m_pView->GetMode() == LB_Node );
}

void MainFrame::OnSelectWhole(wxCommandEvent &event)
{
	m_pView->SetMode(LB_LinkExtend);
}

void MainFrame::OnUpdateSelectWhole(wxUpdateUIEvent& event)
{
	event.Check( m_pView->GetMode() == LB_LinkExtend );
}

void MainFrame::OnDirection(wxCommandEvent &event)
{
	m_pView->SetMode(LB_Dir);
}

void MainFrame::OnUpdateDirection(wxUpdateUIEvent& event)
{
	event.Check( m_pView->GetMode() == LB_Dir );
}

void MainFrame::OnRoadEdit(wxCommandEvent &event)
{
	m_pView->SetMode(LB_LinkEdit);
}

void MainFrame::OnUpdateRoadEdit(wxUpdateUIEvent& event)
{
	event.Check( m_pView->GetMode() == LB_LinkEdit );
}

void MainFrame::OnRoadShowNodes(wxCommandEvent &event)
{
	bool state = vtRoadLayer::GetDrawNodes();
	vtRoadLayer::SetDrawNodes(!state);
	m_pView->Refresh(state);
}

void MainFrame::OnUpdateRoadShowNodes(wxUpdateUIEvent& event)
{
	event.Check(vtRoadLayer::GetDrawNodes());
}

void MainFrame::OnSelectHwy(wxCommandEvent &event)
{
	vtRoadLayer *pRL = GetActiveRoadLayer();
	if (!pRL) return;

	wxTextEntryDialog dlg(this, _("Please enter highway number"),
		_("Select Highway"), _T(""));
	if (dlg.ShowModal() == wxID_OK)
	{
		int num;
		wxString str = dlg.GetValue();
		sscanf(str.mb_str(wxConvUTF8), "%d", &num);
		if (pRL->SelectHwyNum(num))
			m_pView->Refresh();
	}
}

void MainFrame::OnRoadClean(wxCommandEvent &event)
{
	vtRoadLayer *pRL = GetActiveRoadLayer();
	if (!pRL) return;

	// check projection
	vtProjection proj;
	pRL->GetProjection(proj);
	bool bDegrees = (proj.IsGeographic() != 0);

	int count;
	OpenProgressDialog(_("Cleaning RoadMap"));

	UpdateProgressDialog(10, _("Removing unused nodes"));
	count = pRL->RemoveUnusedNodes();
	if (count)
	{
		DisplayAndLog("Removed %i nodes", count);
		pRL->SetModified(true);
	}

	UpdateProgressDialog(20, _("Merging redundant nodes"));
	// potentially takes a long time...
	count = pRL->MergeRedundantNodes(bDegrees, progress_callback);
	if (count)
	{
		DisplayAndLog("Merged %d redundant roads", count);
		pRL->SetModified(true);
	}

	UpdateProgressDialog(30, _("Cleaning link points"));
	count = pRL->CleanLinkPoints();
	if (count)
	{
		DisplayAndLog("Cleaned %d link points", count);
		pRL->SetModified(true);
	}

	UpdateProgressDialog(40, _T("Removing degenerate links"));
	count = pRL->RemoveDegenerateLinks();
	if (count)
	{
		DisplayAndLog("Removed %d degenerate links", count);
		pRL->SetModified(true);
	}

#if 0
	// The following cleanup operations are disabled until they are proven safe!

	UpdateProgressDialog(40, _T("Removing unnecessary nodes"));
	count = pRL->RemoveUnnecessaryNodes();
	if (count)
	{
		DisplayAndLog("Removed %d unnecessary nodes", count);
	}

	UpdateProgressDialog(60, _T("Removing dangling links"));
	count = pRL->DeleteDanglingRoads();
	if (count)
	{
		DisplayAndLog("Removed %i dangling links", count);
	}

	UpdateProgressDialog(70, _T("Fixing overlapped roads"));
	count = pRL->FixOverlappedRoads(bDegrees);
	if (count)
	{
		DisplayAndLog("Fixed %i overlapped roads", count);
	}

	UpdateProgressDialog(80, _T("Fixing extraneous parallels"));
	count = pRL->FixExtraneousParallels();
	if (count)
	{
		DisplayAndLog("Fixed %i extraneous parallels", count);
	}

	UpdateProgressDialog(90, _T("Splitting looping roads"));
	count = pRL->SplitLoopingRoads();
	if (count)
	{
		DisplayAndLog("Split %d looping roads", count);
	}
#endif

	CloseProgressDialog();
	pRL->ComputeExtents();

	m_pView->Refresh();
}

void MainFrame::OnRoadGuess(wxCommandEvent &event)
{
	vtRoadLayer *pRL = GetActiveRoadLayer();
	if (!pRL) return;

	// Set visual properties
	pRL->GuessIntersectionTypes();

	for (NodeEdit *pN = pRL->GetFirstNode(); pN; pN = pN->GetNext())
		pN->DetermineVisualFromLinks();

	m_pView->Refresh();
}

void MainFrame::OnRoadFlatten(wxCommandEvent &event)
{
	if (m_proj.IsGeographic())
	{
		wxMessageBox(_("Sorry, but precise grid operations require a non-geographic coordinate\n system (meters as horizontal units, not degrees.)"),
			_("Info"), wxOK);
		return;
	}

	float margin = 2.0;
	wxString str;
	str.Printf(_T("%g"), margin);
	str = wxGetTextFromUser(_("How many meters for the margin at the edge of each road?"),
		_("Flatten elevation grid under roads"), str, this);
	if (str == _T(""))
		return;

	margin = atof(str.mb_str(wxConvUTF8));

	vtRoadLayer *pR = (vtRoadLayer *)GetMainFrame()->FindLayerOfType(LT_ROAD);
	vtElevLayer *pE = (vtElevLayer *)GetMainFrame()->FindLayerOfType(LT_ELEVATION);

	pR->CarveRoadway(pE, margin);
	m_pView->Refresh();
}

void MainFrame::OnUpdateRoadFlatten(wxUpdateUIEvent& event)
{
	vtElevLayer *pE = (vtElevLayer *)GetMainFrame()->FindLayerOfType(LT_ELEVATION);

	event.Enable(pE != NULL && pE->m_pGrid != NULL);
}


//////////////////////////
// Elevation

void MainFrame::OnUpdateIsElevation(wxUpdateUIEvent& event)
{
	event.Enable(GetActiveElevLayer() != NULL);
}

void MainFrame::OnUpdateIsGrid(wxUpdateUIEvent& event)
{
	vtElevLayer *pEL = GetActiveElevLayer();
	event.Enable(pEL && pEL->IsGrid());
}

void MainFrame::OnElevSelect(wxCommandEvent& event)
{
	m_pView->SetMode(LB_TSelect);
}

void MainFrame::OnUpdateElevSelect(wxUpdateUIEvent& event)
{
	event.Check(m_pView->GetMode() == LB_TSelect);
}

void MainFrame::OnRemoveElevRange(wxCommandEvent &event)
{
	vtElevLayer *t = GetActiveElevLayer();
	if (!t && !t->m_pGrid)
		return;

	wxString str;
	str = wxGetTextFromUser(_("Please specify the elevation range\n(minimum and maximum in the form \"X Y\")\nAll values within this range (and within the area\ntool, if it is defined) will be set to Unknown."));

	float zmin, zmax;
	vtString text = (const char *) str.mb_str(wxConvUTF8);
	if (sscanf(text, "%f %f", &zmin, &zmax) != 2)
	{
		wxMessageBox(_("Didn't get two numbers."));
		return;
	}
	int count = t->RemoveElevRange(zmin, zmax, m_area.IsEmpty() ? NULL : &m_area);
	if (count)
	{
		wxString str;
		str.Printf(_("Set %d heixels to unknown"), count);
		wxMessageBox(str);
		t->SetModified(true);
		t->ReRender();
		m_pView->Refresh();
	}
}

void MainFrame::OnElevSetUnknown(wxCommandEvent &event)
{
	vtElevLayer *t = GetActiveElevLayer();
	if (!t)	return;

	static float fValue = 1.0f;
	wxString str;
	str.Printf(_T("%g"), fValue);
	str = wxGetTextFromUser(_("Set unknown areas to what value?"),
		_("Set Unknown Areas"), str, this);
	if (str == _T(""))
		return;

	fValue = atof(str.mb_str(wxConvUTF8));
	int count = t->SetUnknown(fValue, &m_area);
	if (count)
	{
		t->SetModified(true);
		t->ReRender();
		m_pView->Refresh();
	}
}

void MainFrame::OnFillIn(int method)
{
	vtElevLayer *el = GetActiveElevLayer();

	DRECT *area = NULL;
	if (!m_area.IsEmpty())
		area = &m_area;

	if (FillElevGaps(el, area, method))
	{
		el->SetModified(true);
		el->ReRender();
		m_pView->Refresh();
	}
}

void MainFrame::OnFillFast(wxCommandEvent &event)
{
	OnFillIn(1);
}

void MainFrame::OnFillSlow(wxCommandEvent &event)
{
	OnFillIn(2);
}

void MainFrame::OnFillRegions(wxCommandEvent &event)
{
	OnFillIn(3);
}

void MainFrame::OnScaleElevation(wxCommandEvent &event)
{
	vtElevLayer *el = GetActiveElevLayer();
	if (!el)
		return;

	wxString str = wxGetTextFromUser(_("Please enter a scale factor"),
		_("Scale Elevation"), _T("1.0"), this);
	if (str == _T(""))
		return;

	float fScale;
	fScale = atof(str.mb_str(wxConvUTF8));
	if (fScale == 0.0f)
	{
		wxMessageBox(_("Couldn't parse the number you typed."));
		return;
	}
	if (fScale == 1.0f)
		return;

	vtElevationGrid *grid = el->m_pGrid;
	if (grid)
	{
		grid->Scale(fScale, true);
		el->ReRender();
	}
	vtTin2d *tin = el->m_pTin;
	if (tin)
	{
		tin->Scale(fScale);
	}
	el->SetModified(true);
	m_pView->Refresh();
}

void MainFrame::OnVertOffsetElevation(wxCommandEvent &event)
{
	vtElevLayer *el = GetActiveElevLayer();
	if (!el)
		return;

	wxString str = wxGetTextFromUser(_("Please enter an amout to offset"),
		_("Offset Elevation Vertically"), _T("0.0"), this);
	if (str == _T(""))
		return;

	float fValue;
	fValue = atof(str.mb_str(wxConvUTF8));
	if (fValue == 0.0f)
	{
		wxMessageBox(_("Couldn't parse the number you typed."));
		return;
	}

	vtElevationGrid *grid = el->m_pGrid;
	if (grid)
	{
		grid->VertOffset(fValue);
		el->ReRender();
	}
	vtTin2d *tin = el->m_pTin;
	if (tin)
	{
		tin->VertOffset(fValue);
	}
	el->SetModified(true);
	m_pView->Refresh();
}

void MainFrame::OnElevExport(wxCommandEvent &event)
{
	if (!GetActiveElevLayer())
		return;

	wxString choices[13];
	choices[0] = _T("3TX");
	choices[1] = _T("ArcInfo ASCII Grid");
	choices[2] = _T("BMP");
	choices[3] = _T("ChunkLOD (.chu)");
	choices[4] = _T("GeoTIFF");
	choices[5] = _T("MSI Planet");
	choices[6] = _T("PNG (16-bit greyscale)");
	choices[7] = _T("RAW/INF for MS Flight Simulator");
	choices[8] = _T("STM");
	choices[9] = _T("TIN (.itf)");
	choices[10] = _T("TerraGen");
	choices[11] = _T("VRML ElevationGrid");
	choices[12] = _T("XYZ ASCII Points");

	wxSingleChoiceDialog dlg(this, _("Please choose"),
		_("Export to file format:"), 13, choices);
	if (dlg.ShowModal() != wxID_OK)
		return;

	switch (dlg.GetSelection())
	{
	case 0: Export3TX(); break;
	case 1: ExportASC(); break;
	case 2: ExportBMP(); break;
	case 3: ExportChunkLOD(); break;
	case 4: ExportGeoTIFF(); break;
	case 5: ExportPlanet(); break;
	case 6: ExportPNG16(); break;
	case 7: ExportRAWINF(); break;
	case 8: ExportSTM(); break;
	case 9: ExportTIN(); break;
	case 10: ExportTerragen(); break;
	case 11: ExportVRML(); break;
	case 12: ExportXYZ(); break;
	}
}

void MainFrame::OnElevExportTiles(wxCommandEvent& event)
{
	ElevExportTiles(m_pView);
}

void MainFrame::OnElevCopy(wxCommandEvent& event)
{
	DoElevCopy();
}

void MainFrame::OnElevPasteNew(wxCommandEvent& event)
{
	DoElevPasteNew();
}

void MainFrame::OnElevExportBitmap(wxCommandEvent& event)
{
	int cols, rows;
	vtElevLayer *pEL = GetActiveElevLayer();
	pEL->m_pGrid->GetDimensions(cols, rows);

	RenderDlg dlg(this, -1, _("Render Elevation to Bitmap"));
	dlg.m_iSizeX = cols;
	dlg.m_iSizeY = rows;

	if (dlg.ShowModal() == wxID_CANCEL)
		return;

	OpenProgressDialog(_("Generating Bitmap"));
	ExportBitmap(GetActiveElevLayer(), dlg);
	CloseProgressDialog();
}

void MainFrame::OnElevToTin(wxCommandEvent& event)
{
	vtElevLayer *pEL1 = GetActiveElevLayer();
	vtElevationGrid *grid = pEL1->m_pGrid;

	vtTin2d *tin = new vtTin2d(grid);
	vtElevLayer *pEL = new vtElevLayer;
	pEL->SetTin(tin);
	AddLayer(pEL);
	SetActiveLayer(pEL);

	m_pView->Refresh();
	RefreshTreeView();
}

void MainFrame::OnElevMergeTin(wxCommandEvent& event)
{
	vtElevLayer *pEL = GetActiveElevLayer();
	pEL->MergeSharedVerts();
}

void MainFrame::OnUpdateElevMergeTin(wxUpdateUIEvent& event)
{
	vtElevLayer *pEL = GetActiveElevLayer();
	event.Enable(pEL && !pEL->IsGrid());
}

void MainFrame::OnElevTrimTin(wxCommandEvent& event)
{
	m_pView->SetMode(LB_TrimTIN);
	m_pView->SetCorrectCursor();
}

void MainFrame::OnUpdateElevTrimTin(wxUpdateUIEvent& event)
{
	vtElevLayer *pEL = GetActiveElevLayer();
	event.Enable(pEL && !pEL->IsGrid());
	event.Check(m_pView->GetMode() == LB_TrimTIN);
}



//////////////////////////////////////////////////////////////////////////
// Image Menu
//

void MainFrame::OnImageReplaceRGB(wxCommandEvent& event)
{
	wxString msg = _("R G B in the range 0-255:");
	wxString str = wxGetTextFromUser(msg, _("Replace color:"));
	if (str == _T(""))
		return;
	RGBi rgb1, rgb2;
	int count = sscanf(str.mb_str(), "%d %d %d", &rgb1.r, &rgb1.g, &rgb1.b);
	if (count != 3 || rgb1.r < 0 || rgb1.r > 255 || rgb1.g < 0 ||
		rgb1.g > 255 || rgb1.b < 0 || rgb1.b > 255)
	{
		wxMessageBox(_("Didn't get three R G B values in range."));
		return;
	}
	str = wxGetTextFromUser(msg, _("With color:"));
	if (str == _T(""))
		return;
	count = sscanf(str.mb_str(), "%d %d %d", &rgb2.r, &rgb2.g, &rgb2.b);
	if (count != 3 || rgb2.r < 0 || rgb2.r > 255 || rgb2.g < 0 ||
		rgb2.g > 255 || rgb2.b < 0 || rgb2.b > 255)
	{
		wxMessageBox(_("Didn't get three R G B values in range."));
		return;
	}
	GetActiveImageLayer()->ReplaceColor(rgb1, rgb2);
	RefreshLayerInView(GetActiveImageLayer());
}

void MainFrame::OnImageCreateOverviews(wxCommandEvent& event)
{
	vtImageLayer *pIL = GetActiveImageLayer();

	OpenProgressDialog(_("Creating Overviews"), false, this);

	pIL->GetImage()->CreateOverviews();

	CloseProgressDialog();
}

void MainFrame::OnImageCreateOverviewsAll(wxCommandEvent& event)
{
	OpenProgressDialog(_("Creating Overviews"), false, this);
	for (int i = 0; i < NumLayers(); i++)
	{
		vtImageLayer *pIL = dynamic_cast<vtImageLayer *>(GetLayer(i));
		if (pIL)
			pIL->GetImage()->CreateOverviews();
	}
	CloseProgressDialog();
}

void MainFrame::OnImageCreateMipMaps(wxCommandEvent& event)
{
	vtImageLayer *pIL = GetActiveImageLayer();

	OpenProgressDialog(_("Creating MipMaps"), false, this);

	pIL->GetImage()->AllocMipMaps();
	pIL->GetImage()->DrawMipMaps();

	CloseProgressDialog();
}

void MainFrame::OnImageExportTiles(wxCommandEvent& event)
{
	ImageExportTiles(m_pView);
}

void MainFrame::OnImageExportPPM(wxCommandEvent& event)
{
	ImageExportPPM();
}

void MainFrame::OnUpdateHaveImageLayer(wxUpdateUIEvent& event)
{
	vtImageLayer *pIL = GetActiveImageLayer();
	event.Enable(pIL != NULL);
}


//////////////////////////////////////////////////////////////////////////
// Area Menu
//

void MainFrame::OnAreaClear(wxCommandEvent &event)
{
	m_pView->InvertAreaTool(m_area);
	m_area.SetRect(0, 0, 0, 0);
	m_pView->InvertAreaTool(m_area);
}

void MainFrame::OnAreaZoomAll(wxCommandEvent &event)
{
	m_pView->InvertAreaTool(m_area);
	m_area = GetExtents();
	m_pView->InvertAreaTool(m_area);
}

void MainFrame::OnUpdateAreaZoomAll(wxUpdateUIEvent& event)
{
	event.Enable(NumLayers() != 0);
}

void MainFrame::OnAreaZoomLayer(wxCommandEvent &event)
{
	DRECT area;
	if (GetActiveLayer()->GetExtent(area))
	{
		m_pView->InvertAreaTool(m_area);
		m_area = area;
		m_pView->InvertAreaTool(m_area);
	}
}

void MainFrame::OnUpdateAreaZoomLayer(wxUpdateUIEvent& event)
{
	event.Enable(GetActiveLayer() != NULL);
}

void MainFrame::OnUpdateAreaMatch(wxUpdateUIEvent& event)
{
	int iRasters = LayersOfType(LT_ELEVATION) + LayersOfType(LT_IMAGE);
	event.Enable(!m_area.IsEmpty() && iRasters > 0);
}

void MainFrame::OnAreaTypeIn(wxCommandEvent &event)
{
	ExtentDlg dlg(NULL, -1, _("Edit Area"));
	dlg.SetArea(m_area, (m_proj.IsGeographic() != 0));
	if (dlg.ShowModal() == wxID_OK)
	{
		m_area = dlg.m_area;
		m_pView->Refresh();
	}
}

void MainFrame::OnAreaMatch(wxCommandEvent &event)
{
	MatchDlg dlg(NULL, -1, _("Match Area and Tiling to Layer"));
	dlg.SetView(GetView());
	dlg.SetArea(m_area, (m_proj.IsGeographic() != 0));
	if (dlg.ShowModal() == wxID_OK)
	{
		m_tileopts.cols = dlg.m_tile.x;
		m_tileopts.rows = dlg.m_tile.y;
		m_tileopts.lod0size = dlg.m_iTileSize;
		m_area = dlg.m_area;
		m_pView->Refresh();
	}
	GetView()->HideGridMarks();
}

void MainFrame::OnAreaRequestWFS(wxCommandEvent& event)
{
#if SUPPORT_CURL
	bool success;

	wxTextEntryDialog dlg(this, _T("WFS Server address"),
		_T("Please enter server base URL"), _T("http://10.254.0.29:8081/"));
	if (dlg.ShowModal() != wxID_OK)
		return;
	wxString value = dlg.GetValue();
	vtString server = (const char *) value.mb_str(wxConvUTF8);

	OGCLayerArray layers;
	success = GetLayersFromWFS(server, layers);

	int numlayers = layers.size();
	wxString choices[100];
	for (int i = 0; i < numlayers; i++)
	{
		const char *string = layers[i]->GetValueString("Name");
		choices[i] = wxString::FromAscii(string);
	}

	wxSingleChoiceDialog dlg2(this, _T("Choice Layer"),
		_T("Please indicate layer:"), numlayers, (const wxString *)choices);

	if (dlg2.ShowModal() != wxID_OK)
		return;

	vtRawLayer *pRL = new vtRawLayer;
	success = pRL->ReadFeaturesFromWFS(server, "rail");
	if (success)
		AddLayerWithCheck(pRL);
	else
		delete pRL;
#endif
}

void MainFrame::OnAreaRequestWMS(wxCommandEvent& event)
{
	if (m_wms_servers.empty())
	{
		// supply some hardcoded well-known servers
		OGCServer s;
		s.m_url = "http://wmt.jpl.nasa.gov/wms.cgi";
		m_wms_servers.push_back(s);
		s.m_url = "http://globe.digitalearth.gov/viz-bin/wmt.cgi";
		m_wms_servers.push_back(s);
	}

#if SUPPORT_CURL
	// Ask the user for what server and layer they want
	if (!m_pMapServerDlg)
		m_pMapServerDlg = new MapServerDlg(this, -1, _T("WMS Request"));

	m_pMapServerDlg->m_area = m_area;
	m_pMapServerDlg->m_proj = m_proj;
	m_pMapServerDlg->SetServerArray(m_wms_servers);

	if (m_pMapServerDlg->ShowModal() != wxID_OK)
		return;

	// Prepare to receive the WMS data
	if (m_pMapServerDlg->m_bNewLayer)
	{
		// Enforce PNG, that's all we support so far
		m_pMapServerDlg->m_iFormat = 1;	// png
		m_pMapServerDlg->UpdateURL();
	}
	FILE *fp;
	wxString str;
	if (m_pMapServerDlg->m_bToFile)
	{
		// Very simple: just write the buffer to disk
		fp = vtFileOpen(m_pMapServerDlg->m_strToFile.mb_str(wxConvUTF8), "wb");
		if (!fp)
		{
			str = _("Could not open file");
			str += _T(" '");
			str += m_pMapServerDlg->m_strToFile;
			str += _T("'");
			wxMessageBox(str);
			return;
		}
	}

	// Bring down the WMS data
	OpenProgressDialog(_("Requesting data"), false, this);
	ReqContext rc;
	rc.SetProgressCallback(progress_callback);
	vtBytes data;
	bool success = rc.GetURL(m_pMapServerDlg->m_strQueryURL.mb_str(wxConvUTF8), data);
	CloseProgressDialog();

	if (!success)
	{
		str = wxString(rc.GetErrorMsg(), wxConvUTF8);	// the HTTP request failed
		wxMessageBox(str);
		return;
	}
	if (data.Len() > 5 &&
		(!strncmp((char *)data.Get(), "<?xml", 5) ||
		 !strncmp((char *)data.Get(), "<WMT", 4) ||
		 !strncmp((char *)data.Get(), "<!DOC", 5)))
	{
		// We got an XML-formatted response, not the image we were expecting.
		// The XML probably contains diagnostic error msg.
		// So show it to the user.
		unsigned char ch = 0;
		data.Append(&ch, 1);
		str = wxString((const char*) data.Get(), wxConvUTF8);
		wxMessageBox(str);
		return;
	}

	if (m_pMapServerDlg->m_bNewLayer)
	{
		// Now data contains the PNG file in memory, so parse it.
		vtImageLayer *pIL = new vtImageLayer;
		success = pIL->GetImage()->ReadPNGFromMemory(data.Get(), data.Len());
		if (success)
		{
			pIL->SetExtent(m_area);
			pIL->SetProjection(m_proj);
			AddLayerWithCheck(pIL);
		}
		else
			delete pIL;
	}
	if (m_pMapServerDlg->m_bToFile)
	{
		fwrite(data.Get(), data.Len(), 1, fp);
		fclose(fp);
	}
#endif
}

void MainFrame::OnUpdateAreaRequestWMS(wxUpdateUIEvent& event)
{
	event.Enable(!m_area.IsEmpty() && SUPPORT_CURL);
}

void MainFrame::OnAreaRequestTServe(wxCommandEvent& event)
{
	int zone = m_proj.GetUTMZone();
	if (zone == 0 || m_area.IsEmpty())
	{
		wxMessageBox(
			_T("In order to request data from Terraserver, first set your CRS to\n")
			_T("a UTM zone (4 through 19) and use the Area Tool to indicate the\n")
			_T("area that you want to download."),
			_T("Note"));
		return;
	}

	TSDialog dlg(this, -1, _("Terraserver"));
	if (dlg.ShowModal() != wxID_OK)
		return;
	if (dlg.m_strToFile == _T(""))
		return;

	OpenProgressDialog(_("Requesting data from Terraserver..."));

	vtImageLayer *pIL = new vtImageLayer;
	bool success = pIL->GetImage()->ReadFeaturesFromTerraserver(m_area, dlg.m_iTheme,
		dlg.m_iMetersPerPixel, m_proj.GetUTMZone(), dlg.m_strToFile.mb_str(wxConvUTF8));

	CloseProgressDialog();

	if (success)
		wxMessageBox(_("Successfully wrote file."));
	if (!success)
	{
		wxMessageBox(_("Unable to download."));
	}
	delete pIL;
	return;
#if 0
	if (dlg.m_bNewLayer)
	{
		if (!AddLayerWithCheck(pIL))
			delete pIL;
	}
	else
	{
		pIL->SaveToFile(dlg.m_strToFile.mb_str(wxConvUTF8));
		delete pIL;
	}
#endif
}


//////////////////////////
// Vegetation menu

void MainFrame::OnVegPlants(wxCommandEvent& event)
{
	// if PlantList has not previously been open, get the data from file first
	if (m_strSpeciesFilename == "")
	{
		wxString filter = _("Plant Species List Files (*.xml)|*.xml");

		// Use file dialog to open plant list text file.
		wxFileDialog loadFile(NULL, _("Load Plant Info"), _T(""), _T(""),
			filter, wxFD_OPEN);

		if (loadFile.ShowModal() != wxID_OK)
			return;

		wxString str = loadFile.GetPath();
		if (!LoadSpeciesFile(str.mb_str(wxConvUTF8)))
			return;
	}
	if (!m_SpeciesListDlg)
	{
		// Create new Plant List Dialog
		m_SpeciesListDlg = new SpeciesListDlg(this, wxID_ANY, _("Plants List"),
				wxPoint(140, 100), wxSize(950, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
	}

	// Display plant list data, calling OnInitDialog.
	m_SpeciesListDlg->Show(true);
}


void MainFrame::OnVegBioregions(wxCommandEvent& event)
{
	// if data isn't there, get the data first
	if (m_strBiotypesFilename == "")
	{
		wxString filter = _("Bioregion Files (*.txt)|*.txt");

		// Use file dialog to open bioregion text file.
		wxFileDialog loadFile(NULL, _("Load BioRegion Info"), _T(""), _T(""),
			filter, wxFD_OPEN);

		if (loadFile.ShowModal() != wxID_OK)
			return;

		// Read bioregions, data kept on frame with m_pBioRegion.
		wxString str = loadFile.GetPath();
		if (!LoadBiotypesFile(str.mb_str(wxConvUTF8)))
			return;
	}
	if (!m_BioRegionDlg)
	{
		// Create new Bioregion Dialog
		m_BioRegionDlg = new BioRegionDlg(this, wxID_ANY, _("BioRegions List"),
				wxPoint(120, 80), wxSize(300, 500), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
	}

	// Display bioregion data, calling OnInitDialog.
	m_BioRegionDlg->Show(true);
}

void MainFrame::OnVegRemap(wxCommandEvent& event)
{
	vtVegLayer *pVeg = GetMainFrame()->GetActiveVegLayer();
	if (!pVeg) return;

	vtSpeciesList *list = GetPlantList();

	wxArrayString choices;
	unsigned int i, n = list->NumSpecies();
	for (i = 0; i < n; i++)
	{
		vtPlantSpecies *spe = list->GetSpecies(i);
		wxString str(spe->GetSciName(), wxConvUTF8);
		choices.Add(str);
	}

	wxString result1 = wxGetSingleChoice(_("Remap FROM Species"), _T("Species"),
		choices, this);
	if (result1 == _T(""))	// cancelled
		return;
	short species_from = list->GetSpeciesIdByName(result1.mb_str(wxConvUTF8));

	wxString result2 = wxGetSingleChoice(_("Remap TO Species"), _T("Species"),
		choices, this);
	if (result2 == _T(""))	// cancelled
		return;
	short species_to = list->GetSpeciesIdByName(result2.mb_str(wxConvUTF8));

	vtFeatureSet *pSet = pVeg->GetFeatureSet();
	vtPlantInstanceArray *pPIA = dynamic_cast<vtPlantInstanceArray *>(pSet);
	if (!pPIA)
		return;

	float size;
	short species_id;
	int count = 0;
	for (i = 0; i < pPIA->GetNumEntities(); i++)
	{
		pPIA->GetPlant(i, size, species_id);
		if (species_id == species_from)
		{
			pPIA->SetPlant(i, size, species_to);
			count++;
		}
	}
	wxString str;
	str.Printf(_("Remap successful, %d plants remapped.\n"), count);
	wxMessageBox(str, _("Info"));
	if (count > 0)
		pVeg->SetModified(true);
}

void MainFrame::OnVegExportSHP(wxCommandEvent& event)
{
	vtVegLayer *pVeg = GetMainFrame()->GetActiveVegLayer();
	if (!pVeg) return;

	// Open File Save Dialog
	wxFileDialog saveFile(NULL, _("Export vegetation to SHP"), _T(""), _T(""),
		_("Vegetation Files (*.shp)|*.shp"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (saveFile.ShowModal() == wxID_CANCEL)
		return;
	wxString strPathName = saveFile.GetPath();

	pVeg->ExportToSHP(strPathName.mb_str(wxConvUTF8));
}

void MainFrame::OnUpdateVegExportSHP(wxUpdateUIEvent& event)
{
	vtVegLayer *pVeg = GetMainFrame()->GetActiveVegLayer();
	event.Enable(pVeg && pVeg->IsNative());
}

void MainFrame::OnAreaGenerateVeg(wxCommandEvent& event)
{
	// Open File Save Dialog
	wxFileDialog saveFile(NULL, _("Save Vegetation File"), _T(""), _T(""),
		_("Vegetation Files (*.vf)|*.vf"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (saveFile.ShowModal() == wxID_CANCEL)
		return;
	wxString strPathName = saveFile.GetPath();

	DistribVegDlg dlg(this, -1, _("Vegetation Distribution Options"));

	if (dlg.ShowModal() == wxID_CANCEL)
		return;

	// Generate the plants
	GenerateVegetation(strPathName.mb_str(wxConvUTF8), m_area, dlg.m_opt);
}

void MainFrame::OnUpdateAreaGenerateVeg(wxUpdateUIEvent& event)
{
	// we needs some species, and an area to plant them in
	event.Enable(m_strSpeciesFilename != "" && !m_area.IsEmpty());
}

void MainFrame::OnAreaVegDensity(wxCommandEvent& event)
{
	wxString str, s;

	LinearUnits lu = m_proj.GetUnits();
	float xsize = m_area.Width() * GetMetersPerUnit(lu);
	float ysize = m_area.Height() * GetMetersPerUnit(lu);
	float area = xsize * ysize;

	s.Printf(_("Total area: %.1f square meters (%.3f hectares)\n"), area, area/10000);
	str += s;

	// Get all the objects we'll need
	vtVegLayer *vlay = (vtVegLayer *) FindLayerOfType(LT_VEG);
	if (!vlay) return;
	vtPlantInstanceArray *pia = vlay->GetPIA();
	if (!pia) return;
	unsigned int ent = pia->GetNumEntities();
	vtSpeciesList *list = GetMainFrame()->GetPlantList();

	// Put the results in a biotype as well
	vtBioType btype;

	float size;
	short species;
	int total = 0;
	for (unsigned int i = 0; i < list->NumSpecies(); i++)
	{
		int count = 0;
		float height = 0;
		for (unsigned int j = 0; j < ent; j++)
		{
			pia->GetPlant(j, size, species);
			DPoint2 &p = pia->GetPoint(j);
			if (species == i && m_area.ContainsPoint(p))
			{
				total++;
				count++;
				height += size;
			}
		}
		if (count != 0)
		{
			vtPlantSpecies *spe = list->GetSpecies(i);
			float density = (float) count / area;
			s.Printf(_("  %d instances of species %hs: %.5f per m^2, average height %.1f\n"),
				count, spe->GetSciName(), density, height/count);
			str += s;

			btype.AddPlant(spe, density, height/count);
		}
	}
	s.Printf(_("Total plant instances: %d\n"), total);
	str += s;
	wxMessageBox(str, _("Info"));

	vtBioRegion bregion;
	btype.m_name = "Default";
	bregion.AddType(&btype);
	bregion.WriteXML("bioregion.xml");
	bregion.Clear();
}

void MainFrame::OnUpdateAreaVegDensity(wxUpdateUIEvent& event)
{
	// we needs some plants, and an area to estimate
	vtVegLayer *vlay = (vtVegLayer *) FindLayerOfType(LT_VEG);
	event.Enable(m_strSpeciesFilename != "" && vlay != NULL &&
		vlay->GetVegType() == VLT_Instances && !m_area.IsEmpty());
}


//////////////////////////////
// Utilities Menu

void MainFrame::OnTowerSelect(wxCommandEvent& event)
{
	m_pView->SetMode(LB_TowerSelect);

}
void MainFrame::OnUpdateTowerSelect(wxUpdateUIEvent &event)
{
	event.Check(m_pView->GetMode()== LB_TowerSelect);
}
void MainFrame::OnTowerEdit(wxCommandEvent& event)
{
}
void MainFrame::OnUpdateTowerEdit(wxUpdateUIEvent &event)
{
}
void MainFrame::OnTowerAdd(wxCommandEvent& event)
{
	m_pView->SetMode(LB_TowerAdd);
}
void MainFrame::OnUpdateTowerAdd(wxUpdateUIEvent& event)
{
	event.Check(m_pView->GetMode()==LB_TowerAdd);
}


/////////////////////////////////////
// Buildings / Features

void MainFrame::OnFeatureSelect(wxCommandEvent &event)
{
	m_pView->SetMode(LB_FSelect);
}

void MainFrame::OnUpdateFeatureSelect(wxUpdateUIEvent& event)
{
	event.Check(m_pView->GetMode() == LB_FSelect);
}

void MainFrame::OnFeaturePick(wxCommandEvent &event)
{
	m_pView->SetMode(LB_FeatInfo);
}

void MainFrame::OnFeatureTable(wxCommandEvent &event)
{
	if (m_pFeatInfoDlg && m_pFeatInfoDlg->IsShown())
		m_pFeatInfoDlg->Show(false);
	else
	{
		ShowFeatInfoDlg();
		m_pFeatInfoDlg->SetLayer(GetActiveLayer());
		m_pFeatInfoDlg->SetFeatureSet(GetActiveRawLayer()->GetFeatureSet());
	}
}

void MainFrame::OnUpdateFeaturePick(wxUpdateUIEvent& event)
{
	event.Check(m_pView->GetMode() == LB_FeatInfo);
}

void MainFrame::OnUpdateFeatureTable(wxUpdateUIEvent& event)
{
	event.Check(m_pFeatInfoDlg && m_pFeatInfoDlg->IsShown());
}

void MainFrame::OnBuildingEdit(wxCommandEvent &event)
{
	m_pView->SetMode(LB_BldEdit);
}

void MainFrame::OnUpdateBuildingEdit(wxUpdateUIEvent& event)
{
	event.Check(m_pView->GetMode() == LB_BldEdit);
}

void MainFrame::OnBuildingAddPoints(wxCommandEvent &event)
{
	m_pView->SetMode(LB_BldAddPoints);
}

void MainFrame::OnUpdateBuildingAddPoints(wxUpdateUIEvent& event)
{
	event.Check(m_pView->GetMode() == LB_BldAddPoints);
}

void MainFrame::OnBuildingDeletePoints(wxCommandEvent &event)
{
	m_pView->SetMode(LB_BldDeletePoints);
}

void MainFrame::OnUpdateBuildingDeletePoints(wxUpdateUIEvent& event)
{
	event.Check(m_pView->GetMode() == LB_BldDeletePoints);
}

void MainFrame::OnStructureAddLinear(wxCommandEvent &event)
{
	m_pView->SetMode(LB_AddLinear);
}

void MainFrame::OnUpdateStructureAddLinear(wxUpdateUIEvent& event)
{
	event.Check(m_pView->GetMode() == LB_AddLinear);
}

void MainFrame::OnStructureEditLinear(wxCommandEvent &event)
{
	m_pView->SetMode(LB_EditLinear);
}

void MainFrame::OnUpdateStructureEditLinear(wxUpdateUIEvent& event)
{
	event.Check(m_pView->GetMode() == LB_EditLinear);
}

void MainFrame::OnStructureAddInstances(wxCommandEvent &event)
{
	m_pView->SetMode(LB_AddInstance);
}

void MainFrame::OnUpdateStructureAddInstances(wxUpdateUIEvent& event)
{
	event.Check(m_pView->GetMode() == LB_AddInstance);
}

void MainFrame::OnStructureAddFoundation(wxCommandEvent &event)
{
	vtStructureLayer *pSL = GetActiveStructureLayer();
	vtElevLayer *pEL = (vtElevLayer *) FindLayerOfType(LT_ELEVATION);
	pSL->AddFoundations(pEL);
}

void MainFrame::OnUpdateStructureAddFoundation(wxUpdateUIEvent& event)
{
	vtStructureLayer *pSL = GetActiveStructureLayer();
	vtElevLayer *pEL = (vtElevLayer *) FindLayerOfType(LT_ELEVATION);
	event.Enable(pSL != NULL && pEL != NULL);
}

void MainFrame::OnStructureConstrain(wxCommandEvent &event)
{
	m_pView->m_bConstrain = !m_pView->m_bConstrain;
}

void MainFrame::OnUpdateStructureConstrain(wxUpdateUIEvent& event)
{
	event.Check(m_pView->m_bConstrain);
}

void MainFrame::OnStructureSelectUsingPolygons(wxCommandEvent &event)
{
	vtStructureLayer *pStructureLayer = GetActiveStructureLayer();

	if (NULL != pStructureLayer)
	{
		pStructureLayer->DeselectAll();

		int iNumLayers = m_Layers.GetSize();
		for (int i = 0; i < iNumLayers; i++)
		{
			vtLayer *pLayer = m_Layers.GetAt(i);
			if (LT_RAW == pLayer->GetType())
			{
				vtRawLayer* pRawLayer = dynamic_cast<vtRawLayer*>(pLayer);
				if ((NULL != pRawLayer) && (wkbPolygon == wkbFlatten(pRawLayer->GetGeomType())))
				{
					vtFeatureSetPolygon *pFeatureSetPolygon = dynamic_cast<vtFeatureSetPolygon*>(pRawLayer->GetFeatureSet());
					if (NULL != pFeatureSetPolygon)
					{
						unsigned int iNumEntities = pFeatureSetPolygon->GetNumEntities();
						unsigned int iIndex;
						for (iIndex = 0; iIndex < iNumEntities; iIndex++)
						{
							if (pFeatureSetPolygon->IsSelected(iIndex))
							{
								unsigned int iIndex2;
								const DPolygon2 Polygon = pFeatureSetPolygon->GetPolygon(iIndex);
								unsigned int iNumStructures = pStructureLayer->GetSize();
								for (iIndex2 = 0; iIndex2 < iNumStructures; iIndex2++)
								{
									DRECT Extents;
									if (pStructureLayer->GetAt(iIndex2)->GetExtents(Extents))
									{
										DPoint2 Point((Extents.left + Extents.right)/2, (Extents.bottom + Extents.top)/2);
										if (Polygon.ContainsPoint(Point))
											pStructureLayer->GetAt(iIndex2)->Select(true);
									}
								}
							}
						}
					}
				}
			}
		}
		m_pView->Refresh();
	}
}

void MainFrame::OnUpdateStructureSelectUsingPolygons(wxUpdateUIEvent &event)
{
	bool bFoundSelectedPolygons = false;
	int iNumLayers = m_Layers.GetSize();
	for (int i = 0; i < iNumLayers; i++)
	{
		vtLayer *pLayer = m_Layers.GetAt(i);
		if (LT_RAW == pLayer->GetType())
		{
			vtRawLayer* pRawLayer = dynamic_cast<vtRawLayer*>(pLayer);
			if ((NULL != pRawLayer) && (wkbPolygon == wkbFlatten(pRawLayer->GetGeomType())))
			{
				vtFeatureSet *pFeatureSet = pRawLayer->GetFeatureSet();
				if ((NULL != pFeatureSet) && (pFeatureSet->NumSelected() > 0))
				{
					bFoundSelectedPolygons = true;
					break;
				}
			}
		}
	}
	event.Enable(bFoundSelectedPolygons);
}

void MainFrame::OnStructureColourSelectedRoofs(wxCommandEvent& event)
{
	vtStructureLayer *pLayer = GetActiveStructureLayer();
	if (!pLayer)
		return;

	wxColour Colour = wxGetColourFromUser(this);
	if (Colour.Ok())
	{
		RGBi RoofColour(Colour.Red(), Colour.Green(), Colour.Blue());
		for (unsigned int i = 0; i < pLayer->GetSize(); i++)
		{
			vtStructure *pStructure = pLayer->GetAt(i);
			if (!pStructure->IsSelected())
				continue;

			vtBuilding* pBuilding = pStructure->GetBuilding();
			if (pBuilding)
				pBuilding->GetLevel(pBuilding->GetNumLevels() - 1)->SetEdgeColor(RoofColour);
		}
	}
}

void MainFrame::OnStructureCleanFootprints(wxCommandEvent& event)
{
	vtStructureLayer *pLayer = GetActiveStructureLayer();
	if (!pLayer)
		return;

	double dEpsilon;
	if (m_proj.GetUnits() == LU_DEGREES)
		dEpsilon = 1E-7;
	else
		dEpsilon = 1E-2;

	wxString str;
	str.Printf(_T("%g"), dEpsilon);
	str = wxGetTextFromUser(_("How close are degenerate points? (epsilon)"),
		_("Clean Footprints"), str, this);
	if (str == _T(""))
		return;

	dEpsilon = atof(str.mb_str(wxConvUTF8));

	int degen = 0;
	int olap = 0;
	for (unsigned int i = 0; i < pLayer->GetSize(); i++)
	{
		vtStructure *pStructure = pLayer->GetAt(i);
		vtBuilding *bld = pStructure->GetBuilding();
		if (!bld)
			continue;
		for (unsigned int j = 0; j < bld->GetNumLevels(); j++)
		{
			vtLevel *lev = bld->GetLevel(j);
			DPolygon2 &dp = lev->GetFootprint();
			int rem = dp.RemoveDegeneratePoints(dEpsilon);
			degen += rem;

			// Also try to catch the case of the polygon looping around
			// over the same points more than once.
			for (unsigned int r = 0; r < dp.size(); r++)
			{
				DLine2 &ring = dp[r];
				for (unsigned int k2 = 1; k2 < ring.GetSize(); k2++)
				{
					DPoint2 &p2 = ring.GetAt(k2);
					for (unsigned int k1 = 0; k1 < k2; k1++)
					{
						DPoint2 &p1 = ring.GetAt(k1);
						DPoint2 diff = p1 - p2;
						if (fabs(diff.x) < dEpsilon && fabs(diff.y) < dEpsilon)
						{
							ring.RemoveAt(k2);
							k2--;
							olap++;
							rem++;
							break;
						}
					}
				}
			}
			if (rem)
			{
				// Must size down the edge arrays
				lev->ResizeEdgesToMatchFootprint();
			}
		}
	}
	if (degen)
		DisplayAndLog("%d degenerate points were removed.", degen);
	if (olap)
		DisplayAndLog("%d overlapping points were removed.", olap);

#if 0
	// useful test code for isolating problem buildings
	pLayer->DeselectAll();
	vtStructure *stru = pLayer->GetAt(6464);
	if (stru)
	{
		stru->Select(true);
		DRECT r;
		stru->GetExtents(r);
		m_pView->ZoomToRect(r, 0.1f);
	}
#endif
}

void MainFrame::OnUpdateStructureColourSelectedRoofs(wxUpdateUIEvent& event)
{
	event.Enable((NULL != GetActiveStructureLayer()) && (GetActiveStructureLayer()->NumSelected() > 0));
}

void MainFrame::OnStructureExportFootprints(wxCommandEvent& event)
{
	// Open File Save Dialog
	wxFileDialog saveFile(NULL, _("Export footprints to SHP"), _T(""), _T(""),
		_("SHP Files (*.shp)|*.shp"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (saveFile.ShowModal() == wxID_CANCEL)
		return;
	wxString strPathName = saveFile.GetPath();

	vtStructureLayer *pLayer = GetActiveStructureLayer();
	pLayer->WriteFootprintsToSHP(strPathName.mb_str(wxConvUTF8));
}

void MainFrame::OnStructureExportCanoma(wxCommandEvent& event)
{
	// Open File Save Dialog
	wxFileDialog saveFile(NULL, _("Export footprints to Canoma3DV"), _T(""), _T(""),
		_("Canoma3DV Files (*.3dv)|*.3dv"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (saveFile.ShowModal() == wxID_CANCEL)
		return;
	wxString strPathName = saveFile.GetPath();

	vtStructureLayer *pLayer = GetActiveStructureLayer();
	DRECT area;
	if (m_area.IsEmpty())
		// If the area tool isn't set, use the whole layer extents
		pLayer->GetExtents(area);
	else
		area = m_area;

	vtHeightField *pHF = NULL;
	vtElevLayer *pE = (vtElevLayer *)GetMainFrame()->FindLayerOfType(LT_ELEVATION);
	if (pE)
		pHF = pE->GetHeightField();

	pLayer->WriteFootprintsToCanoma3DV(strPathName.mb_str(wxConvUTF8), &area, pHF);
}

void MainFrame::OnUpdateStructureExportFootprints(wxUpdateUIEvent& event)
{
	event.Enable(NULL != GetActiveStructureLayer());
}


///////////////////////////////////
// Raw menu

void MainFrame::OnUpdateRawIsActive(wxUpdateUIEvent& event)
{
	vtRawLayer *pRL = GetActiveRawLayer();
	event.Enable(pRL != NULL);
}

void MainFrame::OnRawSetType(wxCommandEvent& event)
{
	static OGRwkbGeometryType types[5] = {
		wkbNone,
		wkbPoint,
		wkbPoint25D,
		wkbLineString,
		wkbPolygon
	};
	wxString choices[5];
	for (int i = 0; i < 5; i++)
		choices[i] = wxString::FromAscii(OGRGeometryTypeToName(types[i]));

	int n = 5;
	int cur_type = 0;

	wxSingleChoiceDialog dialog(this, _("Raw Layer Type"),
		_("Please indicate entity type:"), n, (const wxString *)choices);

	dialog.SetSelection(cur_type);

	if (dialog.ShowModal() == wxID_OK)
	{
		cur_type = dialog.GetSelection();
		vtRawLayer *pRL = (vtRawLayer *) GetActiveLayer();

		// must set the projection and layername again, as they are reset on
		//  setting geom type
		wxString name = pRL->GetLayerFilename();
		pRL->SetGeomType(types[cur_type]);
		pRL->SetProjection(m_proj);
		pRL->SetLayerFilename(name);
		RefreshTreeStatus();
	}
}

void MainFrame::OnUpdateRawSetType(wxUpdateUIEvent& event)
{
	vtRawLayer *pRL = GetActiveRawLayer();
	event.Enable(pRL != NULL && pRL->GetGeomType() == wkbNone);
}

void MainFrame::OnRawAddPoints(wxCommandEvent& event)
{
	m_pView->SetMode(LB_AddPoints);
}

void MainFrame::OnUpdateRawAddPoints(wxUpdateUIEvent& event)
{
	vtRawLayer *pRL = GetActiveRawLayer();
	event.Enable(pRL != NULL &&
		(pRL->GetGeomType() == wkbPoint ||
		 pRL->GetGeomType() == wkbPoint25D));
	event.Check(m_pView->GetMode() == LB_AddPoints);
}

void MainFrame::OnRawAddPointText(wxCommandEvent& event)
{
	wxString str = wxGetTextFromUser(_("(X, Y) in current projection"),
			_("Enter coordinate"));
	if (str == _T(""))
		return;
	double x, y;
	int num = sscanf(str.mb_str(wxConvUTF8), "%lf, %lf", &x, &y);
	if (num != 2)
		return;
	DPoint2 p(x, y);

	vtRawLayer *pRL = GetActiveRawLayer();
	pRL->AddPoint(p);
	m_pView->Refresh();
}

void MainFrame::OnUpdateRawAddPointText(wxUpdateUIEvent& event)
{
	vtRawLayer *pRL = GetActiveRawLayer();
	event.Enable(pRL != NULL && pRL->GetGeomType() == wkbPoint);
}

void MainFrame::OnRawAddPointsGPS(wxCommandEvent& event)
{
}

void MainFrame::OnUpdateRawAddPointsGPS(wxUpdateUIEvent& event)
{
//	vtRawLayer *pRL = GetActiveRawLayer();
//	event.Enable(pRL != NULL && pRL->GetEntityType() == SHPT_POINT);
	event.Enable(false);	// not implemented yet
}

void MainFrame::OnRawSelectCondition(wxCommandEvent& event)
{
	vtRawLayer *pRL = GetActiveRawLayer();
	vtFeatureSet *pFS = pRL->GetFeatureSet();

	if (!pFS || pFS->GetNumFields() == 0)
	{
		DisplayAndLog("Can't select by condition because the current\n"
					  "layer has no fields defined.");
		return;
	}
	SelectDlg dlg(this, -1, _("Select"));
	dlg.SetRawLayer(pRL);
	if (dlg.ShowModal() == wxID_OK)
	{
		wxString str = dlg.m_strValue;
		int selected = pFS->SelectByCondition(dlg.m_iField, dlg.m_iCondition,
			str.mb_str(wxConvUTF8));

		wxString msg;
		if (selected == -1)
			msg = _("Unable to select");
		else if (selected == 1)
			msg.Printf(_("Selected 1 entity"));
		else
			msg.Printf(_("Selected %d entities"), selected);
		SetStatusText(msg);

		msg += _T("\n");
		VTLOG1(msg.mb_str(wxConvUTF8));

		m_pView->Refresh(false);
		OnSelectionChanged();
	}
}

void MainFrame::OnRawConvertToTIN(wxCommandEvent& event)
{
	vtRawLayer *pRaw = GetActiveRawLayer();
	vtFeatureSetPoint3D *set = dynamic_cast<vtFeatureSetPoint3D *>(pRaw->GetFeatureSet());
	if (!set)
		return;

	vtTin2d *tin = new vtTin2d(set);
	vtElevLayer *pEL = new vtElevLayer;
	pEL->SetTin(tin);
	AddLayer(pEL);
	SetActiveLayer(pEL);

	m_pView->Refresh();
	RefreshTreeView();
}

void CapWords(vtString &str)
{
	bool bStart = true;
	for (int i = 0; i < str.GetLength(); i++)
	{
		char ch = str.GetAt(i);
		if (bStart)
			ch = toupper(ch);
		else
			ch = tolower(ch);
		str.SetAt(i, ch);

		if (ch == ' ')
			bStart = true;
		else
			bStart = false;
	}
}

void MainFrame::OnRawExportImageMap(wxCommandEvent& event)
{
	vtRawLayer *pRL = GetActiveRawLayer();
	if (!pRL)
		return;

	OGRwkbGeometryType type = pRL->GetGeomType();
	if (type != wkbPolygon)
		return;

	// First grab image
	BuilderView *view = GetView();
	int xsize, ysize;
	view->GetClientSize(&xsize, &ysize);

	wxClientDC dc(view);
	view->PrepareDC(dc);

	vtDIB dib;
	dib.Create(xsize, ysize, 24);

	int x, y;
	int xx, yy;
	wxColour color;
	for (x = 0; x < xsize; x++)
	{
		for (y = 0; y < ysize; y++)
		{
			view->CalcUnscrolledPosition(x, y, &xx, &yy);
			dc.GetPixel(xx, yy, &color);
			dib.SetPixel24(x, y, RGBi(color.Red(), color.Green(), color.Blue()));
		}
	}

	vtFeatureSet *fset = pRL->GetFeatureSet();

	ImageMapDlg dlg(this, -1, _("Export Image Map"));
	dlg.SetFields(fset);
	if (dlg.ShowModal() != wxID_OK)
		return;

	wxFileDialog loadFile(NULL, _("Save to Image File"), _T(""), _T(""),
		FSTRING_PNG, wxFD_SAVE);
	if (loadFile.ShowModal() != wxID_OK)
		return;
	vtString fullname = (const char *) loadFile.GetPath().mb_str(wxConvUTF8);
	vtString filename = (const char *) loadFile.GetFilename().mb_str(wxConvUTF8);

	dib.WritePNG(fullname);

	// Then write imagemap
	wxFileDialog loadFile2(NULL, _("Save to Image File"), _T(""), _T(""),
		FSTRING_HTML, wxFD_SAVE);
	if (loadFile2.ShowModal() != wxID_OK)
		return;
	vtString htmlname = (const char *) loadFile2.GetPath().mb_str(wxConvUTF8);

	FILE *fp = vtFileOpen(htmlname, "wb");
	if (!fp)
		return;
	fprintf(fp, "<html>\n");
	fprintf(fp, "<body>\n");
	fprintf(fp, "<map name=\"ImageMap\">\n");

	vtFeatureSetPolygon *polyset = (vtFeatureSetPolygon *) fset;
	unsigned int i, num = polyset->GetNumEntities();
	wxPoint sp;		// screen point

	for (i = 0; i < num; i++)
	{
		vtString str;
		polyset->GetValueAsString(i, dlg.m_iField, str);

//		CapWords(str);
//		str += ".sid";

		fprintf(fp, "<area href=\"%s\" shape=\"polygon\" coords=\"",
			(const char *) str);

		const DPolygon2 &poly = polyset->GetPolygon(i);

		DLine2 dline;
		poly.GetAsDLine2(dline);

		unsigned int j, points = dline.GetSize();
		for (j = 0; j < points; j++)
		{
			DPoint2 p = dline[j];
			if (j == points-1 && p == dline[0])
				continue;

			if (j > 0)
				fprintf(fp, ", ");

			view->screen(p, sp);
			view->CalcScrolledPosition(sp.x, sp.y, &xx, &yy);

			fprintf(fp, "%d, %d", xx, yy);
		}
		fprintf(fp, "\">\n");
	}
	fprintf(fp, "</map>\n");
	fprintf(fp, "<img border=\"0\" src=\"%s\" usemap=\"#ImageMap\" width=\"%d\" height=\"%d\">\n",
		(const char *) filename, xsize, ysize);
	fprintf(fp, "</body>\n");
	fprintf(fp, "</html>\n");
	fclose(fp);
}

void MainFrame::OnRawExportKML(wxCommandEvent& event)
{
	vtRawLayer *pRL = GetActiveRawLayer();
	if (!pRL)
		return;

	vtFeatureSet *fset = pRL->GetFeatureSet();

	//ImageMapDlg dlg(this, -1, _("Export Image Map"));
	//dlg.SetFields(fset);
	//if (dlg.ShowModal() != wxID_OK)
	//	return;

	wxFileDialog loadFile(NULL, _("Save to KML File"), _T(""), _T(""),
		FSTRING_KML, wxFD_SAVE);
	if (loadFile.ShowModal() != wxID_OK)
		return;
	fset->SaveToKML(loadFile.GetPath().mb_str(wxConvUTF8));
}

void MainFrame::OnRawGenElevation(wxCommandEvent& event)
{
	vtRawLayer *pRL = GetActiveRawLayer();
	if (!pRL)
		return;

	vtFeatureSet *pSet = pRL->GetFeatureSet();
	DRECT extent;
	pSet->ComputeExtent(extent);

	bool bIsGeo = (m_proj.IsGeographic() != 0);

	GenGridDlg dlg(this, -1, _("Generate Grid from 3D Points"), bIsGeo);
	dlg.m_fAreaX = extent.Width();
	dlg.m_fAreaY = extent.Height();
	dlg.m_iSizeX = 512;
	dlg.m_iSizeY = 512;
	dlg.RecomputeSize();
	dlg.m_fDistanceCutoff = 1.5f;

	int ret = dlg.ShowModal();
	if (ret == wxID_CANCEL)
		return;

	vtElevLayer *el = new vtElevLayer;

	OpenProgressDialog(_T("Creating Grid"));
	int xsize = 800;
	int ysize = 300;
	if (el->CreateFromPoints(pSet, dlg.m_iSizeX, dlg.m_iSizeY,
			dlg.m_fDistanceCutoff))
		AddLayerWithCheck(el);
	else
		delete el;
	CloseProgressDialog();
}

void MainFrame::OnUpdateRawGenElevation(wxUpdateUIEvent& event)
{
	vtRawLayer *pRL = GetActiveRawLayer();
	event.Enable(pRL != NULL && pRL->GetGeomType() == wkbPoint25D);
}

void MainFrame::OnRawStyle(wxCommandEvent& event)
{
	vtRawLayer *pRL = GetActiveRawLayer();

	DrawStyle style = pRL->GetDrawStyle();
	style.m_LineColor.Set(0,0,0);
	pRL->SetDrawStyle(style);
}

void MainFrame::OnRawScale(wxCommandEvent& event)
{
	vtRawLayer *pRL = GetActiveRawLayer();

	wxString str = _T("1");
	str = wxGetTextFromUser(_("Scale factor?"),	_("Scale Raw Layer"),
		str, this);
	if (str == _T(""))
		return;

	double value = atof(str.mb_str(wxConvUTF8));
	pRL->Scale(value);
	m_pView->Refresh();
}


////////////////////
// Help

void MainFrame::OnHelpAbout(wxCommandEvent &event)
{
	wxString str = _("Virtual Terrain Builder\nPowerful, easy to use, free!\n");
	str += _T("\n");
	str += _("Please read the HTML documentation and license.\n");
	str += _T("\n");
	str += _("Send feedback to: ");
	str += _T("ben@vterrain.org\n");
	str += _("Build date: ");
	str += wxString(__DATE__, wxConvUTF8);
	str += _T("\n");

#if defined(_MSC_VER) && defined(_DEBUG)	// == 1300 for VC7.1
	_CrtMemState state;
	_CrtMemCheckpoint(&state);
	int iAllocated = state.lSizes[1] + state.lSizes[2];
	wxString str3;
	str3.Printf(_T("Memory in use: %d bytes (%.0fK, %.1fMB)"), iAllocated,
		(float)iAllocated/1024, (float)iAllocated/1024/1024);
	str += _T("\n");
	str += str3;
#endif

	wxString str2 = _("About ");
	str2 += wxString(APPNAME, wxConvUTF8);
	wxMessageBox(str, str2);
}

void MainFrame::OnHelpDocLocal(wxCommandEvent &event)
{
	vtString local_lang_code = (const char *) wxGetApp().GetLanguageCode().mb_str(wxConvUTF8);
	local_lang_code = local_lang_code.Left(2);

	// Launch default web browser with documentation pages
	LaunchAppDocumentation("VTBuilder", local_lang_code);
}

void MainFrame::OnHelpDocOnline(wxCommandEvent &event)
{
	// Launch default web browser with documentation pages
	wxLaunchDefaultBrowser(_T("http://vterrain.org/Doc/VTBuilder/"));
}

////////////////////
// Popup menu items

void MainFrame::OnDistanceClear(wxCommandEvent &event)
{
	ClearDistance();
}

void MainFrame::OnShowAll(wxCommandEvent& event)
{
	int layers = m_Layers.GetSize();
	for (int l = 0; l < layers; l++)
	{
		vtLayer *lp = m_Layers.GetAt(l);
		if (lp->GetType() == m_pTree->m_clicked_layer_type)
		{
			lp->SetVisible(true);
			RefreshLayerInView(lp);
		}
	}
	RefreshTreeStatus();
}

void MainFrame::OnHideAll(wxCommandEvent& event)
{
	int layers = m_Layers.GetSize();
	for (int l = 0; l < layers; l++)
	{
		vtLayer *lp = m_Layers.GetAt(l);
		if (lp->GetType() == m_pTree->m_clicked_layer_type)
		{
			lp->SetVisible(false);
			RefreshLayerInView(lp);
		}
	}
	RefreshTreeStatus();
}

void MainFrame::OnLayerPropsPopup(wxCommandEvent& event)
{
	wxTreeItemId itemId = m_pTree->GetSelection();
	MyTreeItemData *data = (MyTreeItemData *)m_pTree->GetItemData(itemId);
	if (!data)
		return;
	ShowLayerProperties(data->m_pLayer);
}

void MainFrame::OnLayerToTop(wxCommandEvent& event)
{
	wxTreeItemId itemId = m_pTree->GetSelection();
	MyTreeItemData *data = (MyTreeItemData *)m_pTree->GetItemData(itemId);
	if (!data)
		return;

	int num = LayerNum(data->m_pLayer);
	if (num != 0)
	{
		SwapLayerOrder(0, num);
		RefreshLayerInView(data->m_pLayer);
		RefreshTreeView();
	}
}

void MainFrame::OnLayerToBottom(wxCommandEvent& event)
{
	wxTreeItemId itemId = m_pTree->GetSelection();
	MyTreeItemData *data = (MyTreeItemData *)m_pTree->GetItemData(itemId);
	if (!data)
		return;

	int total = NumLayers();
	int num = LayerNum(data->m_pLayer);
	if (num != total-1)
	{
		SwapLayerOrder(num, total-1);
		RefreshLayerInView(data->m_pLayer);
		RefreshTreeView();
	}
}

void MainFrame::OnLayerOverviewDisk(wxCommandEvent& event)
{
	wxTreeItemId itemId = m_pTree->GetSelection();
	MyTreeItemData *data = (MyTreeItemData *)m_pTree->GetItemData(itemId);
	if (!data)
		return;
	vtImageLayer *pIL = (vtImageLayer *) data->m_pLayer;

	OpenProgressDialog(_("Creating Overviews"), false, this);

	pIL->GetImage()->CreateOverviews();

	CloseProgressDialog();
}

void MainFrame::OnLayerOverviewMem(wxCommandEvent& event)
{
	wxTreeItemId itemId = m_pTree->GetSelection();
	MyTreeItemData *data = (MyTreeItemData *)m_pTree->GetItemData(itemId);
	if (!data)
		return;
	vtImageLayer *pIL = (vtImageLayer *) data->m_pLayer;

	OpenProgressDialog(_("Creating MipMaps"), false, this);

	pIL->GetImage()->AllocMipMaps();
	pIL->GetImage()->DrawMipMaps();

	CloseProgressDialog();
}

