// MMGeometryGL.h
//
// Interface for MMGeometryGL, which converts a SurfaceNet into a form that can be used
// for rendering by OpenGL (e.g., as C-style triangle vertex and index arrays). In this
// implementation, surface quads are flat shaded (i.e., one surface normal per quad).
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA

#ifndef MM_GEOMETRY_GL_H
#define MM_GEOMETRY_GL_H

#include <map>

class MMSurfaceNet;

class MMGeometryGL
{
public:
  struct GLVertex
  {
    float pos[3];
    float norm[3];
    float tex[2];
  };

  MMGeometryGL(MMSurfaceNet* surfaceNet);
  ~MMGeometryGL();

  void origin(float origin[3]);
  void maxSize(float size[3]);

  // Vertices are returned as a sequential list of C-style float[8] arrays (i.e.,
  // {pos[0], pos[1], pos[2], norm[0], norm[1], norm[2], tex[0], tex[1]}). Indices
  // are returned as a sequential list of C-style int[3] arrays (i.e., {v0, v1, v2}).
  int numVertices()
  {
    return m_numVertices;
  };
  float* vertices()
  {
    return (float*)m_vertices;
  };
  int numIndices()
  {
    return m_numIndices;
  };
  unsigned int* indices()
  {
    return m_indices;
  };

private:
  float m_origin[3];
  float m_size[3];
  int m_numVertices;
  int m_numIndices;
  float* m_vertices;
  unsigned int* m_indices;
  std::map<int, float> m_labelToTexCoord;

  void makeGLQuad(float* positions, unsigned short tissueLabels[2], float* quadVerts, unsigned int* quadIndices, int idxOffset);
  void computeQuadNormal(float* positions, float* normal);

  /*	static void makeGLQuad(float* positions, unsigned short tissueLabels[2], float* quadVerts,
    unsigned int* quadIndices, int idxOffset);
  static void computeQuadNormal(float* positions, float* normal);*/
};

#endif
