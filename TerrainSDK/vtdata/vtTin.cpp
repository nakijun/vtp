//
// vtTin.cpp
//
// Class which represents a Triangulated Irregular Network.
//
// Copyright (c) 2002-2011 Virtual Terrain Project
// Free for all uses, see license.txt for details.
//

#include "vtTin.h"
#include "vtLog.h"
#include "DxfParser.h"
#include "FilePath.h"
#include "ByteOrder.h"


vtTin::vtTin()
{
	m_trianglebins = NULL;
}

vtTin::~vtTin()
{
	FreeData();
}

void vtTin::AddVert(const DPoint2 &p, float z)
{
	m_vert.Append(p);
	m_z.Append(z);
}

// Add vertex with vertex normal
void vtTin::AddVert(const DPoint2 &p, float z, FPoint3 &normal)
{
	m_vert.Append(p);
	m_z.Append(z);
	m_vert_normal.Append(normal);
}

void vtTin::AddTri(int i1, int i2, int i3, int surface_type)
{
	m_tri.Append(i1);
	m_tri.Append(i2);
	m_tri.Append(i3);
	if (surface_type != -1)
		m_surfidx.Append(surface_type);
}

void vtTin::RemVert(int v)
{
	// safety check
	if (v < 0 || v >= (int) m_vert.GetSize())
		return;
	m_vert.RemoveAt(v);
	m_z.RemoveAt(v);
	m_vert_normal.RemoveAt(v);

	// Re-index the triangles
	for (unsigned int i = 0; i < m_tri.GetSize()/3; i++)
	{
		// Remove any triangles which referenced this vertex
		if (m_tri[i*3 + 0] == v ||
			m_tri[i*3 + 1] == v ||
			m_tri[i*3 + 2] == v)
		{
			m_tri.RemoveAt(i*3, 3);
			i--;
			continue;
		}
		// For all other triangles, adjust the indices to reflect the removal
		if (m_tri[i*3 + 0] > v) m_tri[i*3 + 0] = m_tri[i*3 + 0] - 1;
		if (m_tri[i*3 + 1] > v) m_tri[i*3 + 1] = m_tri[i*3 + 1] - 1;
		if (m_tri[i*3 + 2] > v) m_tri[i*3 + 2] = m_tri[i*3 + 2] - 1;
	}
}

void vtTin::RemTri(int t)
{
	// safety check
	if (t < 0 || t >= (int) m_tri.GetSize())
		return;
	m_tri.RemoveAt(t*3, 3);
}

unsigned int vtTin::AddSurfaceType(const vtString &surface_texture, bool bTiled)
{
	m_surftypes.push_back(surface_texture);
	m_surftype_tiled.Append(bTiled);
	return m_surftypes.size()-1;
}

void vtTin::SetSurfaceType(int iTri, int surface_type)
{
	if (m_surfidx.GetSize() != m_tri.GetSize()/3)
		m_surfidx.SetSize(m_tri.GetSize()/3);

	m_surfidx[iTri] = surface_type;
}

bool vtTin::_ReadTinOld(FILE *fp)
{
	int i, num;
	FPoint3 f;
	DPoint2 p;

	fread(&num, 1, sizeof(int), fp);
	m_vert.SetMaxSize(num);
	for (i = 0; i < num; i++)
	{
		fread(&f.x, 3, sizeof(float), fp);

		p.Set(f.x, f.y);
		AddVert(p, f.z);
	}
	for (i = 0; i < num/3; i++)
	{
		AddTri(i*3, i*3+1, i*3+2);
	}
	return true;
}

bool vtTin::_ReadTin(FILE *fp)
{
	if (!_ReadTinHeader(fp))
		return false;
	if (!_ReadTinBody(fp))
		return false;
	return true;
}

bool vtTin::_ReadTinHeader(FILE *fp)
{
	int proj_len;

	char marker[5];
	fread(marker, 5, 1, fp);
	if (strncmp(marker, "tin", 3))
		return false;	// Not a Tin
	int version = marker[4] - '0';

	fread(&m_file_verts, 4, 1, fp);
	fread(&m_file_tris, 4, 1, fp);
	fread(&m_file_data_start, 4, 1, fp);
	fread(&proj_len, 4, 1, fp);
	if (proj_len > 2000)
		return false;

	if (proj_len)
	{
		char wkt_buf[2000], *wkt = wkt_buf;
		fread(wkt, proj_len, 1, fp);
		wkt_buf[proj_len] = 0;

		OGRErr err = m_proj.importFromWkt((char **) &wkt);
		if (err != OGRERR_NONE)
			return false;
	}

	if (version > 1)
	{
		// version 2 of the format has extents: left, top, right, bottom, min z, max h
		fread(&m_EarthExtents.left, sizeof(double), 4, fp);
		fread(&m_fMinHeight, sizeof(float), 1, fp);
		fread(&m_fMaxHeight, sizeof(float), 1, fp);
	}

	return true;
}

