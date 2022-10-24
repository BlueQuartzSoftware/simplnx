#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  RemoveArraysInputValues inputValues;

  inputValues.DataArraysToRemove = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_DataArraysToRemove_Key);

  return RemoveArrays(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT RemoveArraysInputValues
{
  <<<NOT_IMPLEMENTED>>> DataArraysToRemove;
  /*[x]*/
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT RemoveArrays
{
public:
  RemoveArrays(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RemoveArraysInputValues* inputValues);
  ~RemoveArrays() noexcept;

  RemoveArrays(const RemoveArrays&) = delete;
  RemoveArrays(RemoveArrays&&) noexcept = delete;
  RemoveArrays& operator=(const RemoveArrays&) = delete;
  RemoveArrays& operator=(RemoveArrays&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RemoveArraysInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
