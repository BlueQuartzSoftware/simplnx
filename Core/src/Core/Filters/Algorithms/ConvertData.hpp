#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ConvertDataInputValues inputValues;

  inputValues.ScalarType = filterArgs.value<NumericType>(k_ScalarType_Key);
  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.OutputArrayName = filterArgs.value<DataPath>(k_OutputArrayName_Key);

  return ConvertData(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT ConvertDataInputValues
{
  DataType ScalarType;
  DataPath SelectedArrayPath;
  DataPath OutputArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT ConvertData
{
public:
  ConvertData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertDataInputValues* inputValues);
  ~ConvertData() noexcept;

  ConvertData(const ConvertData&) = delete;
  ConvertData(ConvertData&&) noexcept = delete;
  ConvertData& operator=(const ConvertData&) = delete;
  ConvertData& operator=(ConvertData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ConvertDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
