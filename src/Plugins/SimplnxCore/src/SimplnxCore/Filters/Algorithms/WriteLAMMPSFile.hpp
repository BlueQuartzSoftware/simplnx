#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT WriteLAMMPSFileInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  DataPath VertexGeomPath;
  DataPath AtomLabelsPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT WriteLAMMPSFile
{
public:
  WriteLAMMPSFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteLAMMPSFileInputValues* inputValues);
  ~WriteLAMMPSFile() noexcept;

  WriteLAMMPSFile(const WriteLAMMPSFile&) = delete;
  WriteLAMMPSFile(WriteLAMMPSFile&&) noexcept = delete;
  WriteLAMMPSFile& operator=(const WriteLAMMPSFile&) = delete;
  WriteLAMMPSFile& operator=(WriteLAMMPSFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WriteLAMMPSFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
