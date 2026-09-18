#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

namespace nx::core
{

/**
 * @struct WriteStatsGenOdfAngleFileInputValues
 * @brief Defines ODF angle output formatting and cell-array selections.
 */
struct ORIENTATIONANALYSIS_EXPORT WriteStatsGenOdfAngleFileInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  float32 Weight;
  int32 Sigma;
  ChoicesParameter::ValueType Delimiter;
  bool ConvertToDegrees;
  bool UseMask;
  DataPath CellEulerAnglesArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath MaskArrayPath;
};

/**
 * @class WriteStatsGenOdfAngleFile
 * @brief Writes one StatsGenerator ODF angle file for each nonzero cell phase.
 *
 * Concrete in-memory stores use direct pointers. Other stores use 65,536-tuple
 * pages. Both paths count all phases once, then scan all cells again for each
 * output phase. Cancellation returns success and can leave the current phase
 * file with a header and partial records.
 *
 * Source bulk-I/O errors and file-open failures are returned. The current
 * implementation does not report output stream failures after a file opens.
 */
class ORIENTATIONANALYSIS_EXPORT WriteStatsGenOdfAngleFile
{
public:
  /**
   * @brief Initializes an ODF angle-file writer.
   * @param dataStructure Provides phase, Euler, and optional mask arrays.
   * @param mesgHandler Receives per-phase status messages.
   * @param shouldCancel Signals cancellation between pages and phase files.
   * @param inputValues Defines input paths and output formatting.
   * @pre All arguments outlive this writer.
   */
  WriteStatsGenOdfAngleFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteStatsGenOdfAngleFileInputValues* inputValues);
  ~WriteStatsGenOdfAngleFile() noexcept;

  WriteStatsGenOdfAngleFile(const WriteStatsGenOdfAngleFile&) = delete;
  WriteStatsGenOdfAngleFile(WriteStatsGenOdfAngleFile&&) noexcept = delete;
  WriteStatsGenOdfAngleFile& operator=(const WriteStatsGenOdfAngleFile&) = delete;
  WriteStatsGenOdfAngleFile& operator=(WriteStatsGenOdfAngleFile&&) noexcept = delete;

  /**
   * @brief Writes files for phases present in the selected cells.
   * @return Directory, mask-type, source-read, or file-open errors and empty-phase warnings.
   * @pre Phase, Euler, and optional mask arrays have equal tuple counts.
   * @pre Euler tuples have three components and selected tuple counts fit int32.
   * @pre Delimiter is an index in the range [0, 4].
   *
   * Phase zero is omitted. Other phase values become filename suffixes.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

  /**
   * @brief Counts selected tuples for one phase through bounded reads.
   * @param cellPhases Supplies cell phase IDs.
   * @param mask Optional Bool or UInt8 mask comparator.
   * @param totalPoints Number of tuples to inspect.
   * @param phase Phase ID to count.
   * @return Selected tuple count.
   * @throws std::invalid_argument If mask does not match the configured mask type.
   * @throws std::runtime_error If a source bulk read fails.
   */
  int determineOutputLineCount(const Int32Array& cellPhases, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask, usize totalPoints, int32 phase) const;

  /**
   * @brief Writes one phase header and body through bounded source reads.
   * @param out Open destination stream.
   * @param cellPhases Supplies cell phase IDs.
   * @param mask Optional Bool or UInt8 mask comparator.
   * @param lineCount Header record count.
   * @param totalPoints Number of tuples to inspect.
   * @param phase Phase ID to write.
   * @return Mask-type or source bulk-read errors.
   *
   * This compatibility method does not inspect cancellation or output stream state.
   */
  Result<> writeOutputFile(std::ofstream& out, const Int32Array& cellPhases, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask, int32 lineCount, usize totalPoints, int32 phase) const;

private:
  DataStructure& m_DataStructure;
  const WriteStatsGenOdfAngleFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
