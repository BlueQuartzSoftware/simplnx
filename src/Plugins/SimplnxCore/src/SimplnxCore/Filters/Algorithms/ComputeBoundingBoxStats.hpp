#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

namespace nx::core
{
/**
 * @struct ComputeBoundingBoxStatsInputValues
 * @brief Collects the selected statistics and DataStructure paths.
 *
 * Filter preflight creates the requested output objects. The executor assumes
 * that each selected path has the type and tuple count required by the filter.
 */
struct SIMPLNXCORE_EXPORT ComputeBoundingBoxStatsInputValues
{
  // Options
  bool CalculateLength;
  bool CalculateMin;
  bool CalculateMax;
  bool CalculateSummation;
  bool CalculateMean;
  bool CalculateMedian;
  bool CalculateMode;
  bool CalculateNumUniqueValues;
  bool CalculateStdDev;

  // Input
  DataPath GeometryPath;
  DataPath UnifiedPath;
  DataPath InputPath;

  // Output
  DataPath BoundsHasDataPath;
  DataPath LengthPath;
  DataPath MinPath;
  DataPath MaxPath;
  DataPath SummationPath;
  DataPath MeanPath;
  DataPath MedianPath;
  DataPath ModePath;
  DataPath NumUniqueValuesPath;
  DataPath StdDevPath;
};

/**
 * @class ComputeBoundingBoxStats
 * @brief Dispatches bounding-box statistics to a storage-specific implementation.
 *
 * The dispatcher normally selects the scanline implementation when the input
 * array or unified bounds use out-of-core storage. This separation keeps the
 * fast direct traversal for in-memory data and avoids per-voxel store I/O for
 * out-of-core data. Algorithm override settings can force either path.
 */
class SIMPLNXCORE_EXPORT ComputeBoundingBoxStats
{
public:
  /**
   * @brief Initializes the bounding-box statistics dispatcher.
   * @param dataStructure Contains the geometry, arrays, and outputs.
   * @param mesgHandler Supplies the common algorithm message interface.
   * @param shouldCancel Signals cancellation to the scanline path.
   * @param inputValues Selects statistics and identifies required paths.
   * @pre All arguments outlive this executor.
   */
  ComputeBoundingBoxStats(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeBoundingBoxStatsInputValues* inputValues);
  ~ComputeBoundingBoxStats() noexcept;

  ComputeBoundingBoxStats(const ComputeBoundingBoxStats&) = delete;
  ComputeBoundingBoxStats(ComputeBoundingBoxStats&&) noexcept = delete;
  ComputeBoundingBoxStats& operator=(const ComputeBoundingBoxStats&) = delete;
  ComputeBoundingBoxStats& operator=(ComputeBoundingBoxStats&&) noexcept = delete;

  /**
   * @enum OutputDataType
   * @brief Identifies the bounding-box input layout.
   */
  enum OutputDataType : uint8
  {
    Split = 0,  ///< Uses separate minimum and maximum arrays.
    Unified = 1 ///< Uses one six-component bounds array.
  };

  /**
   * @brief Computes the selected statistics for each bounding box.
   * @return Success, or an input, storage, or temporary-file error.
   *
   * The direct path does not inspect the cancellation flag. The scanline path
   * returns success after cancellation. Mode entries or standard deviations
   * written before scanline cancellation remain in their outputs.
   *
   * Sums accumulate in the input value type. Integral sums can overflow before
   * conversion to the Float32 output.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeBoundingBoxStatsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
