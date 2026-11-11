#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <array>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ApproximatePointCloudHullInputValues
{
  VectorFloat32Parameter::ValueType GridResolution;
  GeometrySelectionParameter::ValueType InputVertexGeometryPath;
  UInt64Parameter::ValueType MinEmptyNeighbors;
  DataGroupCreationParameter::ValueType OutputVertexGeometryPath;
};

/**
 * @class ApproximatePointCloudHull
 * @brief This algorithm implements support code for the ApproximatePointCloudHullFilter
 */

class SIMPLNXCORE_EXPORT ApproximatePointCloudHull
{
public:
  ApproximatePointCloudHull(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ApproximatePointCloudHullInputValues* inputValues);
  ~ApproximatePointCloudHull() noexcept;

  ApproximatePointCloudHull(const ApproximatePointCloudHull&) = delete;
  ApproximatePointCloudHull(ApproximatePointCloudHull&&) noexcept = delete;
  ApproximatePointCloudHull& operator=(const ApproximatePointCloudHull&) = delete;
  ApproximatePointCloudHull& operator=(ApproximatePointCloudHull&&) noexcept = delete;

  /**
   * @brief Checks final voxel bounds and container sizes without allocating a grid.
   * @details The occupied voxel bounds are padded by an empty one-voxel border on each side in integer arithmetic, so the border
   * cannot be lost to float rounding at large voxel indices.
   * @param minimumVoxelCoordinates Gives the inclusive lower voxel coordinates of the occupied points in XYZ order, before the border.
   * @param maximumVoxelCoordinates Gives the inclusive upper voxel coordinates of the occupied points in XYZ order, before the border.
   * @return Dimensions of the padded grid, or a bounds, index or container representation error.
   */
  static Result<SizeVec3> ValidateGridRepresentation(const std::array<float64, 3>& minimumVoxelCoordinates, const std::array<float64, 3>& maximumVoxelCoordinates);

  /**
   * @brief Approximates the hull through a voxel grid.
   * @pre Grid Resolution has three finite positive components; Minimum Number of Empty Neighbors is at most 26.
   * @return Final bounds, grid representation or output resize errors.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ApproximatePointCloudHullInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
