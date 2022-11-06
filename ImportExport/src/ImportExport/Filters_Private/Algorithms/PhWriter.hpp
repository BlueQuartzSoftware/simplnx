#pragma once

#include "ImportExport/ImportExport_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  PhWriterInputValues inputValues;

  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);

  return PhWriter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMPORTEXPORT_EXPORT PhWriterInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  DataPath FeatureIdsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMPORTEXPORT_EXPORT PhWriter
{
public:
  PhWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, PhWriterInputValues* inputValues);
  ~PhWriter() noexcept;

  PhWriter(const PhWriter&) = delete;
  PhWriter(PhWriter&&) noexcept = delete;
  PhWriter& operator=(const PhWriter&) = delete;
  PhWriter& operator=(PhWriter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const PhWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
