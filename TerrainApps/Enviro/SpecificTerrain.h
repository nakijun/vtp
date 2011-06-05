//
// SpecificTerrain.h
//

#ifndef SPECIFICTERRAINH
#define SPECIFICTERRAINH

#include "vtlib/core/Terrain.h"

class Romania : public vtTerrain
{
public:
	void CreateCustomCulture();
};

class TransitTerrain : public vtTerrain
{
public:
	void CreateCustomCulture();
	vtGeode *MakeBlockGeom(FPoint3 size);
};

#endif	// SPECIFICTERRAINH

