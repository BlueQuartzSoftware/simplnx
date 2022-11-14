#include "PSImageGeomInfoGenerator.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
PSImageGeomInfoGenerator::PSImageGeomInfoGenerator(const ImageGeom& geometry, const SizeVec3& numOfPartitionsPerAxis)
: PSInfoGenerator(numOfPartitionsPerAxis)
, m_Geometry(geometry)
{
}

// -----------------------------------------------------------------------------
std::optional<FloatVec3> PSImageGeomInfoGenerator::getOrigin() const
{
  return m_Geometry.getOrigin();
}

// -----------------------------------------------------------------------------
std::optional<FloatVec3> PSImageGeomInfoGenerator::getPartitionLength() const
{
  SizeVec3 dims = m_Geometry.getDimensions();
  FloatVec3 spacing = m_Geometry.getSpacing();
  float lengthX = static_cast<float>(dims.getX()) / numOfPartitionsPerAxis().getX() * spacing[0];
  float lengthY = static_cast<float>(dims.getY()) / numOfPartitionsPerAxis().getY() * spacing[1];
  float lengthZ = static_cast<float>(dims.getZ()) / numOfPartitionsPerAxis().getZ() * spacing[2];
  FloatVec3 lengthPerPartition = {lengthX, lengthY, lengthZ};
  return {lengthPerPartition};
}

// -----------------------------------------------------------------------------
IGeometry::LengthUnit PSImageGeomInfoGenerator::getUnits() const
{
  return m_Geometry.getUnits();
}
