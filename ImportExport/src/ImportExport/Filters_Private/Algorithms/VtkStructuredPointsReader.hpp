#pragma once

#include "ImportExport/ImportExport_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  VtkStructuredPointsReaderInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.ReadPointData = filterArgs.value<bool>(k_ReadPointData_Key);
  inputValues.ReadCellData = filterArgs.value<bool>(k_ReadCellData_Key);
  inputValues.VertexDataContainerName = filterArgs.value<DataPath>(k_VertexDataContainerName_Key);
  inputValues.VolumeDataContainerName = filterArgs.value<DataPath>(k_VolumeDataContainerName_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);

  return VtkStructuredPointsReader(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMPORTEXPORT_EXPORT VtkStructuredPointsReaderInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  bool ReadPointData;
  bool ReadCellData;
  DataPath VertexDataContainerName;
  DataPath VolumeDataContainerName;
  DataPath VertexAttributeMatrixName;
  DataPath CellAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMPORTEXPORT_EXPORT VtkStructuredPointsReader
{
public:
  VtkStructuredPointsReader(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, VtkStructuredPointsReaderInputValues* inputValues);
  ~VtkStructuredPointsReader() noexcept;

  VtkStructuredPointsReader(const VtkStructuredPointsReader&) = delete;
  VtkStructuredPointsReader(VtkStructuredPointsReader&&) noexcept = delete;
  VtkStructuredPointsReader& operator=(const VtkStructuredPointsReader&) = delete;
  VtkStructuredPointsReader& operator=(VtkStructuredPointsReader&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const VtkStructuredPointsReaderInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
