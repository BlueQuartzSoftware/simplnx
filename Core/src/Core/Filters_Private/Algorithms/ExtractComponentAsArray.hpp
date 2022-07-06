#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ExtractComponentAsArrayInputValues inputValues;

  inputValues.CompNumber = filterArgs.value<int32>(k_CompNumber_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.NewArrayArrayName = filterArgs.value<DataPath>(k_NewArrayArrayName_Key);

  return ExtractComponentAsArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT ExtractComponentAsArrayInputValues
{
  int32 CompNumber;
  DataPath SelectedArrayPath;
  DataPath NewArrayArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT ExtractComponentAsArray
{
public:
  ExtractComponentAsArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExtractComponentAsArrayInputValues* inputValues);
  ~ExtractComponentAsArray() noexcept;

  ExtractComponentAsArray(const ExtractComponentAsArray&) = delete;
  ExtractComponentAsArray(ExtractComponentAsArray&&) noexcept = delete;
  ExtractComponentAsArray& operator=(const ExtractComponentAsArray&) = delete;
  ExtractComponentAsArray& operator=(ExtractComponentAsArray&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExtractComponentAsArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
