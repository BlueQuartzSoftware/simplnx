#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>

namespace nx::core
{
class AtomicFile;
class Pipeline;
class PipelineFilter;

/**
 * @struct WriteDREAM3DInputValues
 * @brief Parameter bag consumed by the WriteDREAM3D algorithm. Populated from the
 *        filter's Arguments in WriteDREAM3DFilter::executeImpl.
 */
struct SIMPLNXCORE_EXPORT WriteDREAM3DInputValues
{
  FileSystemPathParameter::ValueType ExportFilePath;
  BoolParameter::ValueType WriteXdmfFile;
  /// Master on/off switch for HDF5 gzip compression of DataArray / NeighborList datasets.
  bool UseCompression = true;
  /// Gzip/deflate level in [1, 9]; ignored when UseCompression is false.
  int32 CompressionLevel = 5;
  const PipelineFilter* PipelineNode = nullptr;
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

  /**
   * @brief Writes the structure to a temporary file and moves it, and the optional .xdmf file, onto the final paths.
   * @return Success with the warnings of the file-write hook and the writer, or the first error with the warnings collected before it.
   *
   * The file-write hook runs on the final DREAM3D path and, when an .xdmf file is written, on the final .xdmf path, after the
   * temporary folder exists and before anything is serialized. The writer's own hook sees only the temporary path, so this is the
   * call that releases a store reading a final path. Cancellation is checked before and after the hook; stores released before a
   * cancel stay released.
   */
  Result<> operator()();

private:
  /**
   * @brief Serializes the structure to the temporary file, then moves the file and its optional .xdmf file onto the final paths.
   * @param atomicFile Temporary file whose commit() replaces the final DREAM3D file.
   * @param pipeline Pipeline metadata written into the file.
   * @param finalXdmfFilePath Final .xdmf path, the one the file-write hook ran on.
   * @return Success with the writer's warnings, or the write, cancel, commit or .xdmf rename error.
   * @pre The file-write hook ran on every final path.
   */
  Result<> writeAndCommit(AtomicFile& atomicFile, const Pipeline& pipeline, const std::filesystem::path& finalXdmfFilePath) const;

  DataStructure& m_DataStructure;
  const WriteDREAM3DInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
