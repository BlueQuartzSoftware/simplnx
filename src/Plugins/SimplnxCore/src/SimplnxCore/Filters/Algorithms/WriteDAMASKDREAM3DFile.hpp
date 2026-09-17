#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/IFilter.hpp"

#include <atomic>
#include <filesystem>

namespace nx::core
{
/**
 * @struct WriteDAMASKDREAM3DFileInputValues
 * @brief Contains the canonical DAMASK DREAM3D export inputs.
 */
struct SIMPLNXCORE_EXPORT WriteDAMASKDREAM3DFileInputValues
{
  std::filesystem::path OutputFile;
  bool UseCompression = true;
  int32 CompressionLevel = 5;
  uint64 Representation = 0;
  bool WritePhaseNames = false;
  float64 ScaleToMeters = 1.0;
  DataPath ImageGeometryPath;
  DataPath CellEulerAnglesArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath FeatureEulerAnglesArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath PhaseNamesArrayPath;
};

/**
 * @class WriteDAMASKDREAM3DFile
 * @brief Writes a minimal DREAM3D file that uses DAMASK's canonical object names.
 */
class SIMPLNXCORE_EXPORT WriteDAMASKDREAM3DFile
{
public:
  /**
   * @brief Constructs the algorithm.
   * @param dataStructure Contains the selected geometry and arrays.
   * @param messageHandler Receives progress messages.
   * @param shouldCancel Indicates that execution must stop.
   * @param inputValues Contains the export parameters.
   */
  WriteDAMASKDREAM3DFile(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, WriteDAMASKDREAM3DFileInputValues* inputValues);

  ~WriteDAMASKDREAM3DFile() noexcept;

  WriteDAMASKDREAM3DFile(const WriteDAMASKDREAM3DFile&) = delete;
  WriteDAMASKDREAM3DFile(WriteDAMASKDREAM3DFile&&) noexcept = delete;
  WriteDAMASKDREAM3DFile& operator=(const WriteDAMASKDREAM3DFile&) = delete;
  WriteDAMASKDREAM3DFile& operator=(WriteDAMASKDREAM3DFile&&) noexcept = delete;

  /**
   * @brief Writes the canonical DREAM3D bridge file.
   * @return Success, cancellation, or an export error.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WriteDAMASKDREAM3DFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
