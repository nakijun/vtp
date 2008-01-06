//
// CoreMeshMat.cpp
//
// Mesh and Material classes and methods which are a core part of the vtlib
// library.
//
// Copyright (c) 2001-2007 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#include "vtlib/vtlib.h"

static FPoint3 c[8] =
{
	FPoint3(-1.0, -1.0, 1.0),	/* 0 */
	FPoint3(-1.0, 1.0, 1.0),	/* 1 */
	FPoint3(1.0, 1.0, 1.0),		/* 2 */
	FPoint3(1.0, -1.0, 1.0),	/* 3 */
	FPoint3(-1.0, -1.0, -1.0),	/* 4 */
	FPoint3(-1.0, 1.0, -1.0),	/* 5 */
	FPoint3(1.0, 1.0, -1.0),	/* 6 */
	FPoint3(1.0, -1.0, -1.0),	/* 7 */
};

static FPoint3 c2[8] =
{
	FPoint3(-0.5, 0, 0.5)	,	/* 0 */
	FPoint3(-0.5, 1.0, 0.5),	/* 1 */
	FPoint3(0.5, 1.0, 0.5),		/* 2 */
	FPoint3(0.5, 0, 0.5),		/* 3 */
	FPoint3(-0.5, 0, -0.5),		/* 4 */
	FPoint3(-0.5, 1.0, -0.5),	/* 5 */
	FPoint3(0.5, 1.0, -0.5),	/* 6 */
	FPoint3(0.5, 0, -0.5),		/* 7 */
};

static FPoint3 n[6] =
{
	FPoint3( 0.0, 0.0, 1.0 ),
	FPoint3( -1.0, 0.0, 0.0 ),
	FPoint3( 0.0, 0.0, -1.0 ),
	FPoint3( 1.0, 0.0, 0.0 ),
	FPoint3( 0.0, -1.0, 0.0 ),
	FPoint3( 0.0, 1.0, 0.0 )
};

static int v_list[6][4] = {
	7, 4, 5, 6,		 /* back */
	3, 7, 6, 2,		 /* right */
	0, 3, 2, 1,		 /* front */
	4, 0, 1, 5,		 /* left */
	6, 5, 1, 2,		 /* top */
	4, 7, 3, 0,		 /* bottom */
};

static FPoint2 t[4] =
{
	FPoint2(0.0, 1.0),
	FPoint2(1.0, 1.0),
	FPoint2(1.0, 0.0),
	FPoint2(0.0, 0.0)
};

/**
 * Computes the bounding box of the mesh, based on the extent of its
 * vertices.  Result is placed in the 'box' parameter.  If there
 * are no vertices, then 'box' is unchanged.
 */
void vtMeshBase::GetBoundBox(FBox3 &box) const
{
	int num = GetNumVertices();
	if (num == 0)
		return;

	box.min.Set(1E8, 1E8, 1E8);
	box.max.Set(-1E8, -1E8, -1E8);

	for (int i = 0; i < num; i++)
	{
		FPoint3 p = GetVtxPos(i);

		if (p.x < box.min.x) box.min.x = p.x;
		if (p.y < box.min.y) box.min.y = p.y;
		if (p.z < box.min.z) box.min.z = p.z;

		if (p.x > box.max.x) box.max.x = p.x;
		if (p.y > box.max.y) box.max.y = p.y;
		if (p.z > box.max.z) box.max.z = p.z;
	}
}

/**
 * Adds a vertex to the mesh.
 *
 * \return The index of the vertex that was added.
 */
int vtMeshBase::AddVertex(float x, float y, float z)
{
	int i = GetNumVertices();
	SetVtxPos(i, FPoint3(x, y, z));
	return i;
}

/**
 * Adds a vertex to the mesh, with a vertex normal.
 *
 * \return The index of the vertex that was added.
 */
int vtMeshBase::AddVertexN(float x, float y, float z, float nx, float ny, float nz)
{
	int i = GetNumVertices();
	SetVtxPos(i, FPoint3(x, y, z));
	SetVtxNormal(i, FPoint3(nx, ny, nz));
	return i;
}

/**
 * Adds a vertex to the mesh, with UV coordinates.
 *
 * \return The index of the vertex that was added.
 */
int vtMeshBase::AddVertexUV(float x, float y, float z, float u, float v)
{
	int i = GetNumVertices();
	SetVtxPos(i, FPoint3(x, y, z));
	SetVtxTexCoord(i, FPoint2(u, v));
	return i;
}

/**
 * Adds a vertex to the mesh.
 *
 * \return The index of the vertex that was added.
 */
int vtMeshBase::AddVertex(const FPoint3 &p)
{
	int i = GetNumVertices();
	SetVtxPos(i, p);
	return i;
}

