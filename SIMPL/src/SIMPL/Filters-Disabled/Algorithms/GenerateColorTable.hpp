#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  GenerateColorTableInputValues inputValues;

  inputValues.SelectedPresetName = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedPresetName_Key);
  inputValues.SelectedDataArrayPath = filterArgs.value<DataPath>(k_SelectedDataArrayPath_Key);
  inputValues.RgbArrayName = filterArgs.value<DataPath>(k_RgbArrayName_Key);

  return GenerateColorTable(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT GenerateColorTableInputValues
{
  <<<NOT_IMPLEMENTED>>> SelectedPresetName;
  /*[x]*/ DataPath SelectedDataArrayPath;
  DataPath RgbArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT GenerateColorTable
{
public:
  GenerateColorTable(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GenerateColorTableInputValues* inputValues);
  ~GenerateColorTable() noexcept;

  GenerateColorTable(const GenerateColorTable&) = delete;
  GenerateColorTable(GenerateColorTable&&) noexcept = delete;
  GenerateColorTable& operator=(const GenerateColorTable&) = delete;
  GenerateColorTable& operator=(GenerateColorTable&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateColorTableInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
