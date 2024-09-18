#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

namespace nx::core
{
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
};

/**
 * @class
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

  Result<> operator()();

  const std::atomic_bool& getCancel();

  int getSeed(int32 newFid) const;
  bool determineGrouping(int32 referenceFeature, int32 neighborFeature, int32 newFid);

private:
  DataStructure& m_DataStructure;
  const MergeTwinsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  std::vector<LaueOps::Pointer> m_OrientationOps;
};

} // namespace nx::core