bool vtTin::_ReadTinBody(FILE *fp)
{
	fseek(fp, m_file_data_start, SEEK_SET);

	// pre-allocate for efficiency
	m_vert.SetMaxSize(m_file_verts);
	m_tri.SetMaxSize(m_file_tris * 3);

	// read verts
	DPoint2 p;
	float z;
	for (int i = 0; i < m_file_verts; i++)
	{
		fread(&p.x, 8, 2, fp);	// 2 doubles
		fread(&z, 4, 1, fp);	// 1 float
		AddVert(p, z);
	}
	// read tris
	int tribuf[3];
	for (int i = 0; i < m_file_tris; i++)
	{
		fread(tribuf, 4, 3, fp);	// 3 ints
		AddTri(tribuf[0], tribuf[1], tribuf[2]);
	}
	return true;
}

/**
 * Read the TIN from a file.  This can either be an old-style or new-style
 * .tin format (custom VTP format)
 */
bool vtTin::Read(const char *fname)
{
	// first read the point from the .tin file
	FILE *fp = vtFileOpen(fname, "rb");
	if (!fp)
		return false;

	bool success = _ReadTin(fp);
	fclose(fp);

	if (!success)
		return false;

	ComputeExtents();
	return true;
}

/**
 * Read the TIN header from a file.
 */
bool vtTin::ReadHeader(const char *fname)
{
	// first read the point from the .tin file
	FILE *fp = vtFileOpen(fname, "rb");
	if (!fp)
		return false;

	bool success = _ReadTinHeader(fp);
	fclose(fp);

	if (!success)
		return false;

	return true;
}

/**
 * Read the TIN body from a file.
 */
bool vtTin::ReadBody(const char *fname)
{
	// first read the point from the .tin file
	FILE *fp = vtFileOpen(fname, "rb");
	if (!fp)
		return false;

	bool success = _ReadTinBody(fp);
	fclose(fp);

	if (!success)
		return false;

	return true;
}

/**
 * Attempt to read TIN data from a DXF file.
 */
bool vtTin::ReadDXF(const char *fname, bool progress_callback(int))
{
	VTLOG("vtTin::ReadDXF():\n");

	std::vector<DxfEntity> entities;
	std::vector<DxfLayer> layers;

	DxfParser parser(fname, entities, layers);
	bool bSuccess = parser.RetrieveEntities(progress_callback);
	if (!bSuccess)
	{
		VTLOG(parser.GetLastError());
		return false;
	}

	int vtx = 0;
	int found = 0;
	for (unsigned int i = 0; i < entities.size(); i++)
	{
		const DxfEntity &ent = entities[i];
		if (ent.m_iType == DET_3DFace || ent.m_iType == DET_Polygon)
		{
			int NumVerts = ent.m_points.size();
			if (NumVerts == 3)
			{
				for (int j = 0; j < 3; j++)
				{
					DPoint2 p(ent.m_points[j].x, ent.m_points[j].y);
					float z = (float) ent.m_points[j].z;

					AddVert(p, z);
				}
				AddTri(vtx, vtx+1, vtx+2);
				vtx += 3;
				found ++;
			}
		}
	}
	VTLOG(" Found %d triangle entities, of type 3DFace or Polygon.\n", found);

	// If we didn't find any surfaces, we haven't got a TIN
	if (found == 0)
		return false;

	// Test each triangle for clockwisdom, fix if needed
	CleanupClockwisdom();

	ComputeExtents();
	return true;
}

