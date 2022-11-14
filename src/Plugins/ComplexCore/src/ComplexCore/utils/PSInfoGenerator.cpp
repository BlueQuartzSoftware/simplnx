#include "PSInfoGenerator.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
PSInfoGenerator::PSInfoGenerator(SizeVec3 numOfPartitionsPerAxis)
: m_NumberOfPartitionsPerAxis(numOfPartitionsPerAxis)
{
}

// -----------------------------------------------------------------------------
const complex::SizeVec3& PSInfoGenerator::numOfPartitionsPerAxis() const
{
  return m_NumberOfPartitionsPerAxis;
}

// -----------------------------------------------------------------------------
FloatVec3 PSInfoGenerator::calculatePartitionLengthsUsingBounds(const FloatVec3& ll, const FloatVec3& ur) const
{
  // Calculate the total distance for each dimension
  float totalXDist = ur[0] - ll[0];
  float totalYDist = ur[1] - ll[1];
  float totalZDist = ur[2] - ll[2];

  // Calculate the length per partition for each dimension, and set it into the partitioning scheme image geometry
  float lengthX = (totalXDist / m_NumberOfPartitionsPerAxis.getX());
  float lengthY = (totalYDist / m_NumberOfPartitionsPerAxis.getY());
  float lengthZ = (totalZDist / m_NumberOfPartitionsPerAxis.getZ());
  FloatVec3 lengthPerPartition = {lengthX, lengthY, lengthZ};
  return {lengthPerPartition};
}