/**
 * Adds a vertex to the mesh, with a vertex normal.
 *
 * \return The index of the vertex that was added.
 */
int vtMeshBase::AddVertexN(const FPoint3 &p, const FPoint3 &n)
{
	int i = GetNumVertices();
	SetVtxPos(i, p);
	SetVtxNormal(i, n);
	return i;
}

/**
 * Adds a vertex to the mesh, with UV coordinates.
 *
 * \return The index of the vertex that was added.
 */
int vtMeshBase::AddVertexUV(const FPoint3 &p, float u, float v)
{
	int i = GetNumVertices();
	SetVtxPos(i, p);
	SetVtxTexCoord(i, FPoint2(u, v));
	return i;
}

/**
 * Adds a vertex to the mesh, with UV coordinates.
 *
 * \return The index of the vertex that was added.
 */
int vtMeshBase::AddVertexUV(const FPoint3 &p, const FPoint2 &uv)
{
	int i = GetNumVertices();
	SetVtxPos(i, p);
	SetVtxTexCoord(i, uv);
	return i;
}

/**
 * Adds a vertex to the mesh, with a vertex normal and UV coordinates.
 *
 * \return The index of the vertex that was added.
 */
int vtMeshBase::AddVertexNUV(const FPoint3 &p, const FPoint3 &n, const FPoint2 &uv)
{
	int i = GetNumVertices();
	SetVtxPos(i, p);
	SetVtxNormal(i, n);
	SetVtxTexCoord(i, uv);
	return i;
}

/**
 * Adds an indexed strip to the mesh, with the assumption that the indices
 * are in linear order.
 *
 * \param iNVerts The number of vertices in the strip.
 * \param iStartIndex The index that starts the linear sequence.
 */
void vtMeshBase::AddStrip2(int iNVerts, int iStartIndex)
{
	unsigned short *idx = new unsigned short[iNVerts];

	for (int i = 0; i < iNVerts; i++)
		idx[i] = iStartIndex + i;

	AddStrip(iNVerts, idx);
	delete [] idx;
}

/**
 * Add a block (rectangular 3d box) to this mesh.  The width, height and
 * depth are specified with the 'size' parameter.
 */
void vtMeshBase::CreateBlock(const FPoint3& size)
{
	int i, j;
	FPoint3 vtx[8];			/* individual vertex values */

	for (i = 0; i < 8; i++)
	{
		vtx[i].x = size.x * c[i].x;
		vtx[i].y = size.y * c[i].y;
		vtx[i].z = size.z * c[i].z;
	}

	int vidx = 0;
	for (i = 0; i < 6; i++)
	{
		int start = vidx;
		for (j = 0; j < 4; j++)
		{
			AddVertex(vtx[v_list[i][j]]);
			if (GetVtxType() & VT_Normals)		/* compute normals */
				SetVtxNormal(vidx, -n[i]);
			if (GetVtxType() & VT_TexCoords)	/* compute tex coords */
				SetVtxTexCoord(vidx, t[j]);
			vidx++;
		}
		AddFan(start, start+1, start+2, start+3);
	}
}

/**
 *	Adds a 3D block to a vtMesh as a series of 5 triangle fan primitives.
 *  The bottom face is omitted, the base is placed at y=0, and texture
 *  coordinates are provided such that texture bitmaps appear right-side-up
 *  on the side faces.
 */
void vtMeshBase::CreateOptimizedBlock(const FPoint3& size)
{
	int i, j;
	FPoint3 vtx[8];			/* individual vertex values */
	FPoint3 half = size / 2.0f;

	for (i = 0; i < 8; i++)
	{
		vtx[i].x = half.x * c[i].x;
		vtx[i].y = half.y * c[i].y + (half.y);
		vtx[i].z = half.z * c[i].z;
	}

	int vidx = 0;
	for (i = 0; i < 5; i++)
	{
		int start = vidx;
		for (j = 0; j < 4; j++)
		{
			AddVertex(vtx[v_list[i][j]]);
			if (GetVtxType() & VT_Normals)		/* compute normals */
				SetVtxNormal(vidx, -n[i]);
			if (GetVtxType() & VT_TexCoords)	/* compute tex coords */
				SetVtxTexCoord(vidx, t[j]);
			vidx++;
		}
		AddFan(start, start+1, start+2, start+3);
	}
}

/**
 * Adds a 3D block (extruded rectangle) to a vtMesh as a series of 5 triangle
 * fan primitives.  The bottom face is omitted, the base is placed at /base/,
 * the extrusion is along /vector_up/.  Texture coordinates are provided such
 * that texture bitmaps appear right-side-up on the side faces.
 */
