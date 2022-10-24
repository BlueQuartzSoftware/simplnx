#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  RemoveComponentFromArrayInputValues inputValues;

  inputValues.CompNumber = filterArgs.value<int32>(k_CompNumber_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.NewArrayArrayName = filterArgs.value<DataPath>(k_NewArrayArrayName_Key);
  inputValues.ReducedArrayArrayName = filterArgs.value<DataPath>(k_ReducedArrayArrayName_Key);
  inputValues.SaveRemovedComponent = filterArgs.value<bool>(k_SaveRemovedComponent_Key);

  return RemoveComponentFromArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT RemoveComponentFromArrayInputValues
{
  int32 CompNumber;
  DataPath SelectedArrayPath;
  DataPath NewArrayArrayName;
  DataPath ReducedArrayArrayName;
  bool SaveRemovedComponent;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT RemoveComponentFromArray
{
public:
  RemoveComponentFromArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RemoveComponentFromArrayInputValues* inputValues);
  ~RemoveComponentFromArray() noexcept;

  RemoveComponentFromArray(const RemoveComponentFromArray&) = delete;
  RemoveComponentFromArray(RemoveComponentFromArray&&) noexcept = delete;
  RemoveComponentFromArray& operator=(const RemoveComponentFromArray&) = delete;
  RemoveComponentFromArray& operator=(RemoveComponentFromArray&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RemoveComponentFromArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