bool vtTin::ReadADF(const char *fname, bool progress_callback(int))
{
	vtString tnxy_name = fname;
	if (tnxy_name.Right(6) != "xy.adf")
		return false;

	vtString base = tnxy_name.Left(tnxy_name.GetLength()-6);
	vtString tnz_name = base + "z.adf";
	vtString tnod_name = base + "od.adf";

	FILE *fp1 = vtFileOpen(tnxy_name, "rb");
	FILE *fp2 = vtFileOpen(tnz_name, "rb");
	FILE *fp3 = vtFileOpen(tnod_name, "rb");
	if (!fp1 || !fp2 || !fp3)
		return false;

	fseek(fp1, 0, SEEK_END);
	int length_xy = ftell(fp1);
	rewind(fp1);	// go back again
	unsigned int num_points = length_xy / 16;	// X and Y, each 8 byte doubles

	fseek(fp2, 0, SEEK_END);
	int length_z = ftell(fp2);
	rewind(fp2);	// go back again
	unsigned int num_heights = length_z / 4;		// Z is a 4 byte float

	DPoint2 p;
	float z;
	for (unsigned int i = 0; i < num_points; i++)
	{
		if ((i%200) == 0 && progress_callback != NULL)
			progress_callback(i * 40 / num_points);

		FRead(&p.x, DT_DOUBLE, 2, fp1, BO_BIG_ENDIAN, BO_LITTLE_ENDIAN);
		FRead(&z, DT_FLOAT, 1, fp2, BO_BIG_ENDIAN, BO_LITTLE_ENDIAN);
		AddVert(p, z);
	}

	fseek(fp3, 0, SEEK_END);
	int length_od = ftell(fp3);
	rewind(fp3);	// go back again
	unsigned int num_faces = length_od / 12;		// A B C as 4-byte ints

	int v[3];
	for (unsigned int i = 0; i < num_faces; i++)
	{
		if ((i%200) == 0 && progress_callback != NULL)
			progress_callback(40 + i * 40 / num_faces);

		FRead(v, DT_INT, 3, fp3, BO_BIG_ENDIAN, BO_LITTLE_ENDIAN);
		AddTri(v[0]-1, v[1]-1, v[2]-1);
	}

	fclose(fp1);
	fclose(fp2);
	fclose(fp3);

	// Cleanup: the ESRI TIN contains four "boundary" point far outside the
	//  extents (directly North, South, East, and West).  We should ignore
	//  those four points and the triangles connected to them.
	// It seems we can assume the four 'extra' vertices are the first four.
	m_vert.RemoveAt(0, 4);
	m_z.RemoveAt(0, 4);
	m_vert_normal.RemoveAt(0, 4);

	// Re-index the triangles
	unsigned int total = m_tri.GetSize()/3;
	for (unsigned int i = 0; i < total; i++)
	{
		if ((i%200) == 0 && progress_callback != NULL)
			progress_callback(80 + i * 20 / total);

		// Remove any triangles which referenced this vertex
		if (m_tri[i*3 + 0] < 4 ||
			m_tri[i*3 + 1] < 4 ||
			m_tri[i*3 + 2] < 4)
		{
			m_tri.RemoveAt(i*3, 3);
			i--;
			total--;
			continue;
		}
	}
	// For all other triangles, adjust the indices to reflect the removal
	for (unsigned int i = 0; i < m_tri.GetSize(); i++)
		m_tri[i] = m_tri[i] - 4;

	// Test each triangle for clockwisdom, fix if needed
	CleanupClockwisdom();

	ComputeExtents();

	return true;
}

