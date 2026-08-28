#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @struct VoxelizePointCloudInputValues
 * @brief Input parameters for the VoxelizePointCloud algorithm.
 */
struct SIMPLNXCORE_EXPORT VoxelizePointCloudInputValues
{
  bool UseExistingGeom;          ///< If true, map points into an existing grid geometry; if false, auto-size a new ImageGeom around the point cloud.
  DataPath PointCloudGeometryPath; ///< Path to the input node-based geometry whose vertex positions are voxelized.
  DataPath OutputGeometryPath;   ///< Path to the existing destination grid geometry (Image or RectGrid). Used only when UseExistingGeom is true.
  std::string MaskName;          ///< Name of the output UInt8 voxel mask array created inside the destination geometry's cell Attribute Matrix.
  DataPath NewGeometryPath;      ///< Path at which the new auto-sized ImageGeom is created. Used only when UseExistingGeom is false.
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
