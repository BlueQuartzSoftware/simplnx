#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include "EbsdLib/Math/Matrix3X1.hpp"

#include <random>
#include <vector>

namespace nx::core
{

/**
 * @struct GroupMicroTextureRegionsInputValues
 * @brief Identifies microtexture-region grouping inputs.
 */
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
 * @brief Groups compatible neighboring features into microtexture parent regions.
 *
 * Feature-level inputs are cached once for random-access grouping. Cell parent
 * IDs are then remapped with bounded bulk I/O to avoid per-cell OOC access.
 */
class ORIENTATIONANALYSIS_EXPORT GroupMicroTextureRegions
{
public:
  /**
   * @brief Initializes microtexture-region grouping.
   * @param dataStructure Provides selected arrays.
   * @param mesgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies grouping settings.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  GroupMicroTextureRegions(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GroupMicroTextureRegionsInputValues* inputValues);
  /**
   * @brief Destroys the microtexture-region executor.
   */
  ~GroupMicroTextureRegions() noexcept;

  GroupMicroTextureRegions(const GroupMicroTextureRegions&) = delete;
  GroupMicroTextureRegions(GroupMicroTextureRegions&&) noexcept = delete;
  GroupMicroTextureRegions& operator=(const GroupMicroTextureRegions&) = delete;
  GroupMicroTextureRegions& operator=(GroupMicroTextureRegions&&) noexcept = delete;

  /**
   * @brief Groups compatible features.
   * @return Success, or an error for an invalid Phase index, grouping, or cell-parent remapping.
   */
  Result<> operator()();

  /**
   * @brief Returns the retained cancellation flag.
   * @return Reference to the cancellation flag supplied at construction.
   */
  const std::atomic_bool& getCancel();

protected:
  int getSeed(int32 newFid);

  /**
   * @brief Determines whether an eligible neighboring feature joins the current parent.
   * @param referenceFeatureIdx Index of the current parent-group feature.
   * @param neighborFeatureIdx Index of the neighboring feature.
   * @param newFid Parent feature identifier assigned when the features group.
   * @return True if the neighbor joins, false if the pair is ineligible, or an error for an invalid Phase index.
   */
  Result<bool> determineGrouping(int32 referenceFeatureIdx, int32 neighborFeatureIdx, int32 newFid);
  Result<> execute();
  Result<> cacheFeatureData();
  Result<> remapCellParentIds();
  void randomizeParentIds(usize totalParentIds);

private:
  DataStructure& m_DataStructure;
  const GroupMicroTextureRegionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  usize m_NumTuples = 0;
  ebsdlib::Matrix3X1F m_AvgCAxes = {0.0f, 0.0f, 0.0f};
  std::mt19937_64 m_Generator = {};
  std::uniform_real_distribution<float32> m_Distribution = {};

  Int32Array& m_FeaturePhases;
  Int32Array& m_FeatureParentIds;
  UInt32Array& m_CrystalStructures;
  Float32Array& m_AvgQuats;
  Float32Array& m_Volumes;

  std::vector<int32> m_FeaturePhasesCache;
  std::vector<int32> m_FeatureParentIdsCache;
  std::vector<uint32> m_CrystalStructuresCache;
  std::vector<float32> m_AvgQuatsCache;
  std::vector<float32> m_VolumesCache;
};
} // namespace nx::core
