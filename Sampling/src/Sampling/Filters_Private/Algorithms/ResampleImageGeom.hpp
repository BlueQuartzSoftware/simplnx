#pragma once

#include "Sampling/Sampling_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ResampleImageGeomInputValues inputValues;

  inputValues.CurrentGeomtryInfo = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_CurrentGeomtryInfo_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.NewGeomtryInfo = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_NewGeomtryInfo_Key);
  inputValues.RenumberFeatures = filterArgs.value<bool>(k_RenumberFeatures_Key);
  inputValues.SaveAsNewDataContainer = filterArgs.value<bool>(k_SaveAsNewDataContainer_Key);
  inputValues.CellAttributeMatrixPath = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CellFeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  inputValues.NewDataContainerPath = filterArgs.value<DataPath>(k_NewDataContainerPath_Key);

  return ResampleImageGeom(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SAMPLING_EXPORT ResampleImageGeomInputValues
{
  <<<NOT_IMPLEMENTED>>> CurrentGeomtryInfo;
  VectorFloat32Parameter::ValueType Spacing;
  <<<NOT_IMPLEMENTED>>> NewGeomtryInfo;
  bool RenumberFeatures;
  bool SaveAsNewDataContainer;
  DataPath CellAttributeMatrixPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellFeatureAttributeMatrixPath;
  DataPath NewDataContainerPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SAMPLING_EXPORT ResampleImageGeom
{
public:
  ResampleImageGeom(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ResampleImageGeomInputValues* inputValues);
  ~ResampleImageGeom() noexcept;

  ResampleImageGeom(const ResampleImageGeom&) = delete;
  ResampleImageGeom(ResampleImageGeom&&) noexcept = delete;
  ResampleImageGeom& operator=(const ResampleImageGeom&) = delete;
  ResampleImageGeom& operator=(ResampleImageGeom&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ResampleImageGeomInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
