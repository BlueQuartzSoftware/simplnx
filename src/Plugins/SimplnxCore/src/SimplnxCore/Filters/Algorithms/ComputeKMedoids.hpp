#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"

namespace nx::core
{
/**
 * @struct KMedoidsInputValues
 * @brief Collects K-Medoids settings and DataStructure paths.
 */
struct SIMPLNXCORE_EXPORT KMedoidsInputValues
{
  uint64 InitClusters;
  ClusterUtilities::DistanceMetric DistanceMetric;
  bool UseMask = false;
  DataPath ClusteringArrayPath;
  DataPath MaskArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath MedoidsArrayPath;
  uint64 Seed;
};

/**
 * @class ComputeKMedoids
 * @brief Dispatches K-Medoids clustering by array storage type.
 *
 * Voronoi iterations assign tuples to the nearest medoid. Each cluster then
 * selects the member with the lowest total intra-cluster distance. The direct
 * path preserves fast resident access. The scanline path uses bounded tiles to
 * prevent out-of-core chunk thrashing. Overrides can force either path.
 */
class SIMPLNXCORE_EXPORT ComputeKMedoids
{
public:
  /**
   * @brief Initializes the K-Medoids dispatcher.
   * @param dataStructure Contains input and output arrays.
   * @param mesgHandler Receives iteration messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects settings and array paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  ComputeKMedoids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, KMedoidsInputValues* inputValues);
  ~ComputeKMedoids() noexcept;

  ComputeKMedoids(const ComputeKMedoids&) = delete;
  ComputeKMedoids(ComputeKMedoids&&) noexcept = delete;
  ComputeKMedoids& operator=(const ComputeKMedoids&) = delete;
  ComputeKMedoids& operator=(ComputeKMedoids&&) noexcept = delete;

  /**
   * @brief Executes the selected K-Medoids implementation.
   * @return Success, or a mask, shape, overflow, recovery, or transfer error.
   * @pre InitClusters fits in Int32.
   * @pre Input and output arrays have compatible tuple and component shapes.
   *
   * Initial medoids are sampled with replacement, so duplicates are permitted.
   * Exact medoid-index equality controls convergence. There is no iteration
   * limit. Masked tuples receive reserved cluster ID zero.
   *
   * Cancellation returns success. Assignments or medoids written before the
   * cancellation checkpoint remain in the output arrays.
   */
  Result<> operator()();

  void updateProgress(const std::string& message);

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const KMedoidsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
