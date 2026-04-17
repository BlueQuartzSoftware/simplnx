#include "GeometryMath.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"

#include <stdexcept>

using namespace nx::core;

float32 nx::core::GeometryMath::AngleBetweenVectors(const nx::core::ZXZEuler& a, const nx::core::ZXZEuler& b)
{
  float norm1 = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
  float norm2 = std::sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
  float cosAng = (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) / (norm1 * norm2);
  if(cosAng < -1.0f)
  {
    cosAng = -1.0f;
  }
  else if(cosAng > 1.0)
  {
    cosAng = 1.0;
  }
  return std::acos(cosAng);
}

BoundingBox3Df nx::core::GeometryMath::FindBoundingBoxOfVertices(INodeGeometry0D& geom)
{
  FloatVec3 ll = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
  FloatVec3 ur = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()};

  const IGeometry::SharedVertexList& vertexList = geom.getVerticesRef();
  if(vertexList.getDataType() != DataType::float32)
  {
    return {ll, ur}; // will be invalid
  }

  auto& vertexListStore = vertexList.getDataStoreRef();

  for(size_t tuple = 0; tuple < vertexListStore.getNumberOfTuples(); tuple++)
  {
    float x = vertexListStore.getComponentValue(tuple, 0);
    ll[0] = (x < ll[0]) ? x : ll[0];
    ur[0] = (x > ur[0]) ? x : ur[0];

    float y = vertexListStore.getComponentValue(tuple, 1);
    ll[1] = (y < ll[1]) ? y : ll[1];
    ur[1] = (y > ur[1]) ? y : ur[1];

    float z = vertexListStore.getComponentValue(tuple, 2);
    ll[2] = (z < ll[2]) ? z : ll[2];
    ur[2] = (z > ur[2]) ? z : ur[2];
  }

  return {ll, ur}; // should be valid
}
