#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include <mutex>

namespace nx::core
{
/**
 * @struct ComputeArrayHistogramByFeatureInputValues
 * @brief Defines per-feature histogram inputs, outputs, range, mask, and feature IDs.
 */
struct SIMPLNXCORE_EXPORT ComputeArrayHistogramByFeatureInputValues
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
  DataPath feature_ids_array_path;
  bool use_mask{};
  DataPath mask_array_path;
};

/**
 * @class ComputeArrayHistogramByFeature
 * @brief Computes per-feature histograms, bin ranges, most-populated bins, and
 * optional modal-bin ranges for one or more scalar cell arrays.
 *
 * Resident arrays use the established parallel feature implementation. If any
 * participating input or output is out-of-core, a bounded multi-pass scan routes
 * each cell directly into its feature histogram. Exact modal values use external
 * sorting when available and an exact repeated-scan fallback otherwise.
 */
class SIMPLNXCORE_EXPORT ComputeArrayHistogramByFeature
{
public:
  /**
   * @brief Initializes per-feature histogram calculation.
   * @param dataStructure Provides source, feature, mask, and output arrays.
   * @param msgHandler Receives phase progress.
   * @param shouldCancel Signals cancellation between bounded passes.
   * @param inputValues Defines histogram, feature, and output settings.
   * @pre All arguments outlive this executor.
   */
  ComputeArrayHistogramByFeature(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ComputeArrayHistogramByFeatureInputValues* inputValues);
  ~ComputeArrayHistogramByFeature() noexcept;

  ComputeArrayHistogramByFeature(const ComputeArrayHistogramByFeature&) = delete;
  ComputeArrayHistogramByFeature(ComputeArrayHistogramByFeature&&) noexcept = delete;
  ComputeArrayHistogramByFeature& operator=(const ComputeArrayHistogramByFeature&) = delete;
  ComputeArrayHistogramByFeature& operator=(ComputeArrayHistogramByFeature&&) noexcept = delete;

  /**
   * @brief Discovers feature count and computes each selected array.
   * @return Validation, storage, external-sort, or fallback reduction errors.
   *
   * Dispatch includes all participating input and output stores.
   */
  Result<> operator()();

  /**
   * @brief Thread-safe progress update. Safe to call from ParallelDataAlgorithm workers.
   * @param counter Features completed since the previous call
   */
  void sendThreadSafeProgressMessage(usize counter);

private:
  DataStructure& m_DataStructure;
  const ComputeArrayHistogramByFeatureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  mutable std::mutex m_ProgressMessageMutex;
  ThrottledMessageHandler m_Throttle;
};
} // namespace nx::core
