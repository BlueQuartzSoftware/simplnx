#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{

/**
 * @struct WriteINLFileInputValues
 * @brief Collects the output path and input data paths required for an INL export.
 */
struct ORIENTATIONANALYSIS_EXPORT WriteINLFileInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  DataPath ImageGeomPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CellEulerAnglesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath MaterialNameArrayPath;
  DataPath NumFeaturesArrayPath;
};

/**
 * @class WriteINLFile
 * @brief Writes image-cell orientation data to an INL text file.
 *
 * Concrete in-memory cell stores use direct contiguous access. Other stores use
 * sequential pages of at most 65,536 tuples. Ensemble arrays are cached in full.
 * A set of distinct feature IDs also grows with the number of unique IDs and can
 * contain one entry per cell.
 *
 * Cancellation returns success. It can leave an empty file during the feature-ID
 * scan or a partial file after the header is written.
 */
class ORIENTATIONANALYSIS_EXPORT WriteINLFile
{
public:
  /**
   * @brief Initializes an INL writer.
   * @param dataStructure Provides the image and input arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation between pages.
   * @param inputValues Identifies input objects and the output path.
   * @pre All arguments outlive this writer.
   */
  WriteINLFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteINLFileInputValues* inputValues);

  ~WriteINLFile() noexcept;

  WriteINLFile(const WriteINLFile&) = delete;
  WriteINLFile(WriteINLFile&&) noexcept = delete;
  WriteINLFile& operator=(const WriteINLFile&) = delete;
  WriteINLFile& operator=(WriteINLFile&&) noexcept = delete;

  /**
   * @brief Writes the INL header and one record per image cell.
   * @return Directory, input bulk-read, or output-write errors.
   * @pre Cell arrays match the image cell count and use component counts 1, 3, and 1.
   * @pre Cell phase IDs are nonnegative and index all ensemble arrays.
   * @pre Material names, crystal structures, and feature counts have equal tuple counts.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WriteINLFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
