#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace nx::core
{
/**
 * @struct WriteDAMASKFilesInputValues
 * @brief Contains the values that control the DAMASK file export.
 */
struct SIMPLNXCORE_EXPORT WriteDAMASKFilesInputValues
{
  /** @brief Selects pointwise or grainwise output. */
  ChoicesParameter::ValueType DataFormat = 0;
  /** @brief Directory that receives both output files. */
  FileSystemPathParameter::ValueType OutputPath;
  /** @brief Base name for the geometry file. */
  StringParameter::ValueType GeometryFileName;
  /** @brief Homogenization index in the geometry header. */
  int32 HomogenizationIndex = 1;
  /** @brief Enables compact pointwise geometry IDs. */
  bool CompressGeomFile = false;
  /** @brief Path to the Image Geometry that defines the grid. */
  DataPath ImageGeometryPath;
  /** @brief Path to the scalar cell feature IDs. */
  DataPath FeatureIdsArrayPath;
  /** @brief Path to the three-component cell Euler angles in radians. */
  DataPath CellEulerAnglesArrayPath;
  /** @brief Path to the scalar cell phase IDs. */
  DataPath CellPhasesArrayPath;
};

/**
 * @class WriteDAMASKFiles
 * @brief Writes a DAMASK geometry file and a partial material configuration file.
 */
class SIMPLNXCORE_EXPORT WriteDAMASKFiles
{
public:
  /**
   * @brief Constructs the DAMASK writer.
   * @param dataStructure Contains the input geometry and arrays.
   * @param messageHandler Receives progress and warning messages.
   * @param shouldCancel Indicates that file generation must stop.
   * @param inputValues Supplies export parameters for the call lifetime.
   */
  WriteDAMASKFiles(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, WriteDAMASKFilesInputValues* inputValues);

  ~WriteDAMASKFiles() noexcept;

  WriteDAMASKFiles(const WriteDAMASKFiles&) = delete;
  WriteDAMASKFiles(WriteDAMASKFiles&&) noexcept = delete;
  WriteDAMASKFiles& operator=(const WriteDAMASKFiles&) = delete;
  WriteDAMASKFiles& operator=(WriteDAMASKFiles&&) noexcept = delete;

  /**
   * @brief Generates and commits both DAMASK files.
   * @return Success, cancellation, an invalid-grain error, or a file-writing error.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WriteDAMASKFilesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
