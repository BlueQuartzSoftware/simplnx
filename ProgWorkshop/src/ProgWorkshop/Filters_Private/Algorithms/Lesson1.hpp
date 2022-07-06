#pragma once

#include "ProgWorkshop/ProgWorkshop_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  Lesson1InputValues inputValues;

  inputValues.InputDataArrayPath = filterArgs.value<DataPath>(k_InputDataArrayPath_Key);
  inputValues.OutputDataArrayPath = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  inputValues.Value = filterArgs.value<float32>(k_Value_Key);

  return Lesson1(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct PROGWORKSHOP_EXPORT Lesson1InputValues
{
  DataPath InputDataArrayPath;
  DataPath OutputDataArrayPath;
  float32 Value;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class PROGWORKSHOP_EXPORT Lesson1
{
public:
  Lesson1(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, Lesson1InputValues* inputValues);
  ~Lesson1() noexcept;

  Lesson1(const Lesson1&) = delete;
  Lesson1(Lesson1&&) noexcept = delete;
  Lesson1& operator=(const Lesson1&) = delete;
  Lesson1& operator=(Lesson1&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const Lesson1InputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
