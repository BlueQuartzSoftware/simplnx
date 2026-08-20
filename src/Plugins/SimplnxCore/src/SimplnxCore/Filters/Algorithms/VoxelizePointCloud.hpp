#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT VoxelizePointCloudInputValues
{
  bool UseExistingGeom;
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

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const VoxelizePointCloudInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
