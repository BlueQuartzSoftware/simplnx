#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  MapPointCloudToRegularGridInputValues inputValues;

  inputValues.SamplingGridType = filterArgs.value<ChoicesParameter::ValueType>(k_SamplingGridType_Key);
  inputValues.GridDimensions = filterArgs.value<VectorInt32Parameter::ValueType>(k_GridDimensions_Key);
  inputValues.ImageDataContainerPath = filterArgs.value<DataPath>(k_ImageDataContainerPath_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.VoxelIndicesArrayPath = filterArgs.value<DataPath>(k_VoxelIndicesArrayPath_Key);
  inputValues.CreatedImageDataContainerName = filterArgs.value<DataPath>(k_CreatedImageDataContainerName_Key);

  return MapPointCloudToRegularGrid(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT MapPointCloudToRegularGridInputValues
{
  ChoicesParameter::ValueType SamplingGridType;
  VectorInt32Parameter::ValueType GridDimensions;
  DataPath ImageDataContainerPath;
  DataPath DataContainerName;
  bool UseMask;
  DataPath MaskArrayPath;
  DataPath VoxelIndicesArrayPath;
  DataPath CreatedImageDataContainerName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT MapPointCloudToRegularGrid
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

} // namespace complex
