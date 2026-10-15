#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @struct ComputeFeatureCentroidsInputValues
 * @brief Stores filter values for feature-centroid execution.
 */
struct SIMPLNXCORE_EXPORT ComputeFeatureCentroidsInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath CentroidsArrayPath;
  DataPath ImageGeometryPath;
  DataPath FeatureAttributeMatrixPath;
  bool IsPeriodic = false; ///< True to adjust features that wrap across periodic geometry faces.
};

/**
 * @class ComputeFeatureCentroids
 * @brief Computes ImageGeom feature centroids with Kahan summation.
 *
 * Feature IDs use 65,536-tuple bulk reads. Kahan sums, counts, and periodic ranges use
 * feature-sized vectors. The periodic path uses a bounded occupied-coordinate bitset and
 * an additional weighted Kahan pass for adjusted axes. It does not retain per-voxel
 * coordinates in memory. The centroid array uses one final bulk write.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureCentroids
{
public:
  /**
   * @brief Initializes the feature-centroid algorithm.
   * @param dataStructure Contains the ImageGeom, Feature IDs, and centroids.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation between Feature ID chunks.
   * @param inputValues Identifies required objects and periodic behavior.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeFeatureCentroids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureCentroidsInputValues* inputValues);
  /**
   * @brief Destroys the feature-centroid algorithm.
   */
  ~ComputeFeatureCentroids() noexcept;

  ComputeFeatureCentroids(const ComputeFeatureCentroids&) = delete;
  ComputeFeatureCentroids(ComputeFeatureCentroids&&) noexcept = delete;
  ComputeFeatureCentroids& operator=(const ComputeFeatureCentroids&) = delete;
  ComputeFeatureCentroids& operator=(ComputeFeatureCentroids&&) noexcept = delete;

  /**
   * @brief Computes feature centroids.
   * @return Success or a validation, working-memory, or store I/O error.
   *
   * When a checkpoint observes cancellation, the method returns before the final centroid
   * bulk write. A failed final write follows the store's existing partial-write contract.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureCentroidsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
