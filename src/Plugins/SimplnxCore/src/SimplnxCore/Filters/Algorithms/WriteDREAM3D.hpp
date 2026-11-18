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

  Result<> operator()();

  /**
   * @brief Installs the XDMF sidecar when requested and returns the output status.
   *
   * A failed disk flush permits sidecar installation because the DREAM3D file was already replaced.
   * @param commitResult Result of installing the DREAM3D file.
   * @param atomicFile Transaction that holds the temporary path for the sidecar.
   * @param destination Final DREAM3D path.
   * @param writeXdmf True when the XDMF sidecar was written.
   * @return Commit and sidecar errors, if any.
   */
  [[nodiscard]] static Result<> finalizeOutput(Result<> commitResult, const AtomicFile& atomicFile, const std::filesystem::path& destination, bool writeXdmf);

private:
  DataStructure& m_DataStructure;
  const WriteDREAM3DInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
