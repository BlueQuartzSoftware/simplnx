#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct ComputeFeatureClusteringInputValues
 * @brief Holds all user-configured parameters for the ComputeFeatureClustering algorithm.
 */
struct SIMPLNXCORE_EXPORT ComputeFeatureClusteringInputValues
{
  DataPath ImageGeometryPath;               ///< Path to the ImageGeom providing box dimensions.
  int32 NumberOfBins;                       ///< Number of histogram bins for the RDF.
  int32 PhaseNumber;                        ///< Ensemble/phase to compute clustering for.
  bool RemoveBiasedFeatures;                ///< If true, exclude features flagged as biased.
  uint64 SeedValue;                         ///< Random seed for the reference random distribution.
  DataPath FeaturePhasesArrayPath;          ///< Per-feature phase/ensemble ID array.
  DataPath CentroidsArrayPath;              ///< Per-feature centroid array (float32, 3-component).
  DataPath BiasedFeaturesArrayPath;         ///< Per-feature bias flag array (used when RemoveBiasedFeatures is true).
  DataPath CellEnsembleAttributeMatrixName; ///< Ensemble-level Attribute Matrix.
  DataPath ClusteringListArrayName;         ///< Output: NeighborList of inter-feature distances.
  DataPath RDFArrayName;                    ///< Output: RDF histogram array (float32).
  DataPath MaxMinArrayName;                 ///< Output: min/max separation distances (float32, 2-component).
};

/**
 * @class ComputeFeatureClustering
 * @brief Computes the radial distribution function (RDF) for features of a
 * specified phase by measuring all pairwise inter-centroid distances and
 * normalizing against a random reference distribution.
 *
 * The algorithm has O(n^2) complexity in the number of features of the target
 * phase, since it computes all pairwise distances. The RDF is binned into a
 * user-specified number of equal-width bins spanning the minimum to maximum
 * inter-feature distance, then normalized by a Monte Carlo random distribution.
 *
 * @section ooc_optimization Out-of-Core Optimization
 * The feature-level arrays (FeaturePhases, Centroids, RDF) are accessed in the
 * inner O(n^2) loop. For OOC data, per-element virtual dispatch on every access
 * inside a quadratic loop is prohibitively expensive. The optimized implementation
 * bulk-reads the entire FeaturePhases and Centroids arrays into local std::vectors
 * via copyIntoBuffer() at the start, and accumulates RDF bins into a local vector.
 * The final RDF is written back to the output DataStore in a single copyFromBuffer()
 * call after normalization.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureClustering
{
public:
  ComputeFeatureClustering(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureClusteringInputValues* inputValues);
  ~ComputeFeatureClustering() noexcept;

  ComputeFeatureClustering(const ComputeFeatureClustering&) = delete;
  ComputeFeatureClustering(ComputeFeatureClustering&&) noexcept = delete;
  ComputeFeatureClustering& operator=(const ComputeFeatureClustering&) = delete;
  ComputeFeatureClustering& operator=(ComputeFeatureClustering&&) noexcept = delete;

  /**
   * @brief Executes the RDF clustering computation.
   * @return Result<> indicating success or error.
   */
  Result<> operator()();

  /**
   * @brief Returns the cancellation flag reference.
   * @return const reference to the atomic cancellation boolean.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;                                     ///< Reference to the DataStructure.
  const ComputeFeatureClusteringInputValues* m_InputValues = nullptr; ///< User-configured parameters.
  const std::atomic_bool& m_ShouldCancel;                             ///< Cancellation flag.
  const IFilter::MessageHandler& m_MessageHandler;                    ///< Message handler for progress.
};

} // namespace nx::core
