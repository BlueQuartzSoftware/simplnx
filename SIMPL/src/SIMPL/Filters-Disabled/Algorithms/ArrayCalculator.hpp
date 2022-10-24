#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ArrayCalculatorInputValues inputValues;

  inputValues.SelectedAttributeMatrix = filterArgs.value<DataPath>(k_SelectedAttributeMatrix_Key);
  inputValues.InfixEquation = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InfixEquation_Key);
  inputValues.ScalarType = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ScalarType_Key);
  inputValues.CalculatedArray = filterArgs.value<DataPath>(k_CalculatedArray_Key);

  return ArrayCalculator(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT ArrayCalculatorInputValues
{
  DataPath SelectedAttributeMatrix;
  <<<NOT_IMPLEMENTED>>> InfixEquation;
  /*[x]*/<<<NOT_IMPLEMENTED>>> ScalarType;
  /*[x]*/ DataPath CalculatedArray;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT ArrayCalculator
{
public:
  ArrayCalculator(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ArrayCalculatorInputValues* inputValues);
  ~ArrayCalculator() noexcept;

  ArrayCalculator(const ArrayCalculator&) = delete;
  ArrayCalculator(ArrayCalculator&&) noexcept = delete;
  ArrayCalculator& operator=(const ArrayCalculator&) = delete;
  ArrayCalculator& operator=(ArrayCalculator&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ArrayCalculatorInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
