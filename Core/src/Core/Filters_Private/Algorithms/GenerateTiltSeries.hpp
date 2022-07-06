#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  GenerateTiltSeriesInputValues inputValues;

  inputValues.RotationAxis = filterArgs.value<ChoicesParameter::ValueType>(k_RotationAxis_Key);
  inputValues.RotationLimits = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationLimits_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.InputDataArrayPath = filterArgs.value<DataPath>(k_InputDataArrayPath_Key);
  inputValues.OutputPrefix = filterArgs.value<StringParameter::ValueType>(k_OutputPrefix_Key);

  return GenerateTiltSeries(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT GenerateTiltSeriesInputValues
{
  ChoicesParameter::ValueType RotationAxis;
  VectorFloat32Parameter::ValueType RotationLimits;
  VectorFloat32Parameter::ValueType Spacing;
  DataPath InputDataArrayPath;
  StringParameter::ValueType OutputPrefix;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT GenerateTiltSeries
{
public:
  GenerateTiltSeries(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GenerateTiltSeriesInputValues* inputValues);
  ~GenerateTiltSeries() noexcept;

  GenerateTiltSeries(const GenerateTiltSeries&) = delete;
  GenerateTiltSeries(GenerateTiltSeries&&) noexcept = delete;
  GenerateTiltSeries& operator=(const GenerateTiltSeries&) = delete;
  GenerateTiltSeries& operator=(GenerateTiltSeries&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateTiltSeriesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
