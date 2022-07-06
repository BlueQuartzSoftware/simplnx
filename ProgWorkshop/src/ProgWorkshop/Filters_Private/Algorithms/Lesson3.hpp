#pragma once

#include "ProgWorkshop/ProgWorkshop_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  Lesson3InputValues inputValues;

  inputValues.InputDataArrayPath = filterArgs.value<DataPath>(k_InputDataArrayPath_Key);
  inputValues.OutputDataArrayPath = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  inputValues.Value = filterArgs.value<float32>(k_Value_Key);
  inputValues.Operator = filterArgs.value<ChoicesParameter::ValueType>(k_Operator_Key);

  return Lesson3(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct PROGWORKSHOP_EXPORT Lesson3InputValues
{
  DataPath InputDataArrayPath;
  DataPath OutputDataArrayPath;
  float32 Value;
  ChoicesParameter::ValueType Operator;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class PROGWORKSHOP_EXPORT Lesson3
{
public:
  Lesson3(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, Lesson3InputValues* inputValues);
  ~Lesson3() noexcept;

  Lesson3(const Lesson3&) = delete;
  Lesson3(Lesson3&&) noexcept = delete;
  Lesson3& operator=(const Lesson3&) = delete;
  Lesson3& operator=(Lesson3&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const Lesson3InputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
