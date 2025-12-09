#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT InterpolatePointCloudToRegularGridInputValues
{
  bool storeKernelDistances;
  uint64 interpolationTechnique;
  DataPath vertexGeomPath;
  DataPath imageGeomPath;
  std::string interpolatedGroupName;
  std::vector<DataPath> interpolatedDataPaths;
  std::vector<DataPath> copyDataPaths;
  DataPath voxelIndicesPath;
  std::vector<float32> kernelSize;
  std::vector<float32> sigmas;
  std::string kernelDistanceArrayName;
  bool useMask;
  DataPath maskDataPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT InterpolatePointCloudToRegularGrid
{
public:
  InterpolatePointCloudToRegularGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                     InterpolatePointCloudToRegularGridInputValues* inputValues);
  ~InterpolatePointCloudToRegularGrid() noexcept;

  InterpolatePointCloudToRegularGrid(const InterpolatePointCloudToRegularGrid&) = delete;
  InterpolatePointCloudToRegularGrid(InterpolatePointCloudToRegularGrid&&) noexcept = delete;
  InterpolatePointCloudToRegularGrid& operator=(const InterpolatePointCloudToRegularGrid&) = delete;
  InterpolatePointCloudToRegularGrid& operator=(InterpolatePointCloudToRegularGrid&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  static constexpr uint64 k_Uniform = 0;
  static constexpr uint64 k_Gaussian = 1;

private:
  DataStructure& m_DataStructure;
  const InterpolatePointCloudToRegularGridInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
