#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  EnsembleInfoReaderInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.CellEnsembleAttributeMatrixName = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  inputValues.CrystalStructuresArrayName = filterArgs.value<DataPath>(k_CrystalStructuresArrayName_Key);
  inputValues.PhaseTypesArrayName = filterArgs.value<DataPath>(k_PhaseTypesArrayName_Key);

  return EnsembleInfoReader(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT EnsembleInfoReaderInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  DataPath DataContainerName;
  DataPath CellEnsembleAttributeMatrixName;
  DataPath CrystalStructuresArrayName;
  DataPath PhaseTypesArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT EnsembleInfoReader
{
public:
  EnsembleInfoReader(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EnsembleInfoReaderInputValues* inputValues);
  ~EnsembleInfoReader() noexcept;

  EnsembleInfoReader(const EnsembleInfoReader&) = delete;
  EnsembleInfoReader(EnsembleInfoReader&&) noexcept = delete;
  EnsembleInfoReader& operator=(const EnsembleInfoReader&) = delete;
  EnsembleInfoReader& operator=(EnsembleInfoReader&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const EnsembleInfoReaderInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
