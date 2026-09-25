#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{
/**
 * @struct WriteOnScaleTableFileInputValues
 * @brief Contains the values that control the OnScale table export.
 */
struct SIMPLNXCORE_EXPORT WriteOnScaleTableFileInputValues
{
  /** @brief Directory that receives the table file. */
  FileSystemPathParameter::ValueType OutputPath;
  /** @brief Prefix for the table file name. */
  StringParameter::ValueType FilePrefix;
  /** @brief Number of keypoints along the X, Y, and Z axes. */
  VectorInt32Parameter::ValueType NumKeypoints;
  /** @brief Path to the Image or Rectilinear Grid Geometry. */
  DataPath InputGeometryPath;
  /** @brief Path to the scalar cell feature IDs. */
  DataPath FeatureIdsArrayPath;
  /** @brief Path to the phase names. */
  DataPath PhaseNamesArrayPath;
};

/**
 * @class WriteOnScaleTableFile
 * @brief Writes one OnScale table file for a grid geometry.
 */
class SIMPLNXCORE_EXPORT WriteOnScaleTableFile
{
public:
  /**
   * @brief Constructs the algorithm.
   * @param dataStructure Contains the input geometry and arrays.
   * @param messageHandler Receives progress messages.
   * @param shouldCancel Indicates that file generation must stop.
   * @param inputValues Supplies the export parameters for the call lifetime.
   */
  WriteOnScaleTableFile(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, WriteOnScaleTableFileInputValues* inputValues);

  ~WriteOnScaleTableFile() noexcept;

  WriteOnScaleTableFile(const WriteOnScaleTableFile&) = delete;
  WriteOnScaleTableFile(WriteOnScaleTableFile&&) noexcept = delete;
  WriteOnScaleTableFile& operator=(const WriteOnScaleTableFile&) = delete;
  WriteOnScaleTableFile& operator=(WriteOnScaleTableFile&&) noexcept = delete;

  /**
   * @brief Generates and commits the OnScale table file.
   * @return Success, cancellation, or an export error.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WriteOnScaleTableFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
