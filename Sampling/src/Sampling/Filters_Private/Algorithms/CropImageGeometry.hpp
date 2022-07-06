#pragma once

#include "Sampling/Sampling_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CropImageGeometryInputValues inputValues;

  inputValues.XMin = filterArgs.value<int32>(k_XMin_Key);
  inputValues.YMin = filterArgs.value<int32>(k_YMin_Key);
  inputValues.ZMin = filterArgs.value<int32>(k_ZMin_Key);
  inputValues.XMax = filterArgs.value<int32>(k_XMax_Key);
  inputValues.YMax = filterArgs.value<int32>(k_YMax_Key);
  inputValues.ZMax = filterArgs.value<int32>(k_ZMax_Key);
  inputValues.OldBoxDimensions = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_OldBoxDimensions_Key);
  inputValues.NewBoxDimensions = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_NewBoxDimensions_Key);
  inputValues.UpdateOrigin = filterArgs.value<bool>(k_UpdateOrigin_Key);
  inputValues.SaveAsNewDataContainer = filterArgs.value<bool>(k_SaveAsNewDataContainer_Key);
  inputValues.NewDataContainerName = filterArgs.value<DataPath>(k_NewDataContainerName_Key);
  inputValues.CellAttributeMatrixPath = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);
  inputValues.RenumberFeatures = filterArgs.value<bool>(k_RenumberFeatures_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellFeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);

  return CropImageGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SAMPLING_EXPORT CropImageGeometryInputValues
{
  int32 XMin;
  int32 YMin;
  int32 ZMin;
  int32 XMax;
  int32 YMax;
  int32 ZMax;
  <<<NOT_IMPLEMENTED>>> OldBoxDimensions;
  <<<NOT_IMPLEMENTED>>> NewBoxDimensions;
  bool UpdateOrigin;
  bool SaveAsNewDataContainer;
  DataPath NewDataContainerName;
  DataPath CellAttributeMatrixPath;
  bool RenumberFeatures;
  DataPath FeatureIdsArrayPath;
  DataPath CellFeatureAttributeMatrixPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SAMPLING_EXPORT CropImageGeometry
{
public:
  CropImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CropImageGeometryInputValues* inputValues);
  ~CropImageGeometry() noexcept;

  CropImageGeometry(const CropImageGeometry&) = delete;
  CropImageGeometry(CropImageGeometry&&) noexcept = delete;
  CropImageGeometry& operator=(const CropImageGeometry&) = delete;
  CropImageGeometry& operator=(CropImageGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CropImageGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