void vtMeshBase::CreatePrism(const FPoint3 &base, const FPoint3 &vector_up,
							 const FPoint2 &size1, const FPoint2 &size2)
{
	int i, j;
	FPoint3 vtx[8];		/* individual vertex values */

	for (i = 0; i < 8; i++)
	{
		vtx[i].y = base.y + c2[i].y * (vector_up.y);
		if (c2[i].y == 0)	// bottom
		{
			vtx[i].x = base.x + size1.x * c2[i].x;
			vtx[i].z = base.z + size1.y * c2[i].z;
		}
		else
		{
			vtx[i].x = base.x + vector_up.x + size2.x * c2[i].x;
			vtx[i].z = base.z + vector_up.z + size2.y * c2[i].z;
		}
	}

	int vidx = GetNumVertices();
	for (i = 0; i < 5; i++)
	{
		int start = vidx;
		for (j = 0; j < 4; j++)
		{
			AddVertex(vtx[v_list[i][j]]);
			if (GetVtxType() & VT_Normals)		/* compute normals */
				SetVtxNormal(vidx, -n[i]);
			if (GetVtxType() & VT_TexCoords)	/* compute tex coords */
				SetVtxTexCoord(vidx, t[j]);
			vidx++;
		}
		AddFan(start, start+1, start+2, start+3);
	}
}

/**
 * Adds triangle/quad strips to this mesh, suitable for a (topologically)
 * rectangular grid.
 *
 * \param xsize Number of vertices in the first dimension.
 * \param ysize Number of vertices in the second dimension.
 */
void vtMeshBase::CreateRectangularMesh(int xsize, int ysize, bool bReverseNormals)
{
	int i, j;

	unsigned short *strip = new unsigned short[xsize*2];
	for (j = 0; j < ysize - 1; j++)
	{
		int start = j * xsize;
		int len = 0;
		for (i = 0; i < xsize; i++)
		{
			if (bReverseNormals)
			{
				// reverse the vertex order so thte normals point the other way
				strip[len++] = start + i + xsize;
				strip[len++] = start + i;
			}
			else
			{
				strip[len++] = start + i;
				strip[len++] = start + i + xsize;
			}
		}
		AddStrip(xsize*2, strip);
	}
	delete [] strip;
}

/**
 * Adds geometry for an ellipsoid to this mesh.
 *
 * The geometry is created with efficient triangle strips.  If the mesh
 * has vertex normals, outward-pointing normals are created for lighting.
 * If the mesh has vertex coordinates, then UVs are set as follows:
 * U ranges from 0 to 1 around the circumference, and V ranges from 0
 * to 1 from the top to the bottom.  For a hemisphere, V ranges from
 * 0 at the top to 1 at the base.
 *
 * \param center Position of the center, pass FPoint3(0,0,0) to center on the origin.
 * \param size The width, height and depth of the ellipsoid.
 * \param res The resolution (number of quads used in the tesselation)
 *		from top to bottom (north pole to south pole).
 * \param hemi Create only the top of the ellipsoid (e.g. a hemisphere).
 * \param bNormalsIn Use a vertex order in the mesh so that the normals point
 *		in, instead of out.  This is useful for, example, a backface-culled
 *		sphere that you want to see from the inside, instead of the outside.
 */
void vtMeshBase::CreateEllipsoid(const FPoint3 &center, const FPoint3 &size,
								 int res, bool hemi, bool bNormalsIn)
{
	int		i, j;
	int		vidx;
	FPoint3 v;
	float	theta_step, phi_step;
	int		theta_res, phi_res;
	float	phi_range;

	if (hemi)
	{
		phi_res = res/2;
		phi_range = PID2f;
	}
	else
	{
		phi_res = res;
		phi_range = PIf;
	}
	theta_res = res * 2;

	theta_step = PI2f / theta_res;
	phi_step = phi_range / phi_res;
	for (j = 0; j <= phi_res; j++)
	{
		float phi = j * phi_step;

		for (i = 0; i <= theta_res; i++)
		{
			float theta = i * theta_step;
			v.x = sinf(theta) * sinf(phi);
			v.y = cosf(phi);
			v.z = cosf(theta) * sinf(phi);

			FPoint3 p(size.x * v.x, size.y * v.y, size.z * v.z);
			vidx = AddVertex(center + p);
			if (GetVtxType() & VT_Normals)		/* compute normals */
				SetVtxNormal(vidx, v);
			if (GetVtxType() & VT_TexCoords)	/* compute tex coords */
			{
				FPoint2 t((float)i / theta_res, 1.0f - (float)j / phi_res);
				SetVtxTexCoord(vidx, t);
			}
		}
	}
	CreateRectangularMesh(theta_res+1, phi_res+1, bNormalsIn);
}

