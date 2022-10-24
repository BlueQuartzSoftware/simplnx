#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CombineAttributeMatricesInputValues inputValues;

  inputValues.FirstAttributeMatrixPath = filterArgs.value<DataPath>(k_FirstAttributeMatrixPath_Key);
  inputValues.SecondAttributeMatrixPath = filterArgs.value<DataPath>(k_SecondAttributeMatrixPath_Key);
  inputValues.FirstIndexArrayPath = filterArgs.value<DataPath>(k_FirstIndexArrayPath_Key);
  inputValues.SecondIndexArrayPath = filterArgs.value<DataPath>(k_SecondIndexArrayPath_Key);
  inputValues.NewIndexArrayName = filterArgs.value<DataPath>(k_NewIndexArrayName_Key);
  inputValues.CombinedAttributeMatrixName = filterArgs.value<DataPath>(k_CombinedAttributeMatrixName_Key);

  return CombineAttributeMatrices(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT CombineAttributeMatricesInputValues
{
  DataPath FirstAttributeMatrixPath;
  DataPath SecondAttributeMatrixPath;
  DataPath FirstIndexArrayPath;
  DataPath SecondIndexArrayPath;
  DataPath NewIndexArrayName;
  DataPath CombinedAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT CombineAttributeMatrices
{
public:
  CombineAttributeMatrices(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CombineAttributeMatricesInputValues* inputValues);
  ~CombineAttributeMatrices() noexcept;

  CombineAttributeMatrices(const CombineAttributeMatrices&) = delete;
  CombineAttributeMatrices(CombineAttributeMatrices&&) noexcept = delete;
  CombineAttributeMatrices& operator=(const CombineAttributeMatrices&) = delete;
  CombineAttributeMatrices& operator=(CombineAttributeMatrices&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CombineAttributeMatricesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
