#include "GeometryUtilities.hpp"

#include "complex/Common/Array.hpp"

using namespace complex;

namespace
{
constexpr float32 k_PartitionEdgePadding = 0.000001;
const Point3Df k_Padding(k_PartitionEdgePadding, k_PartitionEdgePadding, k_PartitionEdgePadding);
}

Result<FloatVec3> GeometryUtilities::CalculatePartitionLengthsByPartitionCount(const INodeGeometry0D& geometry, const SizeVec3& numberOfPartitionsPerAxis)
{
  BoundingBox3Df boundingBox = geometry.getBoundingBox();
  if(!boundingBox.isValid())
  {
    return {};
  }
  return GeometryUtilities::CalculatePartitionLengthsOfBoundingBox({(boundingBox.getMinPoint() - k_Padding), (boundingBox.getMaxPoint() + k_Padding)}, numberOfPartitionsPerAxis);
}

Result<FloatVec3> GeometryUtilities::CalculatePartitionLengthsByPartitionCount(const ImageGeom& geometry, const SizeVec3& numberOfPartitionsPerAxis)
{
  SizeVec3 dims = geometry.getDimensions();
  FloatVec3 spacing = geometry.getSpacing();
  float32 lengthX = static_cast<float32>(dims.getX()) / static_cast<float32>(numberOfPartitionsPerAxis.getX()) * spacing[0];
  float32 lengthY = static_cast<float32>(dims.getY()) / static_cast<float32>(numberOfPartitionsPerAxis.getY()) * spacing[1];
  float32 lengthZ = static_cast<float32>(dims.getZ()) / static_cast<float32>(numberOfPartitionsPerAxis.getZ()) * spacing[2];
  return {FloatVec3(lengthX, lengthY, lengthZ)};
}

Result<FloatVec3> GeometryUtilities::CalculatePartitionLengthsByPartitionCount(const RectGridGeom& geometry, const SizeVec3& numberOfPartitionsPerAxis)
{
  const Float32Array* xBounds = geometry.getXBounds();
  const Float32Array* yBounds = geometry.getYBounds();
  const Float32Array* zBounds = geometry.getZBounds();

  if(xBounds == nullptr)
  {
    return MakeErrorResult<FloatVec3>(-4000, "Unable to calculate partition lengths using the partition count - X Bounds array is not available.");
  }

  if(yBounds == nullptr)
  {
    return MakeErrorResult<FloatVec3>(-4001, "Unable to calculate partition lengths using the partition count - Y Bounds array is not available.");
  }

  if(zBounds == nullptr)
  {
    return MakeErrorResult<FloatVec3>(-4002, "Unable to calculate partition lengths using the partition count - Z Bounds array is not available.");
  }

  if(xBounds->getSize() == 0)
  {
    return MakeErrorResult<FloatVec3>(-4003, "Unable to calculate partition lengths using the partition count - X Bounds array is empty.");
  }

  if(yBounds->getSize() == 0)
  {
    return MakeErrorResult<FloatVec3>(-4004, "Unable to calculate partition lengths using the partition count - Y Bounds array is empty.");
  }

  if(zBounds->getSize() == 0)
  {
    return MakeErrorResult<FloatVec3>(-4005, "Unable to calculate partition lengths using the partition count - Z Bounds array is empty.");
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

  return {lengthPerPartition};
}

Result<FloatVec3> GeometryUtilities::CalculateNodeBasedPartitionSchemeOrigin(const INodeGeometry0D& geometry)
{
  BoundingBox3Df boundingBox = geometry.getBoundingBox();
  if(!boundingBox.isValid())
  {
    return {};
  }
  return {FloatVec3(boundingBox.getMinPoint() - k_Padding)};
}

Result<FloatVec3> GeometryUtilities::CalculatePartitionLengthsOfBoundingBox(const BoundingBox3Df& boundingBox, const SizeVec3& numberOfPartitionsPerAxis)
{
  auto min = boundingBox.getMinPoint();
  auto max = boundingBox.getMaxPoint();
  // Calculate the length per partition for each dimension, and set it into the partitioning scheme image geometry
  float32 lengthX = ((max[0] - min[0]) / static_cast<float32>(numberOfPartitionsPerAxis.getX()));
  float32 lengthY = ((max[1] - min[1]) / static_cast<float32>(numberOfPartitionsPerAxis.getY()));
  float32 lengthZ = ((max[2] - min[2]) / static_cast<float32>(numberOfPartitionsPerAxis.getZ()));
  FloatVec3 lengthPerPartition = {lengthX, lengthY, lengthZ};
  return {lengthPerPartition};
}
