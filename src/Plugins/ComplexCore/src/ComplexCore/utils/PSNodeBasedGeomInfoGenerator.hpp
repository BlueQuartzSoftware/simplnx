#pragma once

#include <optional>

#include "ComplexCore/utils/PSInfoGenerator.hpp"
#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class ImageGeom
 * @brief
 */
template <class Geom>
class COMPLEX_EXPORT PSNodeBasedGeomInfoGenerator : public PSInfoGenerator
{
public:
  PSNodeBasedGeomInfoGenerator(const Geom& geometry, const SizeVec3& numOfPartitionsPerAxis)
  : PSInfoGenerator(numOfPartitionsPerAxis)
  , m_Geometry(geometry)
  , m_BoundingPoints(calculateBoundingPoints())

  {
  }

  // -----------------------------------------------------------------------------
  std::optional<FloatVec3> getOrigin() const override
  {
    if(!m_BoundingPoints.has_value())
    {
      return {};
    }

    std::pair<FloatVec3, FloatVec3> boundingPoints = *m_BoundingPoints;
    return boundingPoints.first;
  }

  // -----------------------------------------------------------------------------
  std::optional<FloatVec3> getPartitionLength() const override
  {
    if(!m_BoundingPoints.has_value())
    {
      return {};
    }

    std::pair<FloatVec3, FloatVec3> boundingPoints = *m_BoundingPoints;
    const FloatVec3& ll = boundingPoints.first;
    const FloatVec3& ur = boundingPoints.second;
    return calculatePartitionLengthsUsingBounds(ll, ur);
  }

private:
  const Geom& m_Geometry;
  std::optional<std::pair<FloatVec3, FloatVec3>> m_BoundingPoints;

  // -----------------------------------------------------------------------------
  std::optional<std::pair<FloatVec3, FloatVec3>> calculateBoundingPoints()
  {
    const IGeometry::SharedVertexList* vertexList = m_Geometry.getVertices();
    if(vertexList == nullptr)
    {
      return {};
    }

    const AbstractDataStore<float32>& vertexListStore = vertexList->getDataStoreRef();

    FloatVec3 ll = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    FloatVec3 ur = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()};

    // For each tuple, update the current lower-left and upper-right coordinates
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

    // Pad the edges
    float padding = 0.000001;
    ll[0] -= padding;
    ll[1] -= padding;
    ll[2] -= padding;
    ur[0] += padding;
    ur[1] += padding;
    ur[2] += padding;

    return std::make_pair(ll, ur);
  }

  // -----------------------------------------------------------------------------
  IGeometry::LengthUnit getUnits() const override
  {
    return m_Geometry.getUnits();
  }
};
} // namespace complex
