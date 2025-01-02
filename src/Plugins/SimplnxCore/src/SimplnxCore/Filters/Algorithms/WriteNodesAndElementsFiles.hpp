#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace fs = std::filesystem;

namespace nx::core
{

struct SIMPLNXCORE_EXPORT WriteNodesAndElementsFilesInputValues
{
  DataPath SelectedGeometryPath;
  bool WriteNodeFile;
  bool NumberNodes;
  bool IncludeNodeFileHeader;
  fs::path NodeFilePath;
  bool WriteElementFile;
  bool NumberElements;
  bool IncludeElementFileHeader;
  fs::path ElementFilePath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT WriteNodesAndElementsFiles
{
public:
  WriteNodesAndElementsFiles(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteNodesAndElementsFilesInputValues* inputValues);
  ~WriteNodesAndElementsFiles() noexcept;

  WriteNodesAndElementsFiles(const WriteNodesAndElementsFiles&) = delete;
  WriteNodesAndElementsFiles(WriteNodesAndElementsFiles&&) noexcept = delete;
  WriteNodesAndElementsFiles& operator=(const WriteNodesAndElementsFiles&) = delete;
  WriteNodesAndElementsFiles& operator=(WriteNodesAndElementsFiles&&) noexcept = delete;

  enum class ErrorCodes : int64
  {
    NoFileWriterChosen = -134,
    FailedToOpenOutputFile = -135,
    VertexGeomHasNoElements = -136,
    UnsupportedGeometryType = -137
  };

  Result<> operator()();

  const std::atomic_bool& getCancel();

  void sendMessage(const std::string& message);

private:
  DataStructure& m_DataStructure;
  const WriteNodesAndElementsFilesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
