#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"

namespace nx::core
{

/**
 * @struct ComputeKMeansInputValues
 * @brief Collects K-Means settings and DataStructure paths.
 */
struct SIMPLNXCORE_EXPORT ComputeKMeansInputValues
{
  uint64 InitClusters;
  ClusterUtilities::DistanceMetric DistanceMetric;
  bool UseMask = false;
  DataPath ClusteringArrayPath;
  DataPath MaskArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath MeansArrayPath;
  uint64 Seed;
};

/**
 * @class ComputeKMeans
 * @brief Dispatches K-Means clustering by array storage type.
 *
 * Lloyd iterations assign tuples to the nearest centroid and then recompute
 * arithmetic means. The direct path preserves fast resident element access.
 * The scanline path prevents chunk thrashing by using bounded bulk transfers.
 * Storage override settings can force either path.
 */
class SIMPLNXCORE_EXPORT ComputeKMeans
{
public:
  /**
   * @brief Initializes the K-Means dispatcher.
   * @param dataStructure Contains input and output arrays.
   * @param mesgHandler Receives iteration messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects settings and array paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  ComputeKMeans(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeKMeansInputValues* inputValues);
  ~ComputeKMeans() noexcept;

  ComputeKMeans(const ComputeKMeans&) = delete;
  ComputeKMeans(ComputeKMeans&&) noexcept = delete;
  ComputeKMeans& operator=(const ComputeKMeans&) = delete;
  ComputeKMeans& operator=(ComputeKMeans&&) noexcept = delete;

  /**
   * @brief Executes the selected K-Means implementation.
   * @return Success, or a mask, shape, overflow, or bulk-transfer error.
   * @pre InitClusters is positive and fits in Int32.
   * @pre Input and output arrays have compatible tuple and component shapes.
   *
   * Initial centroids are sampled with replacement. For more than one tuple,
   * the legacy index formula does not select the final tuple. Initialization
   * cannot finish when only that tuple is selected by the mask. The direct path
   * does not check cancellation during this selection loop.
   *
   * Convergence reads flat means indices 1 through K instead of all centroid
   * components. There is no iteration limit. Centroid arithmetic uses the input
   * type, so integral input can overflow or truncate means.
   *
   * Cancellation returns success. Assignments and centroids written before the
   * cancellation checkpoint remain in the output arrays.
   */
  Result<> operator()();

  void updateProgress(const std::string& message);

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeKMeansInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
