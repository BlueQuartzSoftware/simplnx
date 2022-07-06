#pragma once

#include "UCSBUtilities/UCSBUtilities_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  GenerateMisorientationColorsInputValues inputValues;

  inputValues.ReferenceAxis = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ReferenceAxis_Key);
  inputValues.ReferenceAngle = filterArgs.value<float32>(k_ReferenceAngle_Key);
  inputValues.UseGoodVoxels = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.GoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.MisorientationColorArrayName = filterArgs.value<DataPath>(k_MisorientationColorArrayName_Key);

  return GenerateMisorientationColors(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct UCSBUTILITIES_EXPORT GenerateMisorientationColorsInputValues
{
  VectorFloat32Parameter::ValueType ReferenceAxis;
  float32 ReferenceAngle;
  bool UseGoodVoxels;
  DataPath QuatsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath GoodVoxelsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath MisorientationColorArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class UCSBUTILITIES_EXPORT GenerateMisorientationColors
{
public:
  GenerateMisorientationColors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GenerateMisorientationColorsInputValues* inputValues);
  ~GenerateMisorientationColors() noexcept;

  GenerateMisorientationColors(const GenerateMisorientationColors&) = delete;
  GenerateMisorientationColors(GenerateMisorientationColors&&) noexcept = delete;
  GenerateMisorientationColors& operator=(const GenerateMisorientationColors&) = delete;
  GenerateMisorientationColors& operator=(GenerateMisorientationColors&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateMisorientationColorsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
