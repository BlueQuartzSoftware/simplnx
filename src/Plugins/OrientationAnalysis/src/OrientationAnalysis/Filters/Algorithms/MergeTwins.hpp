#pragma once

#include "OrientationAnalysis/Filters/Algorithms/GroupFeatures.hpp"
#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT MergeTwinsInputValues
{
  bool UseNonContiguousNeighbors;
  DataPath NonContiguousNeighborListArrayPath;
  DataPath ContiguousNeighborListArrayPath;
  float32 AxisTolerance;
  float32 AngleTolerance;
  DataPath FeaturePhasesArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath CellParentIdsArrayName;
  DataPath NewCellFeatureAttributeMatrixName;
  DataPath FeatureParentIdsArrayName;
  DataPath ActiveArrayName;
  bool RandomizeParentIds = true;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT MergeTwins : public GroupFeatures
{
public:
  MergeTwins(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MergeTwinsInputValues* inputValues,
             GroupFeaturesInputValues* groupInputValues);
  ~MergeTwins() noexcept;

  MergeTwins(const MergeTwins&) = delete;
  MergeTwins(MergeTwins&&) noexcept = delete;
  MergeTwins& operator=(const MergeTwins&) = delete;
  MergeTwins& operator=(MergeTwins&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  void characterize_twins();
  int getSeed(int32 newFid) override;
  bool determineGrouping(int32 referenceFeature, int32 neighborFeature, int32 newFid) override;

private:
  const MergeTwinsInputValues* m_InputValues = nullptr;
};

} // namespace complex
