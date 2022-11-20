#pragma once

#include <optional>

#include "ComplexCore/utils/PSInfoGenerator.hpp"
#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class ImageGeom
 * @brief
 */
class COMPLEX_EXPORT PSNodeBasedGeomInfoGenerator : public PSInfoGenerator
{
public:
  PSNodeBasedGeomInfoGenerator(const INodeGeometry0D& geometry, const SizeVec3& numOfPartitionsPerAxis)
  : PSInfoGenerator(numOfPartitionsPerAxis)
  , m_Geometry(geometry)

  {
    const IGeometry::SharedVertexList* vertexList = m_Geometry.getVertices();
    if(vertexList == nullptr)
    {
      return;
    }

    const AbstractDataStore<float32>& vertexListStore = vertexList->getDataStoreRef();

    FloatVec3 ll = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    FloatVec3 ur = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()};

    // For each tuple, update the current lower-left and upper-right coordinates
    std::set<float> setX;
    std::set<float> setY;
    std::set<float> setZ;
    for(size_t tuple = 0; tuple < vertexListStore.getNumberOfTuples(); tuple++)
    {
      float x = vertexListStore.getComponentValue(tuple, 0);
      setX.insert(x);
      ll[0] = (x < ll[0]) ? x : ll[0];
      ur[0] = (x > ur[0]) ? x : ur[0];

      float y = vertexListStore.getComponentValue(tuple, 1);
      setY.insert(y);
      ll[1] = (y < ll[1]) ? y : ll[1];
      ur[1] = (y > ur[1]) ? y : ur[1];

      float z = vertexListStore.getComponentValue(tuple, 2);
      setZ.insert(z);
      ll[2] = (z < ll[2]) ? z : ll[2];
      ur[2] = (z > ur[2]) ? z : ur[2];
    }

    // YZ Plane
    if(setX.size() == 1)
    {
      m_YZPlane = true;
    }

    // XZ Plane
    if(setY.size() == 1)
    {
      m_XZPlane = true;
    }

    // XY Plane
    if(setZ.size() == 1)
    {
      m_XYPlane = true;
    }

    // Pad the edges
    float padding = 0.000001;
    ll[0] -= padding;
    ll[1] -= padding;
    ll[2] -= padding;
    ur[0] += padding;
    ur[1] += padding;
    ur[2] += padding;

    m_Origin = ll;
    m_PartitionLength = calculatePartitionLengthsUsingBounds(ll, ur);
  }

  // -----------------------------------------------------------------------------
  Result<> checkDimensionality() const
  {
    if(m_YZPlane)
    {
      return {MakeErrorResult(-3040, "Unable to create a partitioning scheme with a X dimension size of 0.  Vertices are in an YZ plane.  Use the Advanced or Bounding Box "
                                     "partitioning modes to manually create a partitioning scheme."),
              {}};
    }
    if(m_XZPlane)
    {
      return {MakeErrorResult(-3041, "Unable to create a partitioning scheme with a Y dimension size of 0.  Vertices are in an XZ plane.  Use the Advanced or Bounding Box "
                                     "partitioning modes to manually create a partitioning scheme."),
              {}};
    }
    if(m_XYPlane)
    {
      return {MakeErrorResult(-3042, "Unable to create a partitioning scheme with a Z dimension size of 0.  Vertices are in an XY plane.  Use the Advanced or Bounding Box "
                                     "partitioning modes to manually create a partitioning scheme."),
              {}};
    }

    return {};
  }

  // -----------------------------------------------------------------------------
  std::optional<FloatVec3> getOrigin() const override
  {
    return m_Origin;
  }

  // -----------------------------------------------------------------------------
  std::optional<FloatVec3> getPartitionLength() const override
  {
    return m_PartitionLength;
  }

private:
  const INodeGeometry0D& m_Geometry;
  std::optional<FloatVec3> m_Origin;
  std::optional<FloatVec3> m_PartitionLength;
  bool m_XYPlane = false;
  bool m_XZPlane = false;
  bool m_YZPlane = false;

  // -----------------------------------------------------------------------------
  IGeometry::LengthUnit getUnits() const override
  {
    return m_Geometry.getUnits();
  }
};
} // namespace complex
