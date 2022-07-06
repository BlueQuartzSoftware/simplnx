#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  AdaptiveAlignmentMutualInformationInputValues inputValues;

  inputValues.GlobalCorrection = filterArgs.value<ChoicesParameter::ValueType>(k_GlobalCorrection_Key);
  inputValues.ImageDataArrayPath = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);
  inputValues.ShiftX = filterArgs.value<float32>(k_ShiftX_Key);
  inputValues.ShiftY = filterArgs.value<float32>(k_ShiftY_Key);
  inputValues.IgnoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  inputValues.MisorientationTolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  inputValues.UseGoodVoxels = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.GoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  return AdaptiveAlignmentMutualInformation(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT AdaptiveAlignmentMutualInformationInputValues
{
  ChoicesParameter::ValueType GlobalCorrection;
  DataPath ImageDataArrayPath;
  float32 ShiftX;
  float32 ShiftY;
  MultiArraySelectionParameter::ValueType IgnoredDataArrayPaths;
  float32 MisorientationTolerance;
  bool UseGoodVoxels;
  DataPath QuatsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath GoodVoxelsArrayPath;
  DataPath CrystalStructuresArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT AdaptiveAlignmentMutualInformation
{
public:
  AdaptiveAlignmentMutualInformation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                     AdaptiveAlignmentMutualInformationInputValues* inputValues);
  ~AdaptiveAlignmentMutualInformation() noexcept;

  AdaptiveAlignmentMutualInformation(const AdaptiveAlignmentMutualInformation&) = delete;
  AdaptiveAlignmentMutualInformation(AdaptiveAlignmentMutualInformation&&) noexcept = delete;
  AdaptiveAlignmentMutualInformation& operator=(const AdaptiveAlignmentMutualInformation&) = delete;
  AdaptiveAlignmentMutualInformation& operator=(AdaptiveAlignmentMutualInformation&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AdaptiveAlignmentMutualInformationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
