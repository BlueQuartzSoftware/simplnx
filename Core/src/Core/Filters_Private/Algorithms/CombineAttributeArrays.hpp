#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CombineAttributeArraysInputValues inputValues;

  inputValues.NormalizeData = filterArgs.value<bool>(k_NormalizeData_Key);
  inputValues.MoveValues = filterArgs.value<bool>(k_MoveValues_Key);
  inputValues.SelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputValues.StackedDataArrayName = filterArgs.value<StringParameter::ValueType>(k_StackedDataArrayName_Key);

  return CombineAttributeArrays(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT CombineAttributeArraysInputValues
{
  bool NormalizeData;
  bool MoveValues;
  MultiArraySelectionParameter::ValueType SelectedDataArrayPaths;
  StringParameter::ValueType StackedDataArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT CombineAttributeArrays
{
public:
  CombineAttributeArrays(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CombineAttributeArraysInputValues* inputValues);
  ~CombineAttributeArrays() noexcept;

  CombineAttributeArrays(const CombineAttributeArrays&) = delete;
  CombineAttributeArrays(CombineAttributeArrays&&) noexcept = delete;
  CombineAttributeArrays& operator=(const CombineAttributeArrays&) = delete;
  CombineAttributeArrays& operator=(CombineAttributeArrays&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CombineAttributeArraysInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
