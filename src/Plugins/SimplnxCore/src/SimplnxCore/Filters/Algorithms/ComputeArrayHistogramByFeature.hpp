#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{
/**
 * @struct ComputeArrayHistogramByFeatureInputValues
 * @brief Defines per-feature histogram inputs, outputs, range, mask, and feature IDs.
 */
struct SIMPLNXCORE_EXPORT ComputeArrayHistogramByFeatureInputValues
{
  bool UserDefinedRange = false;
  int32 NumberOfBins = 0;
  float64 MinRange = 0.0;
  float64 MaxRange = 0.0;
  MultiArraySelectionParameter::ValueType SelectedArrayPaths = {};
  MultiArraySelectionParameter::ValueType CreatedBinRangeDataPaths = {};
  MultiArraySelectionParameter::ValueType CreatedHistogramCountsDataPaths = {};
  MultiArraySelectionParameter::ValueType CreatedBinMostPopulatedDataPaths = {};
  std::optional<MultiArraySelectionParameter::ValueType> CreatedBinModalRangesDataPaths;
  DataPath FeatureIdsArrayPath;
  bool UseMask;
  DataPath MaskArrayPath;
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

private:
  DataStructure& m_DataStructure;
  const ComputeArrayHistogramByFeatureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
