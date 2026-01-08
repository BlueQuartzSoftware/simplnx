#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  WriteDREAM3DInputValues inputValues;
  inputValues.ExportFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(export_file_path);
  inputValues.WriteXdmfFile = filterArgs.value<BoolParameter::ValueType>(write_xdmf_file);
  return WriteDREAM3D(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT WriteDREAM3DInputValues
{
  FileSystemPathParameter::ValueType ExportFilePath;
  BoolParameter::ValueType WriteXdmfFile;
};

/**
 * @class WriteDREAM3D
 * @brief This algorithm implements support code for the WriteDREAM3DFilter
 */

class SIMPLNXCORE_EXPORT WriteDREAM3D
{
public:
  WriteDREAM3D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteDREAM3DInputValues* inputValues);
  ~WriteDREAM3D() noexcept;

  WriteDREAM3D(const WriteDREAM3D&) = delete;
  WriteDREAM3D(WriteDREAM3D&&) noexcept = delete;
  WriteDREAM3D& operator=(const WriteDREAM3D&) = delete;
  WriteDREAM3D& operator=(WriteDREAM3D&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WriteDREAM3DInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
