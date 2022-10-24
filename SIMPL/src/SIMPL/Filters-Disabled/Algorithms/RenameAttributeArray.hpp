#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  RenameAttributeArrayInputValues inputValues;

  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.NewArrayName = filterArgs.value<StringParameter::ValueType>(k_NewArrayName_Key);

  return RenameAttributeArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT RenameAttributeArrayInputValues
{
  DataPath SelectedArrayPath;
  StringParameter::ValueType NewArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT RenameAttributeArray
{
public:
  RenameAttributeArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RenameAttributeArrayInputValues* inputValues);
  ~RenameAttributeArray() noexcept;

  RenameAttributeArray(const RenameAttributeArray&) = delete;
  RenameAttributeArray(RenameAttributeArray&&) noexcept = delete;
  RenameAttributeArray& operator=(const RenameAttributeArray&) = delete;
  RenameAttributeArray& operator=(RenameAttributeArray&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RenameAttributeArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
