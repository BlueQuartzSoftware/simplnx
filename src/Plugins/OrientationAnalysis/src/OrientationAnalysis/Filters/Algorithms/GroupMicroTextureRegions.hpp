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
  bool use_non_contiguous_neighbors{};
  DataPath non_contiguous_neighbor_list_array_path;
  DataPath contiguous_neighbor_list_array_path;
  bool use_running_average{};
  float32 c_axis_tolerance{};
  DataPath feature_ids_array_path;
  DataPath feature_phases_array_path;
  DataPath volumes_array_path;
  DataPath avg_quats_array_path;
  DataPath crystal_structures_array_path;
  DataPath new_cell_feature_attribute_matrix_name;
  DataPath cell_parent_ids_array_name;
  DataPath feature_parent_ids_array_name;
  bool randomize_parent_ids{};
  uint64 seed_value{};
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
  std::mt19937_64 m_Generator;
  std::uniform_real_distribution<float32> m_Distribution;

  Int32ArrayType& m_FeaturePhases;
  Int32ArrayType& m_FeatureParentIds;
  UInt32ArrayType& m_CrystalStructures;
  Float32ArrayType& m_AvgQuats;
  Float32ArrayType& m_Volumes;

  std::vector<int32> m_FeaturePhasesCache;
  std::vector<int32> m_FeatureParentIdsCache;
  std::vector<uint32> m_CrystalStructuresCache;
  std::vector<float32> m_AvgQuatsCache;
  std::vector<float32> m_VolumesCache;
};
} // namespace nx::core
