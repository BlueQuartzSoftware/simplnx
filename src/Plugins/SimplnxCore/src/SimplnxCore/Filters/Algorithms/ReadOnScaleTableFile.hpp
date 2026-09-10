#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <array>
#include <filesystem>

namespace nx::core
{
/**
 * @struct OnScaleTableFileHeader
 * @brief Contains the cached dimensions and section counts from an OnScale table file.
 */
struct SIMPLNXCORE_EXPORT OnScaleTableFileHeader
{
  FileSystemPathParameter::ValueType InputFile;
  std::filesystem::file_time_type TimeStamp;
  std::array<usize, 3> BoundsCounts = {2, 2, 2};
  std::array<bool, 3> BoundsPresent = {false, false, false};
  usize NameCount = 0;
  usize MaterialCount = 0;
};

/**
 * @struct ReadOnScaleTableFileInputValues
 * @brief Contains the paths, fallback coordinates, and cached header for an OnScale table import.
 */
struct SIMPLNXCORE_EXPORT ReadOnScaleTableFileInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  VectorFloat32Parameter::ValueType FallbackOrigin;
  VectorFloat32Parameter::ValueType FallbackSpacing;
  DataPath RectGridGeometryPath;
  DataPath CellAttributeMatrixPath;
  DataPath FeatureIdsArrayPath;
  DataPath PhaseAttributeMatrixPath;
  DataPath MaterialNamesArrayPath;
  OnScaleTableFileHeader Header;
};

/**
 * @class ReadOnScaleTableFile
 * @brief Reads grid bounds, material names, and material indices from an OnScale table file.
 */
class SIMPLNXCORE_EXPORT ReadOnScaleTableFile
{
public:
  /**
   * @brief Constructs the OnScale table reader.
   * @param dataStructure Contains the created output objects during execution.
   * @param messageHandler Receives stage, progress, and warning messages.
   * @param shouldCancel Indicates that the read operation must stop.
   * @param inputValues Supplies paths, fallback values, and cached header data for the call lifetime.
   */
  ReadOnScaleTableFile(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, ReadOnScaleTableFileInputValues* inputValues);

  ~ReadOnScaleTableFile() noexcept;

  ReadOnScaleTableFile(const ReadOnScaleTableFile&) = delete;
  ReadOnScaleTableFile(ReadOnScaleTableFile&&) noexcept = delete;
  ReadOnScaleTableFile& operator=(const ReadOnScaleTableFile&) = delete;
  ReadOnScaleTableFile& operator=(ReadOnScaleTableFile&&) noexcept = delete;

  /**
   * @brief Reads and validates section counts and bound values through the `matr` header.
   * @return Cached header data or a file-format error.
   */
  Result<OnScaleTableFileHeader> readHeader() const;

  /**
   * @brief Reads the file directly into the output arrays.
   * @return Success, cancellation, or a file-format error.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadOnScaleTableFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
