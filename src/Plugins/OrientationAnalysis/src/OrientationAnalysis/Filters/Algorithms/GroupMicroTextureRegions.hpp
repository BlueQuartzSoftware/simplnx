#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include "EbsdLib/Math/Matrix3X1.hpp"

#include <random>

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT GroupMicroTextureRegionsInputValues
{
  bool UseNonContiguousNeighbors;
  DataPath NonContiguousNeighborListArrayPath;
  DataPath ContiguousNeighborListArrayPath;
  bool UseRunningAverage;
  float32 CAxisTolerance;
  DataPath FeatureIdsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath VolumesArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath NewCellFeatureAttributeMatrixName;
  DataPath CellParentIdsArrayName;
  DataPath FeatureParentIdsArrayName;
  bool RandomizeParentIds;
  uint64 SeedValue;
};

/**
 * @class GroupMicroTextureRegions
 * @brief This filter ...
 */
class ORIENTATIONANALYSIS_EXPORT GroupMicroTextureRegions
{
public:
  GroupMicroTextureRegions(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GroupMicroTextureRegionsInputValues* inputValues);
  ~GroupMicroTextureRegions() noexcept;

  GroupMicroTextureRegions(const GroupMicroTextureRegions&) = delete;
  GroupMicroTextureRegions(GroupMicroTextureRegions&&) noexcept = delete;
  GroupMicroTextureRegions& operator=(const GroupMicroTextureRegions&) = delete;
  GroupMicroTextureRegions& operator=(GroupMicroTextureRegions&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

protected:
  int getSeed(int32 newFid);
  bool determineGrouping(int32 referenceFeature, int32 neighborFeature, int32 newFid);
  Result<> execute();
  void randomizeParentIds(usize totalPoints, usize totalParentIds);

private:
  DataStructure& m_DataStructure;
  const GroupMicroTextureRegionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  usize m_NumTuples = 0;
  ebsdlib::Matrix3X1F m_AvgCAxes = {0.0f, 0.0f, 0.0f};
  std::mt19937_64 m_Generator = {};
  std::uniform_real_distribution<float32> m_Distribution = {};

  // These are so that we don't have to keep getting the references while we are running

  Int32Array& m_FeaturePhases;
  Int32Array& m_FeatureParentIds;
  UInt32Array& m_CrystalStructures;
  Float32Array& m_AvgQuats;
  Float32Array& m_Volumes;
};
} // namespace nx::core
