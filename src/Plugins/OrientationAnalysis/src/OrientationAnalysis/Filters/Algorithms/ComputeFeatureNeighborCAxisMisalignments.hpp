#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @brief Input values for the ComputeFeatureNeighborCAxisMisalignments algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeFeatureNeighborCAxisMisalignmentsInputValues
{
  bool FindAvgMisals;                      ///< If true, also compute the average misalignment per feature
  DataPath NeighborListArrayPath;          ///< Feature-level NeighborList of neighbor feature IDs
  DataPath AvgQuatsArrayPath;              ///< Feature-level Float32 average quaternions (4 components)
  DataPath FeaturePhasesArrayPath;         ///< Feature-level Int32 phase index per feature
  DataPath CrystalStructuresArrayPath;     ///< Ensemble-level UInt32 crystal structure Laue classes
  DataPath CAxisMisalignmentListArrayName; ///< Output: Feature-level NeighborList of c-axis misalignment angles (degrees)
  DataPath AvgCAxisMisalignmentsArrayName; ///< Output: Feature-level Float32 average c-axis misalignment (degrees)
};

/**
 * @class ComputeFeatureNeighborCAxisMisalignments
 * @brief Computes the c-axis misalignment angle between each Feature and its
 *        neighbors, plus optionally the per-Feature average misalignment.
 *
 * For each pair of neighboring features that share the same Hexagonal-High
 * phase, the c-axis direction of each feature is computed from its average
 * quaternion. The angle between the two c-axis directions gives the
 * misalignment, stored in degrees.
 *
 * ## OOC Optimization
 *
 * Feature-level arrays (phases, avgQuats) and ensemble-level crystal structures
 * are cached entirely in local vectors via `copyIntoBuffer()` at algorithm
 * start. The average misalignment output is accumulated in a local buffer and
 * written back via `copyFromBuffer()` at the end. Since this algorithm operates
 * on feature-level data (not cell-level), the arrays are typically small enough
 * to cache entirely, but using bulk I/O still avoids per-element virtual
 * dispatch overhead in the hot loop.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeFeatureNeighborCAxisMisalignments
{
public:
  ComputeFeatureNeighborCAxisMisalignments(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                           ComputeFeatureNeighborCAxisMisalignmentsInputValues* inputValues);
  ~ComputeFeatureNeighborCAxisMisalignments() noexcept;

  ComputeFeatureNeighborCAxisMisalignments(const ComputeFeatureNeighborCAxisMisalignments&) = delete;
  ComputeFeatureNeighborCAxisMisalignments(ComputeFeatureNeighborCAxisMisalignments&&) noexcept = delete;
  ComputeFeatureNeighborCAxisMisalignments& operator=(const ComputeFeatureNeighborCAxisMisalignments&) = delete;
  ComputeFeatureNeighborCAxisMisalignments& operator=(ComputeFeatureNeighborCAxisMisalignments&&) noexcept = delete;

  /**
   * @brief Executes the c-axis misalignment computation with locally cached data.
   * @return Result<> with any errors or warnings (e.g., non-hexagonal phases).
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureNeighborCAxisMisalignmentsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
