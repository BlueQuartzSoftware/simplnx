#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ReadASCIIDataInputValues inputValues;

  inputValues.WizardData = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_WizardData_Key);

  return ReadASCIIData(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT ReadASCIIDataInputValues
{
  <<<NOT_IMPLEMENTED>>> WizardData;
  /*[x]*/
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT ReadASCIIData
{
public:
  ReadASCIIData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadASCIIDataInputValues* inputValues);
  ~ReadASCIIData() noexcept;

  ReadASCIIData(const ReadASCIIData&) = delete;
  ReadASCIIData(ReadASCIIData&&) noexcept = delete;
  ReadASCIIData& operator=(const ReadASCIIData&) = delete;
  ReadASCIIData& operator=(ReadASCIIData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ReadASCIIDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
