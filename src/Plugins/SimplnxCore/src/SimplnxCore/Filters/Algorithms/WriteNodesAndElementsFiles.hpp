#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
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
    UnsupportedGeometryType = -137,
    SameNodeAndElementFile = -139
  };

  /**
   * @brief Writes the selected node file and element file.
   * @return Success with the warnings of the file-replace hooks, the first hook error, or the first write error with the hook warnings
   * in front.
   *
   * The files are written in place. BeforeFileReplace() runs once on each selected output path before either file is opened, so a
   * hook error changes no file. Equal node and element paths are rejected with -139 before any hook runs.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  /**
   * @brief Writes the selected files after the file-replace hooks ran on their paths.
   * @param vertices Vertex coordinates of the geometry.
   * @param cellsArray Cell connectivity of the geometry; null for a vertex geometry.
   * @param writesElementFile True when the element file is written.
   * @return Success, or the first open error.
   */
  Result<> writeFiles(const Float32Array& vertices, const UInt64Array* cellsArray, bool writesElementFile) const;

  DataStructure& m_DataStructure;
  const WriteNodesAndElementsFilesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace nx::core
