#pragma once

#include "ProgWorkshop/ProgWorkshop_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  Lesson5InputValues inputValues;

  inputValues.InputDataArrayPath = filterArgs.value<DataPath>(k_InputDataArrayPath_Key);
  inputValues.OutputDataArrayPath = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  inputValues.Value = filterArgs.value<float32>(k_Value_Key);
  inputValues.Operator = filterArgs.value<ChoicesParameter::ValueType>(k_Operator_Key);
  inputValues.Selection = filterArgs.value<bool>(k_Selection_Key);
  inputValues.FloatValue = filterArgs.value<float32>(k_FloatValue_Key);

  return Lesson5(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct PROGWORKSHOP_EXPORT Lesson5InputValues
{
  DataPath InputDataArrayPath;
  DataPath OutputDataArrayPath;
  float32 Value;
  ChoicesParameter::ValueType Operator;
  bool Selection;
  float32 FloatValue;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class PROGWORKSHOP_EXPORT Lesson5
{
public:
  Lesson5(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, Lesson5InputValues* inputValues);
  ~Lesson5() noexcept;

  Lesson5(const Lesson5&) = delete;
  Lesson5(Lesson5&&) noexcept = delete;
  Lesson5& operator=(const Lesson5&) = delete;
  Lesson5& operator=(Lesson5&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const Lesson5InputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