/**
 * Adds an cylinder to this mesh.
 *
 * \param height The height of the cylinder.
 * \param radius The radius of the cylinder.
 * \param res The resolution (number of side of the cylinder).
 * \param bTop True to create the top of the cylinder.
 * \param bBottom True to create the bottom of the cylinder.  You could set
 *		this to false, for example, if the cylinder is going to sit on a
 *		flat surface where you will never see its bottom.
 * \param bCentered True to create a cylinder centered around its origin,
 *		false for a cylinder with its base at the origin that extends outward.
 * \param direction An orientation, 0-2 corresponds to X, Y, Z.  Default is 1 (Y).
 */
void vtMeshBase::CreateCylinder(float height, float radius, int res,
								bool bTop, bool bBottom, bool bCentered, int direction)
{
	// One way to model a cylinder is as a triangle/quad strip and
	// a pair of triangle fans for the top and bottom, although that
	// requires 2 kinds of primitive, so we would need 2 vtMesh objects.
	// Instead, we use triangle strips for both the sides and top/bottom.

	// In order to do vertex-based shading, we will have to provide correct
	// vertex normals for the sides and top/bottom separately.  Unfortunately
	// this doubles the number of vertices, since each vertex can only
	// have a single normal.  So we may have to make 2 passes through vertex
	// construction.

	int		a, b, passes;
	int		i, j, k;
	int		vidx;
	FPoint3 p, norm;

	float	theta_step = PI2f / res;
	int		verts_per_pass = res * 2;

	if ((bTop || bBottom) && (GetVtxType() & VT_Normals))
		passes = 2;
	else
		passes = 1;

	for (a = 0; a < passes; a++)
	{
		for (b = 0; b < 2; b++)
		{
			if (bCentered)
				p.y = b ? height/2 : -height/2;
			else
				p.y = b ? height : 0;

			for (i = 0; i < res; i++)
			{
				float theta = i * theta_step;
				p.x = cosf(theta) * radius;
				p.z = sinf(theta) * radius;
				vidx = AddVertex(p);

				if (GetVtxType() & VT_Normals)	/* compute normals */
				{
					if (a == 0)		// first pass, outward normals for sides
					{
						norm.x = cosf(theta);
						norm.y = 0;
						norm.z = sinf(theta);
					}
					else if (a == 1)	// second pass, top/bottom normals
					{
						norm.x = 0;
						norm.y = b ? 1.0f : -1.0f;
						norm.z = 0;
					}
					SetVtxNormal(vidx, norm);
				}
			}
		}
	}
	// Create sides
	unsigned short *indices = new unsigned short[(res+1) * 2];
	j = 0;
	k = 0;
	for (i = 0; i < res+1; i++)
	{
		indices[j++] = k;
		indices[j++] = k+res;
		k++;
		if (k == res)
			k = 0;
	}
	AddStrip(j, indices);

	// create top and bottom
	int offset = (passes == 2 ? verts_per_pass : 0);
	if (bBottom)
	{
		j = 0;
		for (i = 0; i < res/2; i++)
		{
			indices[j++] = offset + i;
			k = res - 1 - i;
			if (i >= k)
				break;
			indices[j++] = offset + k;
		}
		AddStrip(j, indices);
	}

	if (bTop)
	{
		j = 0;
		for (i = 0; i < res/2; i++)
		{
			indices[j++] = offset + res + i;
			k = res - 1 - i;
			if (i >= k)
				break;
			indices[j++] = offset + res + k;
		}
		AddStrip(j, indices);
	}
	delete indices;
}

void vtMeshBase::CreateTetrahedron(const FPoint3 &center, float fRadius)
{
	float A = fRadius * 1.632993161858f;
	float B = fRadius;
	float D = fRadius * 0.333333333333f;
	float F = fRadius * 0.9428090415834f;
	float G = fRadius * 0.4714045207904f;
	FPoint3 p0(-A/2, -D, G);
	FPoint3 p1( A/2, -D, G);
	FPoint3 p2( 0, B, 0);
	FPoint3 p3( 0, -D, -F);

	p0 += center;
	p1 += center;
	p2 += center;
	p3 += center;

	int vidx;
	if (m_iVtxType & VT_Normals)
	{
		// We need distinct vertices for each triangle, so they can have
		//  different normals.  This means 12 vertices, 4 faces * 3 corners.
		vidx = AddVertex(p0); AddVertex(p1); AddVertex(p2);
		AddTri(vidx, vidx+1, vidx+2);

		vidx = AddVertex(p1); AddVertex(p3); AddVertex(p2);
		AddTri(vidx, vidx+1, vidx+2);

		vidx = AddVertex(p3); AddVertex(p0); AddVertex(p2);
		AddTri(vidx, vidx+1, vidx+2);

		vidx = AddVertex(p0); AddVertex(p3); AddVertex(p1);
		AddTri(vidx, vidx+1, vidx+2);
	}
	else
	{
		// With no shading, all we need is 4 vertices for the 4 faces.
		vidx = AddVertex(p0); AddVertex(p1); AddVertex(p2); AddVertex(p3);
		AddTri(vidx+0, vidx+1, vidx+2);
		AddTri(vidx+1, vidx+3, vidx+2);
		AddTri(vidx+3, vidx+0, vidx+2);
		AddTri(vidx+0, vidx+3, vidx+1);
	}
}


