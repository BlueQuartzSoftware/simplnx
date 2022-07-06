#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateFeatureArrayFromElementArrayInputValues inputValues;

  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellFeatureAttributeMatrixName = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixName_Key);
  inputValues.CreatedArrayName = filterArgs.value<DataPath>(k_CreatedArrayName_Key);

  return CreateFeatureArrayFromElementArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT CreateFeatureArrayFromElementArrayInputValues
{
  DataPath SelectedCellArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellFeatureAttributeMatrixName;
  DataPath CreatedArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT CreateFeatureArrayFromElementArray
{
public:
  CreateFeatureArrayFromElementArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                     CreateFeatureArrayFromElementArrayInputValues* inputValues);
  ~CreateFeatureArrayFromElementArray() noexcept;

  CreateFeatureArrayFromElementArray(const CreateFeatureArrayFromElementArray&) = delete;
  CreateFeatureArrayFromElementArray(CreateFeatureArrayFromElementArray&&) noexcept = delete;
  CreateFeatureArrayFromElementArray& operator=(const CreateFeatureArrayFromElementArray&) = delete;
  CreateFeatureArrayFromElementArray& operator=(CreateFeatureArrayFromElementArray&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateFeatureArrayFromElementArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
