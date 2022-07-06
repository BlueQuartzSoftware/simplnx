#pragma once

#include "ImportExport/ImportExport_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  SPParksDumpReaderInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.OneBasedArrays = filterArgs.value<bool>(k_OneBasedArrays_Key);
  inputValues.VolumeDataContainerName = filterArgs.value<DataPath>(k_VolumeDataContainerName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  inputValues.FeatureIdsArrayName = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);

  return SPParksDumpReader(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMPORTEXPORT_EXPORT SPParksDumpReaderInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  VectorFloat32Parameter::ValueType Origin;
  VectorFloat32Parameter::ValueType Spacing;
  bool OneBasedArrays;
  DataPath VolumeDataContainerName;
  DataPath CellAttributeMatrixName;
  DataPath FeatureIdsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMPORTEXPORT_EXPORT SPParksDumpReader
{
public:
  SPParksDumpReader(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SPParksDumpReaderInputValues* inputValues);
  ~SPParksDumpReader() noexcept;

  SPParksDumpReader(const SPParksDumpReader&) = delete;
  SPParksDumpReader(SPParksDumpReader&&) noexcept = delete;
  SPParksDumpReader& operator=(const SPParksDumpReader&) = delete;
  SPParksDumpReader& operator=(SPParksDumpReader&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SPParksDumpReaderInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