/**
 * Adds the vertices and a fan primitive for a single flat rectangle.
 */
void vtMeshBase::AddRectangleXZ(float xsize, float zsize)
{
	int vidx =
	AddVertexUV(-xsize/2, 0.0f, -zsize/2,	0.0f, 0.0f);
	AddVertexUV( xsize/2, 0.0f, -zsize/2,	1.0f, 0.0f);
	AddVertexUV( xsize/2, 0.0f,  zsize/2,	1.0f, 1.0f);
	AddVertexUV(-xsize/2, 0.0f,  zsize/2,	0.0f, 1.0f);
	AddFan(vidx, vidx+1, vidx+2, vidx+3);
}

/**
 * Adds the vertices and a fan primitive for a single flat rectangle.
 */
void vtMeshBase::AddRectangleXY(float x, float y, float xsize, float ysize,
								float z, bool bCentered)
{
	int vidx;
	if (bCentered)
	{
		vidx =
		AddVertexUV(-xsize/2, -ysize/2,	z, 0.0f, 0.0f);
		AddVertexUV( xsize/2, -ysize/2,	z, 1.0f, 0.0f);
		AddVertexUV( xsize/2,  ysize/2,	z, 1.0f, 1.0f);
		AddVertexUV(-xsize/2,  ysize/2,	z, 0.0f, 1.0f);
	}
	else
	{
		vidx =
		AddVertexUV(x,		 y,			z, 0.0f, 0.0f);
		AddVertexUV(x+xsize, y,			z, 1.0f, 0.0f);
		AddVertexUV(x+xsize, y+ysize,	z, 1.0f, 1.0f);
		AddVertexUV(x,		 y+ysize,	z, 0.0f, 1.0f);
	}
	if (m_ePrimType == vtMesh::TRIANGLE_FAN)
		AddFan(vidx, vidx+1, vidx+2, vidx+3);
	else if (m_ePrimType == vtMesh::QUADS)
		AddQuad(vidx, vidx+1, vidx+2, vidx+3);
}

/**
 * Adds an conical surface to this mesh.  This is a subset of a full cone,
 * bounded by start/end factors along the two degrees of freedom of the
 * surface of the cone.  The default orientation of the cone is with the
 * tip pointing up (radius increasing downward).
 *
 * \param tip The top point of the cone.
 * \param radial_angle This is the angle between the cone's edge and its
 *		center axis, in radians.  Expected range is 0 to PI/2.  Small values
 *		indicate a sharp, pointed cone, large values indicate a blunt cone.
 *		The slope of the cone's edge is tan(radial_angle).
 * \param theta1, theta2 Start and end values for the theta value, which
 *		ranges from 0 to 2*PI around the central axis of the cone.
 * \param r1, r2 Start and end values for the cone's radius.  These range
 *		from 0 (at the tip of the cone) and increase downward.
 * \param res Resolution, number of polygons along each side of the
 *		surface mesh.
 */
void vtMeshBase::CreateConicalSurface(const FPoint3 &tip, double radial_angle,
									double theta1, double theta2,
									double r1, double r2, int res)
{
	int i, j, vidx;
	double tan_cr = tan(radial_angle);
	double theta, theta_step = (theta2 - theta1) / (res - 1);
	double r, r_step = (r2 - r1) / (res - 1);

	FPoint3 p, norm;

	r = r1;
	for (i = 0; i < res; i++)
	{
		theta = theta1;
		for (j = 0; j < res; j++)
		{
			p.x = (float) (tip.x + cos(theta) * r);
			p.z = (float) (tip.z - sin(theta) * r);
			p.y = (float) (tip.y - (r / tan_cr));
			vidx = AddVertex(p);

			if (GetVtxType() & VT_Normals)
			{
				// compute vertex normal for lighting
				norm.x = (float) (cos(theta) * r);
				norm.y = 0.0f;
				norm.z = (float) (sin(theta) * r);
				norm.Normalize();
				SetVtxNormal(vidx, norm);
			}

			theta += theta_step;
		}
		r += r_step;
	}
	CreateRectangularMesh(res, res);
}