bool vtTin::ReadGMS(const char *fname, bool progress_callback(int))
{
	FILE *fp = vtFileOpen(fname, "rb");
	if (!fp)
		return false;

	char buf[256];
	vtString tin_name;
	int material_id;
	int num_points;

	// first line is file identifier
	if (fgets(buf, 256, fp) == NULL)
		return false;

	if (strncmp(buf, "TIN", 3) != 0)
		return false;

	while (1)
	{
		if (fgets(buf, 256, fp) == NULL)
			break;

		// trim trailing EOL characters
		vtString vstr = buf;
		vstr.Remove('\r');
		vstr.Remove('\n');
		const char *str = (const char *)vstr;

		if (!strncmp(str, "BEGT", 4))	// beginning of TIN block
			continue;

		if (!strncmp(str, "ID", 2))	// material ID
		{
			sscanf(str, "ID %d", &material_id);
		}
		else if (!strncmp(str, "MAT", 3))	// material ID
		{
			sscanf(str, "MAT %d", &material_id);
		}
		else if (!strncmp(str, "TCOL", 4))	// material ID
		{
			sscanf(str, "TCOL %d", &material_id);
		}
		else if (!strncmp(str, "TNAM", 4))	// TIN name
		{
			tin_name = str + 5;
		}
		else if (!strncmp(str, "VERT", 4))	// Beginning of vertices
		{
			sscanf(buf, "VERT %d\n", &num_points);
			DPoint2 p;
			float z;
			int optional;
			for (int i = 0; i < num_points; i++)
			{
				if (fgets(buf, 256, fp) == NULL)
					break;

				// First three are X, Y, Z.  Optional fourth is "ID" or "locked".
				sscanf(buf, "%lf %lf %f %d", &p.x, &p.y, &z, &optional);

#if 0
				// Some files have Y/-Z flipped (but they are non-standard)
				double temp = p.y; p.y = -z; z = temp;
#endif

				AddVert(p, z);

				if ((i%200) == 0 && progress_callback != NULL)
				{
					if (progress_callback(i * 49 / num_points))
					{
						fclose(fp);
						return false;	// user cancelled
					}
				}
			}
		}
		else if (!strncmp(str, "TRI", 3))	// Beginning of triangles
		{
			int num_faces;
			sscanf(str, "TRI %d\n", &num_faces);
			int v[3];
			for (int i = 0; i < num_faces; i++)
			{
				fscanf(fp, "%d %d %d\n", v, v+2, v+1);
				// the indices in the file are 1-based, so subtract 1
				AddTri(v[0]-1, v[1]-1, v[2]-1);

				if ((i%200) == 0 && progress_callback != NULL)
				{
					if (progress_callback(49 + i * 50 / num_faces))
					{
						fclose(fp);
						return false;	// user cancelled
					}
				}
			}
		}
	}

	fclose(fp);

	// Test each triangle for clockwisdom, fix if needed
	//CleanupClockwisdom();

	ComputeExtents();

	return true;
}

bool vtTin::WriteGMS(const char *fname, bool progress_callback(int))
{
	FILE *fp = vtFileOpen(fname, "wb");
	if (!fp)
		return false;

	// first line is file identifier
	fprintf(fp, "TIN\n");
	fprintf(fp, "BEGT\n");
	//fprintf(fp, "TNAM tin\n");	// "name" of the TIN
	//fprintf(fp, "MAT 1\n");		// "TIN material ID"; optional?

	int i, count = 0;
	int verts = NumVerts();
	int tris = NumTris();
	int total = verts + tris;

	// write verts
	fprintf(fp, "VERT %d\n", verts);
	for (i = 0; i < verts; i++)
	{
		fprintf(fp, "%lf %lf %lf\n", m_vert[i].x, m_vert[i].y, m_z[i]);

		if (progress_callback && (++count % 200) == 0)
			progress_callback(count * 99 / total);
	}
	// write tris
	fprintf(fp, "TRI %d\n", tris);
	for (i = 0; i < tris; i++)
	{
		// the indices in the file are 1-based, so add 1
		fprintf(fp, "%d %d %d\n", m_tri[i*3+0]+1, m_tri[i*3+1]+1, m_tri[i*3+2]+1);

		if (progress_callback && (++count % 200) == 0)
			progress_callback(count * 99 / total);
	}
	fprintf(fp, "ENDT\n");
	fclose(fp);
	return true;
}

void vtTin::FreeData()
{
	m_vert.FreeData();
	m_tri.FreeData();

	// The bins must be cleared when the triangles are freed
	if (m_trianglebins)
	{
		delete m_trianglebins;
		m_trianglebins = NULL;
	}
}

/**
 * Write the TIN to a new-style .tin file (custom VTP format).
 */
