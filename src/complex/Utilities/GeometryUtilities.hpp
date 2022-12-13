#pragma once

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"

namespace complex
{
const float32 k_PartitionEdgePadding = 0.000001;

/**
 * @brief Calculates the X,Y,Z partition length if the given bounding box were partitioned into equal numberOfPartitionsPerAxis partitions.
 * @param boundingBox The bounding box
 * @param numberOfPartitionsPerAxis The number of partitions in each axis
 */
FloatVec3 CalculatePartitionLengthsOfBoundingBox(const INodeGeometry0D::BoundingBox& boundingBox, const SizeVec3& numberOfPartitionsPerAxis)
{
  // Calculate the length per partition for each dimension, and set it into the partitioning scheme image geometry
  float32 lengthX = ((boundingBox.second[0] - boundingBox.first[0]) / static_cast<float32>(numberOfPartitionsPerAxis.getX()));
  float32 lengthY = ((boundingBox.second[1] - boundingBox.first[1]) / static_cast<float32>(numberOfPartitionsPerAxis.getY()));
  float32 lengthZ = ((boundingBox.second[2] - boundingBox.first[2]) / static_cast<float32>(numberOfPartitionsPerAxis.getZ()));
  FloatVec3 lengthPerPartition = {lengthX, lengthY, lengthZ};
  return {lengthPerPartition};
}

/**
 * @brief Calculates the X,Y,Z partition length for a given geometry if the geometry were partitioned into equal numberOfPartitionsPerAxis partitions.
 * @param geometry The geometry to be partitioned
 * @param numberOfPartitionsPerAxis The number of partitions in each axis
 */
template <typename Geom>
std::optional<FloatVec3> CalculatePartitionLengthsByPartitionCount(const Geom& geometry, const SizeVec3& numberOfPartitionsPerAxis)
{
  std::optional<INodeGeometry0D::BoundingBox> boundingBox = geometry.getBoundingBox();
  if(!boundingBox.has_value())
  {
    return {};
  }

  // Pad the edges
  boundingBox->first[0] -= k_PartitionEdgePadding;
  boundingBox->first[1] -= k_PartitionEdgePadding;
  boundingBox->first[2] -= k_PartitionEdgePadding;
  boundingBox->second[0] += k_PartitionEdgePadding;
  boundingBox->second[1] += k_PartitionEdgePadding;
  boundingBox->second[2] += k_PartitionEdgePadding;

  return CalculatePartitionLengthsOfBoundingBox(*boundingBox, numberOfPartitionsPerAxis);
}

/**
 * @brief Calculates the X,Y,Z partition length for a given Image geometry if the geometry were partitioned into equal numberOfPartitionsPerAxis partitions.
 * @param geometry The geometry to be partitioned
 * @param numberOfPartitionsPerAxis The number of partitions in each axis
 */
template <>
std::optional<FloatVec3> CalculatePartitionLengthsByPartitionCount(const ImageGeom& geometry, const SizeVec3& numberOfPartitionsPerAxis)
{
  SizeVec3 dims = geometry.getDimensions();
  FloatVec3 spacing = geometry.getSpacing();
  float32 lengthX = static_cast<float32>(dims.getX()) / static_cast<float32>(numberOfPartitionsPerAxis.getX()) * spacing[0];
  float32 lengthY = static_cast<float32>(dims.getY()) / static_cast<float32>(numberOfPartitionsPerAxis.getY()) * spacing[1];
  float32 lengthZ = static_cast<float32>(dims.getZ()) / static_cast<float32>(numberOfPartitionsPerAxis.getZ()) * spacing[2];
  FloatVec3 lengthPerPartition = {lengthX, lengthY, lengthZ};
  return {lengthPerPartition};
}

/**
 * @brief Calculates the X,Y,Z partition length for a given RectGrid geometry if the geometry were partitioned into equal numberOfPartitionsPerAxis partitions.
 * @param geometry The geometry to be partitioned
 * @param numberOfPartitionsPerAxis The number of partitions in each axis
 */
template <>
std::optional<FloatVec3> CalculatePartitionLengthsByPartitionCount(const RectGridGeom& geometry, const SizeVec3& numberOfPartitionsPerAxis)
{
  const Float32Array* xBounds = geometry.getXBounds();
  const Float32Array* yBounds = geometry.getYBounds();
  const Float32Array* zBounds = geometry.getZBounds();

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
  float32 maxX = xStore.getValue(xBounds->getNumberOfTuples() - 1);
  lengthPerPartition.setX(maxX / static_cast<float32>(numberOfPartitionsPerAxis.getX()));

  const AbstractDataStore<float32>& yStore = yBounds->getDataStoreRef();
  float32 maxY = yStore.getValue(yBounds->getNumberOfTuples() - 1);
  lengthPerPartition.setY(maxY / static_cast<float32>(numberOfPartitionsPerAxis.getY()));

  const AbstractDataStore<float32>& zStore = yBounds->getDataStoreRef();
  float32 maxZ = zStore.getValue(zBounds->getNumberOfTuples() - 1);
  lengthPerPartition.setZ(maxZ / static_cast<float32>(numberOfPartitionsPerAxis.getZ()));

  return lengthPerPartition;
}

/**
 * @brief Calculates the X,Y,Z partition scheme origin for a given node-based geometry using the geometry's bounding box.
 * @param geometry The geometry whose bounding box origin will be calculated
 */
template <typename Geom>
std::optional<FloatVec3> CalculateNodeBasedPartitionSchemeOrigin(const INodeGeometry0D& geometry)
{
  std::optional<INodeGeometry0D::BoundingBox> boundingBox = geometry.getBoundingBox();
  if(!boundingBox.has_value())
  {
    return {};
  }

  // Pad the edges
  boundingBox->first[0] -= k_PartitionEdgePadding;
  boundingBox->first[1] -= k_PartitionEdgePadding;
  boundingBox->first[2] -= k_PartitionEdgePadding;
  boundingBox->second[0] += k_PartitionEdgePadding;
  boundingBox->second[1] += k_PartitionEdgePadding;
  boundingBox->second[2] += k_PartitionEdgePadding;

  return (*boundingBox).first;
}
} // namespace complex