/**
 * Adds an rectangular surface to this mesh.
 * The rectangle will lie in the first two axes given, facing toward the
 * third.  Axes can be specified by number.  For example, to produce a
 * rectangle in the XZ plane facing along the Y axis, you would pass 0, 2, 1.
 *
 * \param iQuads1 The number of quads along the first axis.
 * \param iQuads2 The number of quads along the second axis.
 * \param Axis1 The first axis (X=0, Y=1, Z=2)
 * \param Axis2 The second axis (X=0, Y=1, Z=2)
 * \param Axis3 The third axis (X=0, Y=1, Z=2)
 * \param min1 The lower-left-hand corner of the rectangle's position
 * \param max1 The size of the rectangle.
 * \param fLevel The value of the rectangle on the third axis.
 * \param fTiling UV tiling.  Set to 1 for UV coordinate of O..1.
 */
void vtMeshBase::CreateRectangle(int iQuads1, int iQuads2,
		int Axis1, int Axis2, int Axis3,
		const FPoint2 &min1, const FPoint2 &max1, float fLevel, float fTiling)
{
	int iVerts1 = iQuads1 + 1;
	int iVerts2 = iQuads2 + 1;

	FPoint2 size = max1 - min1;
	FPoint3 pos, normal;

	pos[Axis3] = fLevel;
	normal[Axis1] = 0;
	normal[Axis2] = 0;
	normal[Axis3] = 1;

	for (int i = 0; i < iVerts1; i++)
	{
		pos[Axis1] = min1.x + (size.x / iQuads1) * i;
		for (int j = 0; j < iVerts2; j++)
		{
			pos[Axis2] = min1.y + (size.y / iQuads2) * j;

			int vidx = AddVertex(pos);

			if (GetVtxType() & VT_Normals)
				SetVtxNormal(vidx, normal);

			if (GetVtxType() & VT_TexCoords)		/* compute tex coords */
			{
				FPoint2 tc((float) i/iQuads1 * fTiling,
							(float) j/iQuads2 * fTiling);
				SetVtxTexCoord(vidx, tc);
			}
		}
	}
	CreateRectangularMesh(iVerts1, iVerts2);
}

/**
 * Transform all the vertices of the mesh by the indicated matrix.
 */
void vtMeshBase::TransformVertices(const FMatrix4 &mat)
{
	int i, num = GetNumVertices();
	FPoint3 p, p2;

	for (i = 0; i < num; i++)
	{
		p = GetVtxPos(i);
		mat.Transform(p, p2);
		SetVtxPos(i, p2);
	}
	if (GetVtxType() & VT_Normals)
	{
		for (i = 0; i < num; i++)
		{
			p = GetVtxNormal(i);
			mat.TransformVector(p, p2);
			SetVtxNormal(i, p2);
		}
	}
}

////////////////////////////////////////////////////////////////////

vtMaterialArrayBase::~vtMaterialArrayBase()
{
	Empty(); free(m_Data); m_Data = NULL; m_MaxSize = 0;
}

void vtMaterialArrayBase::DestructItems(unsigned int first, unsigned int last)
{
	for (unsigned int i = first; i <= last; i++)
	{
		vtMaterial *pMat = GetAt(i);
		delete pMat;
	}
}

/**
 * Create and add a simple textured material.  This method takes a vtImage
 * and let you control many other aspects of the material.  Only the first
 * three parameters are required, the rest will be assumed with default
 * values if desired.
 *
 * \param pImage  A valid image (bitmap) which you have created.
 *
 * \param bCulling  true to cull backfaces (only the front side
 *		of each polygon is rendered.)
 *
 * \param bLighting  true to "light" the material.  This means it will
 *		use the material's color values, and any active lights to
 *		determine the color of the drawn geometry.  If false, then
 *		only the material's diffuse color is used, and it is not affected
 *		by any lights.
 *
 * \param bTransp  true for a material with some transparency (default
 *		is false).
 *
 * \param bAdditive  true for an additive material (default is false).
 *		This means that the rendered color will be added to, rather than
 *		replace, the rendering surface.  This is useful for some effects
 *		such as drawing stars against a twilight sky, in which the light
 *		of a dim star should be added to the background sky.
 *
 * \param fAmbient  Ambient material value, ranges from 0 to 1 (default 0).
 *		If lighting is enabled, this determines how this material is affected
 *		by the Ambient component of each existing light.
 *
 * \param fDiffuse  Diffuse material value, ranges from 0 to 1 (default 1).
 *		If lighting is enabled, this determines how this material is affected
 *		by the Diffuse component of each existing light.  If lighting is
 *		not enabled, this indicates the exact lighting value to use for the
 *		material.
 *
 * \param fAlpha	Alpha value (opacity), ranges from 0 (completely
 *		transparent) to 1 (opaque).  Default is 1.  If transparency is
 *		not enabled, this value is ignored.
 *
 * \param fEmissive  Emmisive material value, ranges from 0 to 1 (default 0).
 *		If lighting is enabled, this value is added to the combined
 *		effect of each existing light.  This is useful for geometry which
 *		is brighter than the existing light level, such as illuminated
 *		objects at night.
 *
 * \param bTexGen	true for materials whose texture mapping will be generated
 *		automatically.  false if you will provide explicit UV values to
 *		drape your texture.  Default is false.
 *
 * \param bClamp	true for Texture Clamping, which prevents sub-texel
 *		interpolation at the edge of the texture.  Default is false.
 *
 * \param bMipMap	true for Mip-mapping, which provided smoother interpolation
 *		of the textured geometry when it is under-sampled (generally when it
 *		is in the distance.)  Useful for avoiding unpleasant aliasing artifacts,
 *		but costs 1/3 more texture memory.
 *
 * \return The index of the added material.
 */
