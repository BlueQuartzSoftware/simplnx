#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateAMScanPathsInputValues inputValues;

  inputValues.StripeWidth = filterArgs.value<float32>(k_StripeWidth_Key);
  inputValues.HatchSpacing = filterArgs.value<float32>(k_HatchSpacing_Key);
  inputValues.Power = filterArgs.value<float32>(k_Power_Key);
  inputValues.Speed = filterArgs.value<float32>(k_Speed_Key);
  inputValues.CADSliceDataContainerName = filterArgs.value<DataPath>(k_CADSliceDataContainerName_Key);
  inputValues.CADSliceIdsArrayPath = filterArgs.value<DataPath>(k_CADSliceIdsArrayPath_Key);
  inputValues.CADRegionIdsArrayPath = filterArgs.value<DataPath>(k_CADRegionIdsArrayPath_Key);
  inputValues.HatchDataContainerName = filterArgs.value<StringParameter::ValueType>(k_HatchDataContainerName_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  inputValues.HatchAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_HatchAttributeMatrixName_Key);
  inputValues.TimeArrayName = filterArgs.value<StringParameter::ValueType>(k_TimeArrayName_Key);
  inputValues.RegionIdsArrayName = filterArgs.value<StringParameter::ValueType>(k_RegionIdsArrayName_Key);
  inputValues.PowersArrayName = filterArgs.value<StringParameter::ValueType>(k_PowersArrayName_Key);

  return CreateAMScanPaths(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CreateAMScanPathsInputValues
{
  float32 StripeWidth;
  float32 HatchSpacing;
  float32 Power;
  float32 Speed;
  DataPath CADSliceDataContainerName;
  DataPath CADSliceIdsArrayPath;
  DataPath CADRegionIdsArrayPath;
  StringParameter::ValueType HatchDataContainerName;
  StringParameter::ValueType VertexAttributeMatrixName;
  StringParameter::ValueType HatchAttributeMatrixName;
  StringParameter::ValueType TimeArrayName;
  StringParameter::ValueType RegionIdsArrayName;
  StringParameter::ValueType PowersArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMPLNXCORE_EXPORT CreateAMScanPaths
{
public:
  CreateAMScanPaths(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateAMScanPathsInputValues* inputValues);
  ~CreateAMScanPaths() noexcept;

  CreateAMScanPaths(const CreateAMScanPaths&) = delete;
  CreateAMScanPaths(CreateAMScanPaths&&) noexcept = delete;
  CreateAMScanPaths& operator=(const CreateAMScanPaths&) = delete;
  CreateAMScanPaths& operator=(CreateAMScanPaths&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateAMScanPathsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
