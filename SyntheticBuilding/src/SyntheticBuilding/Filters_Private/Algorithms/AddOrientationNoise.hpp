#pragma once

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  AddOrientationNoiseInputValues inputValues;

  inputValues.Magnitude = filterArgs.value<float32>(k_Magnitude_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);

  return AddOrientationNoise(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SYNTHETICBUILDING_EXPORT AddOrientationNoiseInputValues
{
  float32 Magnitude;
  DataPath CellEulerAnglesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SYNTHETICBUILDING_EXPORT AddOrientationNoise
{
public:
  AddOrientationNoise(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AddOrientationNoiseInputValues* inputValues);
  ~AddOrientationNoise() noexcept;

  AddOrientationNoise(const AddOrientationNoise&) = delete;
  AddOrientationNoise(AddOrientationNoise&&) noexcept = delete;
  AddOrientationNoise& operator=(const AddOrientationNoise&) = delete;
  AddOrientationNoise& operator=(AddOrientationNoise&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AddOrientationNoiseInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
