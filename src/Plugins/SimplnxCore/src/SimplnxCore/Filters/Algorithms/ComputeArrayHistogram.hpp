#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{
/**
 * @struct ComputeArrayHistogramInputValues
 * @brief Defines histogram ranges, inputs, outputs, mask, and modal-range options.
 */
struct SIMPLNXCORE_EXPORT ComputeArrayHistogramInputValues
{
  bool user_defined_range = false;
  int32 number_of_bins = 0;
  float64 min_range = 0.0;
  float64 max_range = 0.0;
  MultiArraySelectionParameter::ValueType selected_array_paths;
  MultiArraySelectionParameter::ValueType created_bin_range_data_paths;
  MultiArraySelectionParameter::ValueType created_histogram_counts_data_paths;
  MultiArraySelectionParameter::ValueType created_bin_most_populated_data_paths;
  std::optional<MultiArraySelectionParameter::ValueType> created_bin_modal_ranges_data_paths;
  std::optional<DataPath> mask_path;
};

/**
 * @class ComputeArrayHistogram
 * @brief Computes histograms and optional exact modal ranges for numeric arrays.
 *
 * Range and histogram passes use sequential 65,536-value pages for all stores.
 * Resident modal calculation can retain every selected value. Out-of-core modal
 * calculation uses an exact external merge sort in a unique temporary directory.
 * It can require two temporary files proportional to the selected value count.
 *
 * Cancellation returns success and removes temporary files. Outputs from prior
 * arrays and earlier passes remain. The legacy modal-range calculation interprets
 * the paired lower/upper range array as one edge sequence; modal ranges can differ
 * from the stored histogram bins. NaN equivalence can also differ between direct
 * and external-sort modal paths.
 */
class SIMPLNXCORE_EXPORT ComputeArrayHistogram
{
public:
  /**
   * @brief Initializes histogram calculation.
   * @param dataStructure Provides source, mask, and output arrays.
   * @param msgHandler Receives overflow messages.
   * @param shouldCancel Signals cancellation between pages and sort passes.
   * @param inputValues Defines histogram and output settings.
   * @pre All arguments outlive this executor.
   */
  ComputeArrayHistogram(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ComputeArrayHistogramInputValues* inputValues);
  ~ComputeArrayHistogram() noexcept = default;

  ComputeArrayHistogram(const ComputeArrayHistogram&) = delete;
  ComputeArrayHistogram(ComputeArrayHistogram&&) noexcept = delete;
  ComputeArrayHistogram& operator=(const ComputeArrayHistogram&) = delete;
  ComputeArrayHistogram& operator=(ComputeArrayHistogram&&) noexcept = delete;

  /**
   * @brief Computes each selected array in input order.
   * @return Range, mask, bulk-I/O, temporary-file, or modal-sort errors.
   * @pre Output-path collections match input count.
   * @pre NumberOfBins is at least one and mask type is Bool or UInt8.
   * @pre The mask has component shape {1} and one tuple per tuple of each input array.
   * @pre An automatic integral range can increment its maximum without overflow.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeArrayHistogramInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
