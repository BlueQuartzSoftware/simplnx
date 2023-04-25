#pragma once

#include "complex/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"

namespace complex::GeometryUtilities
{
/**
 * @brief Calculates the X,Y,Z partition length for a given geometry if the geometry were partitioned into equal numberOfPartitionsPerAxis partitions.
 * @param geometry The geometry to be partitioned
 * @param numberOfPartitionsPerAxis The number of partitions in each axis
 */
Result<FloatVec3> CalculatePartitionLengthsByPartitionCount(const INodeGeometry0D& geometry, const SizeVec3& numberOfPartitionsPerAxis);

/**
 * @brief Calculates the X,Y,Z partition length for a given Image geometry if the geometry were partitioned into equal numberOfPartitionsPerAxis partitions.
 * @param geometry The geometry to be partitioned
 * @param numberOfPartitionsPerAxis The number of partitions in each axis
 */
Result<FloatVec3> CalculatePartitionLengthsByPartitionCount(const ImageGeom& geometry, const SizeVec3& numberOfPartitionsPerAxis);

/**
 * @brief Calculates the X,Y,Z partition length for a given RectGrid geometry if the geometry were partitioned into equal numberOfPartitionsPerAxis partitions.
 * @param geometry The geometry to be partitioned
 * @param numberOfPartitionsPerAxis The number of partitions in each axis
 */
Result<FloatVec3> CalculatePartitionLengthsByPartitionCount(const RectGridGeom& geometry, const SizeVec3& numberOfPartitionsPerAxis);

/**
 * @brief Calculates the X,Y,Z partition scheme origin for a given node-based geometry using the geometry's bounding box.
 * @param geometry The geometry whose bounding box origin will be calculated
 */
Result<FloatVec3> CalculateNodeBasedPartitionSchemeOrigin(const INodeGeometry0D& geometry);

/**
 * @brief Calculates the X,Y,Z partition length if the given bounding box were partitioned into equal numberOfPartitionsPerAxis partitions.
 * @param boundingBox The bounding box
 * @param numberOfPartitionsPerAxis The number of partitions in each axis
 */
Result<FloatVec3> CalculatePartitionLengthsOfBoundingBox(const BoundingBox3Df& boundingBox, const SizeVec3& numberOfPartitionsPerAxis);
} // namespace complex::GeometryUtilities