int vtMaterialArrayBase::AddTextureMaterial(vtImage *pImage,
						 bool bCulling, bool bLighting,
						 bool bTransp, bool bAdditive,
						 float fAmbient, float fDiffuse,
						 float fAlpha, float fEmissive,
						 bool bTexGen, bool bClamp,
						 bool bMipMap)
{
	vtMaterial *pMat = new vtMaterial;
	pMat->SetTexture(pImage);
	pMat->SetCulling(bCulling);
	pMat->SetLighting(bLighting);
	pMat->SetTransparent(bTransp, bAdditive);
	pMat->SetAmbient(fAmbient, fAmbient, fAmbient);
	pMat->SetDiffuse(fDiffuse, fDiffuse, fDiffuse, fAlpha);
	pMat->SetEmission(fEmissive, fEmissive, fEmissive);
	pMat->SetClamp(bClamp);
	pMat->SetMipMap(bMipMap);

	return AppendMaterial(pMat);
}

/**
 * Create and add a simple textured material.  This method takes a a filename
 * of the texture image to use.
 *
 * See AddTextureMaterial() for a description of the parameters, which
 * lets you control many other aspects of the material.
 *
 * \return The index of the added material if successful, or -1 on failure.
 */
int vtMaterialArrayBase::AddTextureMaterial2(const char *fname,
						 bool bCulling, bool bLighting,
						 bool bTransp, bool bAdditive,
						 float fAmbient, float fDiffuse,
						 float fAlpha, float fEmissive,
						 bool bTexGen, bool bClamp,
						 bool bMipMap)
{
	// check for common mistake
	if (*fname == 0)
		return -1;

	vtImage *pImage = vtImageRead(fname);
	if (!pImage)
		return -1;

	int index = AddTextureMaterial(pImage, bCulling, bLighting,
		bTransp, bAdditive, fAmbient, fDiffuse, fAlpha, fEmissive, bTexGen,
		bClamp, bMipMap);
	pImage->Release();	// pass ownership to the material
	return index;
}

/**
 * Create and add a simple colored material.  This method takes diffuse
 * and ambient color and let you control several other aspects of the material.
 *
 * \param diffuse The Diffuse color component of the material.
 *
 * \param ambient The Ambient color component of the material.
 *
 * \param bCulling  true to cull backfaces (only the front side
 *		of each polygon is rendered.)
 *
 * \param bLighting  true to "light" the material.  This means it will
 *		use the material's color values, and any active lights to
 *		determine the color of the drawn geometry.  If false, then
 *		only the material's diffuse color is used, and it is not affected
 *		by any lights.
 *
 * \param bWireframe True for a material which will render only the edges
 *		of polygons.
 *
 * \param fAlpha	Alpha value (opacity), ranges from 0 (completely
 *		transparent) to 1 (opaque).  Default is 1.  If transparency is
 *		not enabled, this value is ignored.
 *
 * \param fEmissive  Emmisive material value, ranges from 0 to 1 (default 0).
 *		If lighting is enabled, this value is added to the combined
 *		effect of each existing light.  This is useful for geometry which
 *		is brighter than the existing light level, such as illuminated
 *		objects at night.
 *
 * \return The index of the added material.
 */
int vtMaterialArrayBase::AddRGBMaterial(const RGBf &diffuse, const RGBf &ambient,
					 bool bCulling, bool bLighting, bool bWireframe,
					 float fAlpha, float fEmissive)
{
	vtMaterial *pMat = new vtMaterial;
	pMat->SetCulling(bCulling);
	pMat->SetLighting(bLighting);
	pMat->SetWireframe(bWireframe);
	pMat->SetDiffuse(diffuse.r, diffuse.g, diffuse.b, fAlpha);
	pMat->SetSpecular(0.0f, 0.0f, 0.0f);
	pMat->SetAmbient(ambient.r, ambient.g, ambient.b);
	pMat->SetEmission(fEmissive, fEmissive, fEmissive);
	return AppendMaterial(pMat);
}

