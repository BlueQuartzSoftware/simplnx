// MMGeometryOBJ.h
//
// Interface for MMGeometryOBJ, which is used to export the surface net as an OBJ file
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA

#ifndef MM_GEOMETRY_OBJ_H
#define MM_GEOMETRY_OBJ_H

#include <vector>
#include <set>

class MMSurfaceNet;
class MMQuad;

class MMGeometryOBJ
{
public:
	MMGeometryOBJ(MMSurfaceNet *surfaceNet);
	~MMGeometryOBJ();

	// OBJ data for a single model consists of a vector of unique vertex positions
	// (x, y, z) and a vector of triangle faces (v0, v1, v2), where v0, v1 and v2 are 
	// indices into the vertex position vector. Note that indices in OBJData start at
	// 1 (OBJ convention) rather than at 0 (C++ convention).
	struct OBJData {
		std::vector<std::array<float, 3>> vertexPositions;
		std::vector<std::array<int, 3>> triangles;
	};

	// Get the material labels for this SurfaceNet and the OBJ data for surfaces of the  
	// specified label
	std::vector<int> labels();
	OBJData objData(int label);

private:
	MMSurfaceNet* m_surfaceNet;
	std::vector<MMQuad> m_quads;

	struct vtxData {
		int vID;
		float position[3];
	};
	static void getQuadTriangleIDs(vtxData vData[4], bool isQuadFrontFacing, int triangleVtxIDs[6]);
};

#endif
