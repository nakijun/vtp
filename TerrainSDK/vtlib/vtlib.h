//
// Main header for VT library, for all platforms
//
// Copyright (c) 2001-2007 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#ifdef _MSC_VER
#pragma warning(disable: 4786)	// prevent common warning about templates
#endif

// frequently used standard headers
#include <stdio.h>

// vtdata headers
#include "vtdata/config_vtdata.h"
#include "vtdata/MathTypes.h"

// OpenGL headers
#if WIN32
  // Some scene graph libraries will need the full windows.h header on Win32
  #if VTLIB_PLIB || VTLIB_NI || VTLIB_OPENSG
	#include <windows.h>
	#undef min
	#undef max
  #else
	// these definitions let us include gl.h without the entire Windows headers
	#include "core/winglue.h"
  #endif
#endif

#if !VTLIB_PSM
  #if !VTLIB_NI
	#ifdef __DARWIN_OSX__
	  #include <OpenGL/gl.h>
	#else
	  #include <GL/gl.h>
	#endif
  #endif
//
// Base classes from which the API-specific implementation
// classes are derived.
//
  #include "core/Base.h"
#endif

//
// API-specific headers
//

////////////////////////////// PSM ///////////////////////////////
#if VTLIB_PSM
	#define PV_OGL

	#include "psm.h"
	#include "psutil.h"

	#include <GL/gl.h>

	#include "core/Base.h"
	#include "vtpsm/MathPSM.h"
	#include "vtpsm/ImagePSM.h"
	#include "vtpsm/MeshMatPSM.h"
	#include "vtpsm/NodePSM.h"
	#include "vtpsm/ScenePSM.h"

  #ifdef _MSC_VER
	#if _DEBUG
		#pragma comment(lib, "freeimaged.lib")
	#else
		#pragma comment(lib, "freeimage.lib")
	#endif
	#pragma comment(lib, "winmm.lib")
	#pragma comment(lib, "wininet.lib")
	#pragma comment(lib, "user32.lib")
  #endif
#endif	// PSM

////////////////////////////// PLIB/SSG ////////////////////////////
#if VTLIB_PLIB
	#include "ssg.h"

	#include "vtplib/ImageSSG.h"
	#include "vtplib/MeshMat.h"
	#include "vtplib/NodeSSG.h"
	#include "vtplib/SceneSSG.h"
  #ifdef _MSC_VER
	#pragma comment(lib, "sg.lib")
	#pragma comment(lib, "ssg.lib")
	#pragma comment(lib, "ul.lib")
  #endif
#endif	// PLIB

///////////////////////////////// OSG //////////////////////////////
#if VTLIB_OSG
	#ifdef _MSC_VER
	  #pragma warning(disable: 4511) // turn off warning about copy constructors
	  #pragma warning(disable: 4512) // turn off warning about assignment operators
	#endif
	#include "vtosg/ImageOSG.h"
	#include "vtosg/MathOSG.h"
	#include "vtosg/MeshMat.h"
	#include "vtosg/NodeOSG.h"
	#include "vtosg/SceneOSG.h"
#endif // OSG

///////////////////////////////// OPENSG //////////////////////////////
#if VTLIB_OPENSG
	#include "GL/glext.h"
	#ifdef _MSC_VER
	  #pragma warning(disable: 4251) // turn off warning about std::map dll-interface
	  #pragma warning(disable: 4275) // turn off warning about subclass dll-interface
	  #pragma warning(disable: 4231) // turn off warning about nonstandard 'extern'
	#endif
	#include "vtopensg/ImageOSG.h"
	#include "vtopensg/MathOSG.h"
	#include "vtopensg/MeshMat.h"
	#include "vtopensg/NodeOSG.h"
	#include "vtopensg/SceneOSG.h"
#endif // OSG

///////////////////////////////// NI //////////////////////////////
#if VTLIB_NI
	#ifdef _MSC_VER
//	  #pragma warning(disable: 4511) // turn off warning
	#endif
	#include "vtni/ImageNI.h"
	#include "vtni/MathNI.h"
	#include "vtni/MeshMatNI.h"
	#include "vtni/NodeNI.h"
	#include "vtni/SceneNI.h"

	#include <limits>
	#include <map>

	#define glBegin(x)
	#define glEnd()
	#define glVertex3f(x,y,z)
	#define glTexParameteri(a,b,c)
	#define glPolygonOffset(x,y)
	#define glEnable(x)
	#define glDisable(x)
	#define glColor4f(a,b,c,d)
	#define glPushMatrix()
	#define glScalef(x,y,z)
	#define glPopMatrix()
	#define glFrontFace(x)
#endif // OSG

/////////////////////////////// SGL ////////////////////////////
#if VTLIB_SGL
	#include <sgl/sgl.h>
	#include <sglu/sglu.h>
	#include <sgl/sglVector.hpp>
	#include <sgl/sglMatrix.hpp>
	#include <sgl/sglPlane.hpp>

	#include <sgl/sglBillboard.hpp>
	#include <sgl/sglScene.hpp>
	#include <sgl/sglGeode.hpp>
	#include <sgl/sglMonoIndexedLineSet.hpp>
	#include <sgl/sglMonoIndexedLineStripSet.hpp>
	#include <sgl/sglMonoIndexedPointSet.hpp>
	#include <sgl/sglMonoIndexedQuadSet.hpp>
	#include <sgl/sglMonoIndexedQuadStripSet.hpp>
	#include <sgl/sglMonoIndexedTriangleFanSet.hpp>
	#include <sgl/sglMonoIndexedTriangleSet.hpp>
	#include <sgl/sglMonoIndexedTriangleStripSet.hpp>
	#include <sgl/sglTexture2D.hpp>
	#include <sgl/sglTexEnv.hpp>
	#include <sgl/sglTexGen.hpp>
	#include <sgl/sglTransform.hpp>
	#include <sgl/sglLineSet.hpp>
	#include <sgl/sglLOD.hpp>
	#include <sgl/sglMaterial.hpp>
	#include <sgl/sglMat4.hpp>
	#include <sgl/sglViewPlatform.hpp>
	#include <sgl/sglDirectionalLight.hpp>
	#include <sgl/sglView.hpp>

	#include "vtsgl/ImageSGL.h"
	#include "vtsgl/MathSGL.h"
	#include "vtsgl/MeshMat.h"
	#include "vtsgl/NodeSGL.h"
	#include "vtsgl/SceneSGL.h"
#endif	// SGL

#if defined(_MSC_VER) && _DEBUG && !defined(_STDAFX_H_INCLUDED_) && 0
#pragma message("(Including MemTracker)")
#include "vtlib/core/MemoryTracker.h"
#endif

