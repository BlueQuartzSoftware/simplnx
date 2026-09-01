#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"

namespace nx::core
{

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @struct ComputeFeatureClusteringInputValues
 * @brief Stores filter values for feature-clustering execution.
 */
struct SIMPLNXCORE_EXPORT ComputeFeatureClusteringInputValues
{
  DataPath ImageGeometryPath;
  int32 NumberOfBins;
  int32 PhaseNumber;
  bool RemoveBiasedFeatures;
  uint64 SeedValue; ///< Seeds the deterministic random reference distribution.
  DataPath FeaturePhasesArrayPath;
  DataPath CentroidsArrayPath;
  DataPath BiasedFeaturesArrayPath;
  DataPath CellEnsembleAttributeMatrixName;
  DataPath ClusteringListArrayName;
  DataPath RDFArrayName;
  DataPath MaxMinArrayName;
};

/**
 * @class ComputeFeatureClustering
 * @brief Computes a phase radial distribution function from pair distances.
 *
 * Feature phases and centroids are copied to local vectors before the O(n^2) pair loop. The RDF
 * uses a local histogram and one final bulk write. Pair distances remain in per-feature lists, so
 * this algorithm can use O(n^2) resident memory for the selected phase.
 *
 * Current bulk-I/O Result values are not inspected. A storage failure can leave partial output while
 * the method returns success.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureClustering
{
public:
  /**
   * @brief Initializes the feature-clustering algorithm.
   * @param dataStructure Contains the ImageGeom, feature arrays, and outputs.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation between outer feature iterations.
   * @param inputValues Selects phase, bins, and output objects.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeFeatureClustering(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureClusteringInputValues* inputValues);
  /**
   * @brief Destroys the feature-clustering algorithm.
   */
  ~ComputeFeatureClustering() noexcept;

  ComputeFeatureClustering(const ComputeFeatureClustering&) = delete;
  ComputeFeatureClustering(ComputeFeatureClustering&&) noexcept = delete;
  ComputeFeatureClustering& operator=(const ComputeFeatureClustering&) = delete;
  ComputeFeatureClustering& operator=(ComputeFeatureClustering&&) noexcept = delete;

  /**
   * @brief Computes the selected phase RDF and neighbor lists.
   * @return Success, or a biased-feature mask error.
   *
   * Cancellation returns success between outer feature iterations. Minimum and maximum values can
   * be written before cancellation prevents RDF or neighbor-list output.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureClusteringInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
