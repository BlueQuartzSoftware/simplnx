#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  LinkFeatureMapToElementArrayInputValues inputValues;

  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.CellFeatureAttributeMatrixName = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixName_Key);
  inputValues.ActiveArrayName = filterArgs.value<DataPath>(k_ActiveArrayName_Key);

  return LinkFeatureMapToElementArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT LinkFeatureMapToElementArrayInputValues
{
  DataPath SelectedCellArrayPath;
  DataPath CellFeatureAttributeMatrixName;
  DataPath ActiveArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT LinkFeatureMapToElementArray
{
public:
  LinkFeatureMapToElementArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, LinkFeatureMapToElementArrayInputValues* inputValues);
  ~LinkFeatureMapToElementArray() noexcept;

  LinkFeatureMapToElementArray(const LinkFeatureMapToElementArray&) = delete;
  LinkFeatureMapToElementArray(LinkFeatureMapToElementArray&&) noexcept = delete;
  LinkFeatureMapToElementArray& operator=(const LinkFeatureMapToElementArray&) = delete;
  LinkFeatureMapToElementArray& operator=(LinkFeatureMapToElementArray&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const LinkFeatureMapToElementArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
