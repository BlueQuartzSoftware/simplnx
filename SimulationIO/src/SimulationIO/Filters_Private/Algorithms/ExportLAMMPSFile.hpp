#pragma once

#include "SimulationIO/SimulationIO_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ExportLAMMPSFileInputValues inputValues;

  inputValues.LammpsFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_LammpsFile_Key);
  inputValues.AtomFeatureLabelsPath = filterArgs.value<DataPath>(k_AtomFeatureLabelsPath_Key);

  return ExportLAMMPSFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SIMULATIONIO_EXPORT ExportLAMMPSFileInputValues
{
  FileSystemPathParameter::ValueType LammpsFile;
  DataPath AtomFeatureLabelsPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMULATIONIO_EXPORT ExportLAMMPSFile
{
public:
  ExportLAMMPSFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExportLAMMPSFileInputValues* inputValues);
  ~ExportLAMMPSFile() noexcept;

  ExportLAMMPSFile(const ExportLAMMPSFile&) = delete;
  ExportLAMMPSFile(ExportLAMMPSFile&&) noexcept = delete;
  ExportLAMMPSFile& operator=(const ExportLAMMPSFile&) = delete;
  ExportLAMMPSFile& operator=(ExportLAMMPSFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExportLAMMPSFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
