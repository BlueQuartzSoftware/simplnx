#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{
/**
 * @struct VoxelizePointCloudInputValues
 * @brief Input parameters for the VoxelizePointCloud algorithm.
 */
struct SIMPLNXCORE_EXPORT VoxelizePointCloudInputValues
{
  ChoicesParameter::ValueType PartitioningMode;
  VectorInt32Parameter::ValueType NumberOfCellsPerAxis;
  VectorFloat32Parameter::ValueType PartitionGridOrigin;
  VectorFloat32Parameter::ValueType CellLength;
  VectorFloat32Parameter::ValueType MinGridCoord;
  VectorFloat32Parameter::ValueType MaxGridCoord;
  DataPath PointCloudGeometryPath;
  DataPath OutputGeometryPath;
  std::string MaskName;
  DataPath NewGeometryPath;
};

/**
 * @class VoxelizePointCloud
 * @brief This filter creates a grid mask of a point cloud in an existing Image Geometry or optionally
 * creates a new image geometry that fits the point cloud and includes the mask
 */
class SIMPLNXCORE_EXPORT VoxelizePointCloud
{
public:
  VoxelizePointCloud(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, VoxelizePointCloudInputValues* inputValues);
  ~VoxelizePointCloud() noexcept;

  VoxelizePointCloud(const VoxelizePointCloud&) = delete;
  VoxelizePointCloud(VoxelizePointCloud&&) noexcept = delete;
  VoxelizePointCloud& operator=(const VoxelizePointCloud&) = delete;
  VoxelizePointCloud& operator=(VoxelizePointCloud&&) noexcept = delete;

  /**
   * @brief Executes the voxelization algorithm.
   * @return A Result<> containing any errors or warnings produced during execution.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const VoxelizePointCloudInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