/**
 * Create and add a simple colored material.  This method takes diffuse
 * color and let you control several other aspects of the material.
 * Ambient color will be assumed to a be a dimmer shade of the supplied
 * diffuse color (diffuse / 4).
 *
 * \param diffuse The Diffuse color component of the material.
 *
 * \param bCulling  true to cull backfaces (only the front side
 *		of each polygon is rendered.)
 *
 * \param bLighting  true to "light" the material.  This means it will
 *		use the material's color values, and any active lights to
 *		determine the color of the drawn geometry.  If false, then
 *		only the material's diffuse color is used, and it is not affected
 *		by any lights.
 *
 * \param bWireframe True for a material which will render only the edges
 *		of polygons.
 *
 * \param fAlpha	Alpha value (opacity), ranges from 0 (completely
 *		transparent) to 1 (opaque).  Default is 1.  If transparency is
 *		not enabled, this value is ignored.
 *
 * \param fEmissive  Emmisive material value, ranges from 0 to 1 (default 0).
 *		If lighting is enabled, this value is added to the combined
 *		effect of each existing light.  This is useful for geometry which
 *		is brighter than the existing light level, such as illuminated
 *		objects at night.
 *
 * \return The index of the added material.
 */
int vtMaterialArrayBase::AddRGBMaterial1(const RGBf &diffuse,
				 bool bCulling, bool bLighting, bool bWireframe,
				 float fAlpha, float fEmissive)
{
	return AddRGBMaterial(diffuse, diffuse/4, bCulling, bLighting, bWireframe,
		fAlpha, fEmissive);
}

/**
 * Create and add a "shadow" material, which is a black material with
 * transparency, suitable for drawing a shadow under an object.
 *
 * \param fOpacity Ranges from 0 (fully transparent) to 1 (fully opaque).
 */
void vtMaterialArrayBase::AddShadowMaterial(float fOpacity)
{
	vtMaterial *pMat = new vtMaterial;
	pMat->SetCulling(true);
	pMat->SetLighting(false);
	pMat->SetTransparent(true);
	pMat->SetDiffuse(0.0f, 0.0f, 0.0f, fOpacity);
	AppendMaterial(pMat);
}

/**
 * Copies all the materials from one vtMaterialArray to another.
 *
 * The materials copied from the source are simply appending to this
 * array.
 */
void vtMaterialArrayBase::CopyFrom(vtMaterialArrayBase *pFrom)
{
	int num = pFrom->GetSize();
	for (int i = 0; i < num; i++)
	{
		vtMaterial *pMat1 = pFrom->GetAt(i);
		vtMaterial *pMat2 = new vtMaterial;
		pMat2->CopyFrom(pMat1);
		AppendMaterial(pMat2);
	}
}

void vtMaterialArrayBase::RemoveMaterial(vtMaterial *pMat)
{
	int num = Find(pMat);
	if (num != -1)
		RemoveAt(num);
}

/**
 * Find a material in an array by looking for a specific diffuse color.
 *
 * \return The index of the material if found, otherwise -1.
 */
int vtMaterialArrayBase::FindByDiffuse(const RGBAf &rgba) const
{
	for (unsigned int i = 0; i < GetSize(); i++)
	{
		if (rgba == GetAt(i)->GetDiffuse())
			return i;
	}
	return -1;
}

/**
 * Find a material in an array by looking for a specific texture image.
 *
 * \return The index of the material if found, otherwise -1.
 */
int vtMaterialArrayBase::FindByImage(const vtImage *image) const
{
	for (unsigned int i = 0; i < GetSize(); i++)
	{
		const vtImage *tex = GetAt(i)->GetTexture();
		if (tex == image)
			return i;
	}
	return -1;
}


///////////////////////////////////////////////////////////

vtMaterialBase::vtMaterialBase()
{
}

/**
 * Copy this material from another.
 */
void vtMaterialBase::CopyFrom(vtMaterial *pFrom)
{
	SetDiffuse1(pFrom->GetDiffuse());
	SetSpecular1(pFrom->GetSpecular());
	SetAmbient1(pFrom->GetAmbient());
	SetEmission1(pFrom->GetEmission());

	SetCulling(pFrom->GetCulling());
	SetLighting(pFrom->GetLighting());

//	SetTexture(pFrom->GetTexture());
	SetTransparent(pFrom->GetTransparent());
}