bool vtTin::Write(const char *fname, bool progress_callback(int)) const
{
	FILE *fp = vtFileOpen(fname, "wb");
	if (!fp)
		return false;

	char *wkt;
	OGRErr err = m_proj.exportToWkt(&wkt);
	if (err != OGRERR_NONE)
	{
		fclose(fp);
		return false;
	}
	int proj_len = strlen(wkt);
	int data_start = 5 + 4 + 4 + 4 + + 4 + proj_len + 32 + 4 + 4;

	int i;
	int verts = NumVerts();
	int tris = NumTris();

	fwrite("tin02", 5, 1, fp);	// version 2
	fwrite(&verts, 4, 1, fp);
	fwrite(&tris, 4, 1, fp);
	fwrite(&data_start, 4, 1, fp);
	fwrite(&proj_len, 4, 1, fp);
	fwrite(wkt, proj_len, 1, fp);
	OGRFree(wkt);

	// version 2 of the format has extents: left, top, right, bottom, min z, max h
	fwrite(&m_EarthExtents.left, sizeof(double), 4, fp);
	fwrite(&m_fMinHeight, sizeof(float), 1, fp);
	fwrite(&m_fMaxHeight, sizeof(float), 1, fp);

	// room for future extention: you can add fields here, as long as you
	// increase the data_start offset above accordingly

	// count progress
	int count = 0, total = verts + tris;

	// write verts
	for (i = 0; i < verts; i++)
	{
		fwrite(&m_vert[i].x, 8, 2, fp);	// 2 doubles
		fwrite(&m_z[i], 4, 1, fp);		// 1 float

		if (progress_callback && (++count % 100) == 0)
			progress_callback(count * 99 / total);
	}
	// write tris
	for (i = 0; i < tris; i++)
	{
		fwrite(&m_tri[i*3], 4, 3, fp);	// 3 ints

		if (progress_callback && (++count % 100) == 0)
			progress_callback(count * 99 / total);
	}

	fclose(fp);
	return true;
}

bool vtTin::ComputeExtents()
{
	int size = NumVerts();
	if (size == 0)
		return false;

	m_EarthExtents.SetRect(1E9, -1E9, -1E9, 1E9);
	m_fMinHeight = 1E9;
	m_fMaxHeight = -1E9;

	for (int j = 0; j < size; j++)
	{
		m_EarthExtents.GrowToContainPoint(m_vert[j]);

		float z = m_z[j];
		if (z > m_fMaxHeight)
			m_fMaxHeight = z;
		if (z < m_fMinHeight)
			m_fMinHeight = z;
	}
	return true;
}

void vtTin::Offset(const DPoint2 &p)
{
	unsigned int size = m_vert.GetSize();
	for (unsigned int j = 0; j < size; j++)
		m_vert[j] += p;
	ComputeExtents();
}

void vtTin::Scale(float fFactor)
{
	unsigned int size = m_z.GetSize();
	for (unsigned int j = 0; j < size; j++)
		m_z[j] *= fFactor;
	ComputeExtents();
}

void vtTin::VertOffset(float fAmount)
{
	unsigned int size = m_z.GetSize();
	for (unsigned int j = 0; j < size; j++)
		m_z[j] += fAmount;
	ComputeExtents();
}

bool vtTin::TestTriangle(int tri, const DPoint2 &p, float &fAltitude) const
{
	// get points
	int v0 = m_tri[tri*3];
	int v1 = m_tri[tri*3+1];
	int v2 = m_tri[tri*3+2];
	DPoint2 p1 = m_vert.GetAt(v0);
	DPoint2 p2 = m_vert.GetAt(v1);
	DPoint2 p3 = m_vert.GetAt(v2);

	// First try to identify which triangle
	if (PointInTriangle(p, p1, p2, p3))
	{
		double bary[3], val;
		if (BarycentricCoords(p1, p2, p3, p, bary))
		{
			// compute barycentric combination of function values at vertices
			val = bary[0] * m_z[v0] +
				bary[1] * m_z[v1] +
				bary[2] * m_z[v2];
			fAltitude = (float) val;
			return true;
		}
	}
	return false;
}

/**
 * If you are going to do a large number of height-testing of this TIN
 * (with FindAltitudeOnEarth), call this method once first to set up a
 * series of bins which greatly speed up testing.
 *
 * \param bins Number of bins per dimension, e.g. a value of 50 produces
 *		50*50=2500 bins.  More bins produces faster height-testing with
 *		the only tradeoff being a small amount of RAM per bin.
 * \param progress_callback If supplied, this function will be called back
 *		with a value of 0 to 100 as the operation progresses.
 */
