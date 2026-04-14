#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <random>

namespace nx::core
{
/**
 * @brief Input values for the MergeTwins algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT MergeTwinsInputValues
{
  DataPath ContiguousNeighborListArrayPath;   ///< Feature-level NeighborList of contiguous neighbors
  float32 AxisTolerance;                      ///< Tolerance (degrees) for the twin axis direction
  float32 AngleTolerance;                     ///< Tolerance (degrees) for the twin misorientation angle
  DataPath FeaturePhasesArrayPath;            ///< Feature-level Int32 phase index per feature
  DataPath AvgQuatsArrayPath;                 ///< Feature-level Float32 average quaternions (4 components)
  DataPath FeatureIdsArrayPath;               ///< Cell-level Int32 feature ID per voxel
  DataPath CrystalStructuresArrayPath;        ///< Ensemble-level UInt32 crystal structure Laue classes
  DataPath CellParentIdsArrayPath;            ///< Output: Cell-level Int32 parent feature ID
  DataPath NewCellFeatureAttributeMatrixPath; ///< Output: AttributeMatrix for new parent features
  DataPath FeatureParentIdsArrayPath;         ///< Output: Feature-level Int32 parent feature ID
  DataPath ActiveArrayPath;                   ///< Output: Feature-level bool active status
  uint64 Seed;                                ///< Random seed for parent ID randomization
  bool RandomizeParentIds = false;            ///< Whether to randomize the order of parent IDs
};

/**
 * @class MergeTwins
 * @brief Groups neighboring Features that share a sigma-3 twin relationship
 *        (FCC, 60 degrees about <111>) into parent features.
 *
 * Only Cubic-High (m3m) Laue class features are considered. The algorithm
 * compares average orientations of neighboring features against the sigma-3
 * twin misorientation within user-specified axis and angle tolerances.
 *
 * ## OOC Optimization
 *
 * The voxel-level pass that assigns cellParentIds from featureParentIds is
 * the only cell-level operation. It uses chunked bulk I/O:
 *   - `cellParentIds` is filled with -1 in chunks via `copyFromBuffer()`.
 *   - `featureIds` are read in chunks of 65536 via `copyIntoBuffer()`.
 *   - `featureParentIds` are cached locally (feature-level, small).
 *   - The computed `cellParentIds` are written back in matching chunks.
 *
 * This avoids per-voxel virtual dispatch that would cause OOC chunk thrashing
 * during the cell-level parent ID assignment loop.
 */
class ORIENTATIONANALYSIS_EXPORT MergeTwins
{
public:
  MergeTwins(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MergeTwinsInputValues* inputValues);
  ~MergeTwins() noexcept;

  MergeTwins(const MergeTwins&) = delete;
  MergeTwins(MergeTwins&&) noexcept = delete;
  MergeTwins& operator=(const MergeTwins&) = delete;
  MergeTwins& operator=(MergeTwins&&) noexcept = delete;

  /**
   * @brief Executes twin merging and assigns parent IDs using chunked bulk I/O.
   * @return Result<> with any errors or warnings encountered.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const MergeTwinsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  std::vector<ebsdlib::LaueOps::Pointer> m_OrientationOps;

  std::mt19937_64 m_Generator = {};
  std::uniform_real_distribution<float32> m_Distribution = {};

  /** @brief Iterates over features, grouping twins into parent features. */
  void groupFeaturesExecute();
  /** @brief Returns the seed feature for a new parent group. */
  int getSeed(int32 newFid);
  /** @brief Tests if two features satisfy the sigma-3 twin relationship. */
  bool determineGrouping(int32 referenceFeature, int32 neighborFeature, int32 newFid);
};

} // namespace nx::core
