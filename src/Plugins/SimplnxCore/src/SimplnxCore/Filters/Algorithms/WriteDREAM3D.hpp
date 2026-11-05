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
   * @return Success with the warnings of the file-replace hooks and the writer. Otherwise the first error, with the warnings
   * collected before it. Error -6065 when the staged .xdmf file cannot move into the temporary path of its AtomicFile.
   *
   * Each final path has its own AtomicFile. AtomicFile::Create() runs the file-replace hook on the final DREAM3D path. When an
   * .xdmf file is written, it then runs the hook on the final .xdmf path. Both hooks run before anything is serialized. The
   * .dream3d file is committed first and the .xdmf file after it. If the .xdmf commit fails, the new .dream3d file stays and the previous .xdmf file stays. Cancellation is checked
   * after the hooks and after the write; components released by a hook before a cancel stay released.
   */
  Result<> operator()();

private:
  /**
   * @brief Serializes the structure to the temporary file, then commits the file and its optional .xdmf file onto the final paths.
   * @param atomicFile Temporary file whose commit() replaces the final DREAM3D file.
   * @param xdmfAtomicFile Temporary file whose commit() replaces the final .xdmf file, or null when no .xdmf file is written.
   * @return Success with the writer's warnings, or the pipeline lookup, write, cancel, staged .xdmf move or commit error.
   * @pre The file-replace hook ran on every final path.
   */
  Result<> writeAndCommit(AtomicFile& atomicFile, AtomicFile* xdmfAtomicFile) const;

  DataStructure& m_DataStructure;
  const WriteDREAM3DInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