void vtTin::SetupTriangleBins(int bins, bool progress_callback(int))
{
	DRECT rect = m_EarthExtents;
	m_BinSize.x = rect.Width() / bins;
	m_BinSize.y = rect.Height() / bins;

	if (m_trianglebins)
		delete m_trianglebins;
	m_trianglebins = new BinArray(bins, bins);

	DPoint2 p1, p2, p3;
	unsigned int tris = NumTris();
	for (unsigned int i = 0; i < tris; i++)
	{
		if ((i%100)==0 && progress_callback)
			progress_callback(i * 100 / tris);

		// get 2D points
		p1 = m_vert.GetAt(m_tri[i*3]);
		p2 = m_vert.GetAt(m_tri[i*3+1]);
		p3 = m_vert.GetAt(m_tri[i*3+2]);

		// find the correct range of bins, and add the index of this index to it
		DPoint2 fminrange, fmaxrange;

		fminrange.x = std::min(std::min(p1.x, p2.x), p3.x);
		fmaxrange.x = std::max(std::max(p1.x, p2.x), p3.x);

		fminrange.y = std::min(std::min(p1.y, p2.y), p3.y);
		fmaxrange.y = std::max(std::max(p1.y, p2.y), p3.y);

		IPoint2 bin_start, bin_end;

		bin_start.x = (unsigned int) ((fminrange.x-rect.left) / m_BinSize.x);
		bin_end.x = (unsigned int)	 ((fmaxrange.x-rect.left) / m_BinSize.x);

		bin_start.y = (unsigned int) ((fminrange.y-rect.bottom) / m_BinSize.y);
		bin_end.y = (unsigned int)	 ((fmaxrange.y-rect.bottom) / m_BinSize.y);

		for (int j = bin_start.x; j <= bin_end.x; j++)
		{
			for (int k = bin_start.y; k <= bin_end.y; k++)
			{
				Bin *bin = m_trianglebins->GetBin(j, k);
				if (bin)
					bin->Append(i);
			}
		}
	}
}

int vtTin::MemoryNeededToLoad() const
{
	int bytes = m_file_verts * sizeof(DPoint2);	// xy 
	bytes += m_file_verts * sizeof(float);		// z
	bytes += sizeof(int) * 3 * m_file_tris;		// triangles
	return bytes;
}

bool vtTin::FindAltitudeOnEarth(const DPoint2 &p, float &fAltitude, bool bTrue) const
{
	unsigned int tris = NumTris();

	// If we have some triangle bins, they can be used for a much faster test
	if (m_trianglebins != NULL)
	{
		int col = (int) ((p.x - m_EarthExtents.left) / m_BinSize.x);
		int row = (int) ((p.y - m_EarthExtents.bottom) / m_BinSize.y);
		Bin *bin = m_trianglebins->GetBin(col, row);
		if (!bin)
			return false;

		for (unsigned int i = 0; i < bin->GetSize(); i++)
		{
			if (TestTriangle(bin->GetAt(i), p, fAltitude))
				return true;
		}
		// If it was not in any of these bins, then it did not hit anything
		return false;
	}

	for (unsigned int i = 0; i < tris; i++)
	{
		if (TestTriangle(i, p, fAltitude))
			return true;
	}
	return false;
}

bool vtTin::FindAltitudeAtPoint(const FPoint3 &p3, float &fAltitude,
		bool bTrue, int iCultureFlags, FPoint3 *vNormal) const
{
	// Convert to 2D earth point, and test vs. TIN triangles
	DPoint3 earth;
	m_Conversion.ConvertToEarth(p3, earth);

	return FindAltitudeOnEarth(DPoint2(earth.x, earth.y), fAltitude, bTrue);
}


bool vtTin::ConvertProjection(const vtProjection &proj_new)
{
	// Create conversion object
	OCT *trans = CreateCoordTransform(&m_proj, &proj_new);
	if (!trans)
		return false;		// inconvertible projections

	int size = NumVerts();
	for (int i = 0; i < size; i++)
	{
		DPoint2 &p = m_vert[i];
		trans->Transform(1, &p.x, &p.y);
	}
	delete trans;

	// adopt new projection
	m_proj = proj_new;

	return true;
}


/**
 * Test each triangle for clockwisdom, fix if needed.  The result should
 *  be a TIN with consistent vertex ordering, such that all face normals
 *  point up rather than down, that is, counter-clockwise.
 */
void vtTin::CleanupClockwisdom()
{
	DPoint2 p1, p2, p3;		// 2D points
	int v0, v1, v2;
	unsigned int tris = NumTris();
	for (unsigned int i = 0; i < tris; i++)
	{
		v0 = m_tri[i*3];
		v1 = m_tri[i*3+1];
		v2 = m_tri[i*3+2];
		// get 2D points
		p1 = m_vert.GetAt(v0);
		p2 = m_vert.GetAt(v1);
		p3 = m_vert.GetAt(v2);

		// The so-called 2D cross product
		double cross2d = (p2-p1).Cross(p3-p1);
		if (cross2d < 0)
		{
			// flip
			m_tri[i*3+1] = v2;
			m_tri[i*3+2] = v1;
		}
	}
}

