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
  DataPath selected_geometry_path;
  bool write_node_file{};
  bool number_nodes{};
  bool include_node_file_header{};
  fs::path node_file_path;
  bool write_element_file{};
  bool number_elements{};
  bool include_element_file_header{};
  fs::path element_file_path;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT WriteNodesAndElementsFiles
{
public:
  WriteNodesAndElementsFiles(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteNodesAndElementsFilesInputValues* inputValues);
  ~WriteNodesAndElementsFiles() noexcept = default;

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

private:
  DataStructure& m_DataStructure;
  const WriteNodesAndElementsFilesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace nx::core
