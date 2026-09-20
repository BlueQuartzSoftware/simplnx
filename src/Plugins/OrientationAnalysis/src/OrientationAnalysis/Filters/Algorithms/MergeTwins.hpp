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
 * @struct MergeTwinsInputValues
 * @brief Identifies twin-merging inputs.
 *
 * AxisTolerance and AngleTolerance are in degrees.
 */
struct ORIENTATIONANALYSIS_EXPORT MergeTwinsInputValues
{
  DataPath ContiguousNeighborListArrayPath;
  float32 AxisTolerance;
  float32 AngleTolerance;
  DataPath FeaturePhasesArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath CellParentIdsArrayPath;
  DataPath NewCellFeatureAttributeMatrixPath;
  DataPath FeatureParentIdsArrayPath;
  DataPath ActiveArrayPath;
  uint64 Seed;
  bool RandomizeParentIds = false;
};

/**
 * @class MergeTwins
 * @brief Groups neighboring sigma-3 twins into parent features.
 *
 * Cubic features compare average orientations to the 60-degree [111]
 * relationship. Cell parent IDs use 65,536-tuple bulk transfers. Feature
 * parent IDs stay local for random cell-to-feature lookup.
 */
class ORIENTATIONANALYSIS_EXPORT MergeTwins
{
public:
  /**
   * @brief Initializes twin merging.
   * @param dataStructure Provides selected arrays.
   * @param mesgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies selected arrays and tolerances.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  MergeTwins(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MergeTwinsInputValues* inputValues);
  /**
   * @brief Destroys the twin-merging executor.
   */
  ~MergeTwins() noexcept;

  MergeTwins(const MergeTwins&) = delete;
  MergeTwins(MergeTwins&&) noexcept = delete;
  MergeTwins& operator=(const MergeTwins&) = delete;
  MergeTwins& operator=(MergeTwins&&) noexcept = delete;

  /**
   * @brief Merges twin features and assigns parent IDs.
   * @return Success, or an error for an invalid Phase index, grouping, or cell-parent assignment.
   */
  Result<> operator()();

  /**
   * @brief Returns the retained cancellation flag.
   * @return Reference to the cancellation flag supplied at construction.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const MergeTwinsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  std::vector<ebsdlib::LaueOps::Pointer> m_OrientationOps;

  std::mt19937_64 m_Generator = {};
  std::uniform_real_distribution<float32> m_Distribution = {};

  /**
   * @brief Groups connected twin features.
   * @return Success, cancellation, or the first Phase-index or feature-matrix resize error.
   */
  Result<> groupFeaturesExecute();

  /**
   * @brief Selects and initializes one unassigned feature seed.
   * @param newFid Parent feature identifier assigned to the seed.
   * @return Seed index, or the first feature-matrix resize error.
   */
  Result<int32> getSeed(int32 newFid);

  /**
   * @brief Determines whether an eligible neighboring feature is a sigma-3 twin.
   * @param referenceFeatureIdx Index of the current parent-group feature.
   * @param neighborFeatureIdx Index of the neighboring feature.
   * @param newFid Parent feature identifier assigned when the features merge.
   * @return True if the neighbor merges, false if the pair is ineligible, or an error for an invalid Phase index.
   */
  Result<bool> determineGrouping(int32 referenceFeatureIdx, int32 neighborFeatureIdx, int32 newFid);
};

} // namespace nx::core