/**
 * Because the TIN triangles refer to their vertices by index, it's possible
 * to have some vertices which are not referenced.  Find and remove those
 * vertices.
 * \return The number of unused vertices removed.
 */
int vtTin::RemoveUnusedVertices()
{
	size_t verts = NumVerts();
	std::vector<bool> used;
	used.resize(verts, false);

	// Flag all the vertices that are used
	size_t tris = NumTris();
	for (size_t i = 0; i < tris; i++)
	{
		used[m_tri[i*3]] = true;
		used[m_tri[i*3+1]] = true;
		used[m_tri[i*3+2]] = true;
	}

	// Remove all the vertices that weren't flagged
	int count = 0;
	for (size_t i = 0; i < verts;)
	{
		if (!used[i])
		{
			// Remove vertex
			RemVert(i);
			used.erase(used.begin()+i);
			verts--;
			count++;
		}
		else
			i++;
	}
	return count;
}

/**
 * Return the length of the longest edge of a specific triangle.
 */
double vtTin::GetTriMaxEdgeLength(int iTri) const
{
	int tris = NumTris();
	if (iTri < 0 || iTri >= tris)
		return 0.0;

	// get points
	int v0 = m_tri[iTri*3];
	int v1 = m_tri[iTri*3+1];
	int v2 = m_tri[iTri*3+2];
	DPoint2 p1 = m_vert.GetAt(v0);
	DPoint2 p2 = m_vert.GetAt(v1);
	DPoint2 p3 = m_vert.GetAt(v2);

	// check lengths
	double len1 = (p2 - p1).Length();
	double len2 = (p3 - p2).Length();
	double len3 = (p1 - p3).Length();
	return len1 > len2 ?
		(len1 > len3 ? len1 : len3) :
	(len2 > len3 ? len2 : len3);
}


// Number of bins used by the merge algorithm.  Time is roughly proportional
// to N*N/BINS, where N is the number of vertices, so increase BINS for speed.
//
#define BINS	4000

/**
 * Combine all vertices which are at the same location.  By removing these
 * redundant vertices, the mesh will consume less space in memory and on disk.
 */
void vtTin::MergeSharedVerts(bool progress_callback(int))
{
	unsigned int verts = NumVerts();

	unsigned int i, j;
	int bin;

	DRECT rect = m_EarthExtents;
	double width = rect.Width();

	// make it slightly larger avoid edge condition
	rect.left -= 0.000001;
	width += 0.000002;

	m_bReplace = new int[verts];
	m_vertbin = new Bin[BINS];
	m_tribin = new Bin[BINS];

	// sort the vertices into bins
	for (i = 0; i < verts; i++)
	{
		// flag all vertices initially not to remove
		m_bReplace[i] = -1;

		// find the correct bin, and add the index of this vertex to it
		bin = (int) (BINS * (m_vert[i].x - rect.left) / width);
		m_vertbin[bin].Append(i);
	}
	unsigned int trisize = m_tri.GetSize();
	for (i = 0; i < trisize; i++)
	{
		// find the correct bin, and add the index of this index to it
		bin = (int) (BINS * (m_vert[m_tri[i]].x - rect.left) / width);
		m_tribin[bin].Append(i);
	}

	// compare within each bin, and between each adjacent bin,
	// looking for matching vertices to flag for removal
	for (bin = 0; bin < BINS; bin++)
	{
		if (progress_callback != NULL)
			progress_callback(bin * 100 / BINS);

		_CompareBins(bin, bin);
		if (bin < BINS-1)
			_CompareBins(bin, bin+1);
	}
	// now update each triangle index to point to the merge result
	for (bin = 0; bin < BINS; bin++)
	{
		if (progress_callback != NULL)
			progress_callback(bin * 100 / BINS);

		_UpdateIndicesInInBin(bin);
	}

	// now compact the vertex bins into a single array

	// make a copy to copy from
	DLine2 *vertcopy = new DLine2(m_vert);
	float *zcopy = new float[m_z.GetSize()];
	for (i = 0; i < m_z.GetSize(); i++)
		zcopy[i] = m_z[i];

	int inew = 0;	// index into brand new array (actually re-using old)

	for (bin = 0; bin < BINS; bin++)
	{
		if (progress_callback != NULL)
			progress_callback(bin * 100 / BINS);

		unsigned int binverts = m_vertbin[bin].GetSize();
		for (i = 0; i < binverts; i++)
		{
			int v_old = m_vertbin[bin].GetAt(i);
			if (m_bReplace[v_old] != -1)
				continue;

			int v_new = inew;

			// copy old to new
			m_vert[v_new] = vertcopy->GetAt(v_old);
			m_z[v_new] = zcopy[v_old];

			unsigned int bintris = m_tribin[bin].GetSize();
			for (j = 0; j < bintris; j++)
			{
				int trindx = m_tribin[bin].GetAt(j);
				if (m_tri[trindx] == v_old)
					m_tri[trindx] = v_new;
			}
			inew++;
		}
	}

	// our original array containers now hold the compacted result
	int newsize = inew;
	m_vert.SetSize(newsize);
	m_z.SetSize(newsize);

	// free up all the crud we allocated along the way
	delete [] m_bReplace;
	delete [] m_vertbin;
	delete [] m_tribin;
	delete vertcopy;
	delete [] zcopy;
}

