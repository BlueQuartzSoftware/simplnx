#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <random>

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
  bool RandomizeParentIds = false;
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

private:
  DataStructure& m_DataStructure;
  const MergeTwinsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  std::vector<ebsdlib::LaueOps::Pointer> m_OrientationOps;

  std::mt19937_64 m_Generator = {};
  std::uniform_real_distribution<float32> m_Distribution = {};

  void groupFeaturesExecute();
  int getSeed(int32 newFid);
  bool determineGrouping(int32 referenceFeature, int32 neighborFeature, int32 newFid);
};

} // namespace nx::core
