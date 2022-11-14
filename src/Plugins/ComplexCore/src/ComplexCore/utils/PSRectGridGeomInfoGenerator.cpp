#include "PSRectGridGeomInfoGenerator.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
PSRectGridGeomInfoGenerator::PSRectGridGeomInfoGenerator(const RectGridGeom& geometry, const SizeVec3& numOfPartitionsPerAxis)
: PSInfoGenerator(numOfPartitionsPerAxis)
, m_Geometry(geometry)
{
}

// -----------------------------------------------------------------------------
std::optional<FloatVec3> PSRectGridGeomInfoGenerator::getOrigin() const
{
  const Float32Array* xBounds = m_Geometry.getXBounds();
  const Float32Array* yBounds = m_Geometry.getYBounds();
  const Float32Array* zBounds = m_Geometry.getZBounds();

  if(xBounds == nullptr)
  {
    return {};
  }

  if(yBounds == nullptr)
  {
    return {};
  }

  if(zBounds == nullptr)
  {
    return {};
  }

  if(xBounds->getSize() == 0)
  {
    return {};
  }

  if(yBounds->getSize() == 0)
  {
    return {};
  }

  if(zBounds->getSize() == 0)
  {
    return {};
  }

  FloatVec3 origin = {0.0f, 0.0f, 0.0f};
  origin.setX(xBounds->at(0));
  origin.setY(yBounds->at(0));
  origin.setZ(zBounds->at(0));
  return origin;
}

// -----------------------------------------------------------------------------
std::optional<FloatVec3> PSRectGridGeomInfoGenerator::getPartitionLength() const
{
  const Float32Array* xBounds = m_Geometry.getXBounds();
  const Float32Array* yBounds = m_Geometry.getYBounds();
  const Float32Array* zBounds = m_Geometry.getZBounds();

  if(xBounds == nullptr)
  {
    return {};
  }

  if(yBounds == nullptr)
  {
    return {};
  }

  if(zBounds == nullptr)
  {
    return {};
  }

  if(xBounds->getSize() == 0)
  {
    return {};
  }

  if(yBounds->getSize() == 0)
  {
    return {};
  }

  if(zBounds->getSize() == 0)
  {
    return {};
  }

  FloatVec3 lengthPerPartition = {0.0f, 0.0f, 0.0f};

  const AbstractDataStore<float32>& xStore = xBounds->getDataStoreRef();
  float maxX = xStore.getValue(xBounds->getNumberOfTuples() - 1);
  lengthPerPartition.setX(maxX / numOfPartitionsPerAxis().getX());

  const AbstractDataStore<float32>& yStore = yBounds->getDataStoreRef();
  float maxY = yStore.getValue(yBounds->getNumberOfTuples() - 1);
  lengthPerPartition.setY(maxY / numOfPartitionsPerAxis().getY());

  const AbstractDataStore<float32>& zStore = yBounds->getDataStoreRef();
  float maxZ = zStore.getValue(zBounds->getNumberOfTuples() - 1);
  lengthPerPartition.setZ(maxZ / numOfPartitionsPerAxis().getZ());

  return lengthPerPartition;
}

// -----------------------------------------------------------------------------
IGeometry::LengthUnit PSRectGridGeomInfoGenerator::getUnits() const
{
  return m_Geometry.getUnits();
}
