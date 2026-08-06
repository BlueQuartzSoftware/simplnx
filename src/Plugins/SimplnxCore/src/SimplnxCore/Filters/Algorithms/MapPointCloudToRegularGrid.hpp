#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT MapPointCloudToRegularGridInputValues
{
  bool UseMask;
  DataPath ImageGeomPath;
  DataPath MaskArrayPath;
  DataPath VertexGeomPath;
  DataPath VoxelIndicesPath;
  uint64 OutOfBoundsValue;
  ChoicesParameter::ValueType OutOfBoundsHandling;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT MapPointCloudToRegularGrid
{
public:
  MapPointCloudToRegularGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MapPointCloudToRegularGridInputValues* inputValues);
  ~MapPointCloudToRegularGrid() noexcept;

  MapPointCloudToRegularGrid(const MapPointCloudToRegularGrid&) = delete;
  MapPointCloudToRegularGrid(MapPointCloudToRegularGrid&&) noexcept = delete;
  MapPointCloudToRegularGrid& operator=(const MapPointCloudToRegularGrid&) = delete;
  MapPointCloudToRegularGrid& operator=(MapPointCloudToRegularGrid&&) noexcept = delete;

  Result<> operator()();
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const MapPointCloudToRegularGridInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