void vtTin::_UpdateIndicesInInBin(int bin)
{
	int i, j;

	int binverts = m_vertbin[bin].GetSize();
	for (i = 0; i < binverts; i++)
	{
		int v_before = m_vertbin[bin].GetAt(i);
		int v_after = m_bReplace[v_before];

		if (v_after == -1)
			continue;

		int bintris = m_tribin[bin].GetSize();
		for (j = 0; j < bintris; j++)
		{
			int trindx = m_tribin[bin].GetAt(j);
			if (m_tri[trindx] == v_before)
				m_tri[trindx] = v_after;
		}
	}
}

void vtTin::_CompareBins(int bin1, int bin2)
{
	int i, j;
	int ix1, ix2;
	int start;

	int size1 = m_vertbin[bin1].GetSize();
	int size2 = m_vertbin[bin2].GetSize();
	for (i = 0; i < size1; i++)
	{
		ix1 = m_vertbin[bin1].GetAt(i);

		// within a bin, we can do N*N/2 instead of N*N compares
		// i.e. size1*size1/2, instead of size1*size2
		if (bin1 == bin2)
			start = i+1;
		else
			start = 0;

		for (j = start; j < size2; j++)
		{
			ix2 = m_vertbin[bin2].GetAt(j);

			// don't compare against itself
			if (ix1 == ix2)
				continue;

			if (m_vert[ix1] == m_vert[ix2])
			{
				// ensure that one of them is flagged
				if (m_bReplace[ix1] == -1)
				{
					if (m_bReplace[ix2] == -1)
						m_bReplace[ix1] = ix2;
					else if (m_bReplace[ix2] != ix1)
						m_bReplace[ix1] = m_bReplace[ix2];
				}
				if (m_bReplace[ix2] == -1)
				{
					if (m_bReplace[ix1] == -1)
						m_bReplace[ix2] = ix1;
					else if (m_bReplace[ix1] != ix2)
						m_bReplace[ix2] = m_bReplace[ix1];
				}
			}
		}
	}
}

/**
 * Remove all the triangles of this TIN which intersect a given line segment.
 *
 * \param ep1, ep2 The endpoints of the line segment.
 * \return The number of triangles removed.
 */
int vtTin::RemoveTrianglesBySegment(const DPoint2 &ep1, const DPoint2 &ep2)
{
	int count = 0;

	DPoint2 p1, p2, p3;		// 2D points
	int v0, v1, v2;
	unsigned int tris = NumTris();
	for (unsigned int i = 0; i < tris; i++)
	{
		// get 2D points
		v0 = m_tri[i*3];
		v1 = m_tri[i*3+1];
		v2 = m_tri[i*3+2];
		p1 = m_vert.GetAt(v0);
		p2 = m_vert.GetAt(v1);
		p3 = m_vert.GetAt(v2);

		if (LineSegmentsIntersect(ep1, ep2, p1, p2) ||
			LineSegmentsIntersect(ep1, ep2, p2, p3) ||
			LineSegmentsIntersect(ep1, ep2, p3, p1))
		{
			m_tri.RemoveAt(i*3, 3);
			i--;
			tris--;
			count++;
		}
	}
	if (count > 0)
	{
		RemoveUnusedVertices();
		ComputeExtents();
	}
	return count;
}

