
#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nx::core
{

// Forward declarations
template <typename T>
class DataArray;
using Int32Array = DataArray<int32>;

template <typename T>
class AbstractDataStore;
using Int32AbstractDataStore = AbstractDataStore<int32>;

/**
 * @class ChunkAwareUnionFind
 * @brief Union-Find data structure for tracking connected component equivalences across chunks
 */
class SIMPLNXCORE_EXPORT ChunkAwareUnionFind
{
public:
  ChunkAwareUnionFind() = default;
  ~ChunkAwareUnionFind() = default;

  /**
   * @brief Find the root label with path compression
   * @param x Label to find
   * @return Root label
   */
  int64 find(int64 x);

  /**
   * @brief Unite two labels into the same equivalence class
   * @param a First label
   * @param b Second label
   */
  void unite(int64 a, int64 b);

  /**
   * @brief Add to the size count for a label
   * @param label Label to update
   * @param count Number of voxels to add
   */
  void addSize(int64 label, uint64 count);

  /**
   * @brief Get the total size of a label's equivalence class
   * @param label Label to query
   * @return Total number of voxels in the equivalence class
   */
  uint64 getSize(int64 label);

  /**
   * @brief Flatten the union-find structure and sum sizes to roots
   */
  void flatten();

private:
  std::unordered_map<int64, int64> m_Parent;
  std::unordered_map<int64, int32> m_Rank;
  std::unordered_map<int64, uint64> m_Size;
};

struct SIMPLNXCORE_EXPORT FillBadDataInputValues
{
  int32 minAllowedDefectSizeValue;
  bool storeAsNewPhase;
  DataPath featureIdsArrayPath;
  DataPath cellPhasesArrayPath;
  std::vector<DataPath> ignoredDataArrayPaths;
  DataPath inputImageGeometry;
};

/**
 * @class FillBadData

 */
class SIMPLNXCORE_EXPORT FillBadData
{
public:
  FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FillBadDataInputValues* inputValues);
  ~FillBadData() noexcept;

  FillBadData(const FillBadData&) = delete;
  FillBadData(FillBadData&&) noexcept = delete;
  FillBadData& operator=(const FillBadData&) = delete;
  FillBadData& operator=(FillBadData&&) noexcept = delete;

  Result<> operator()() const;

  const std::atomic_bool& getCancel() const;

private:
  /**
   * @brief Phase 1: Chunk-sequential connected component labeling
   * @param featureIdsStore Feature IDs data store
   * @param unionFind Union-find structure for tracking equivalences
   * @param provisionalLabels Map from voxel index to provisional label
   * @param dims Image geometry dimensions
   */
  static void phaseOneCCL(Int32AbstractDataStore& featureIdsStore, ChunkAwareUnionFind& unionFind, std::unordered_map<usize, int64>& provisionalLabels, const std::array<int64_t, 3>& dims);

  /**
   * @brief Phase 2: Global resolution of equivalences and region classification
   * @param unionFind Union-find structure to flatten
   * @param smallRegions Output set of labels for small regions that need filling
   */
  static void phaseTwoGlobalResolution(ChunkAwareUnionFind& unionFind, std::unordered_set<int64>& smallRegions);

  /**
   * @brief Phase 3: Relabel voxels based on region classification
   * @param featureIdsStore Feature IDs data store
   * @param cellPhasesPtr Cell phases array (could be null)
   * @param provisionalLabels Map from voxel index to provisional label
   * @param smallRegions Set of labels for small regions
   * @param unionFind Union-find for looking up equivalences
   * @param maxPhase Maximum phase value (for new phase assignment)
   */
  void phaseThreeRelabeling(Int32AbstractDataStore& featureIdsStore, Int32Array* cellPhasesPtr, const std::unordered_map<usize, int64>& provisionalLabels,
                            const std::unordered_set<int64>& smallRegions, ChunkAwareUnionFind& unionFind, size_t maxPhase) const;

  /**
   * @brief Phase 4: Iterative morphological fill
   * @param featureIdsStore Feature IDs data store
   * @param dims Image geometry dimensions
   * @param numFeatures Number of features
   */
  void phaseFourIterativeFill(Int32AbstractDataStore& featureIdsStore, const std::array<int64_t, 3>& dims, size_t numFeatures) const;

  /**
   * @brief Get the 6 face-connected neighbor offsets
   * @param dims Image geometry dimensions
   * @return Array of 6 neighbor offsets
   */
  static std::array<int64_t, 6> getNeighborOffsets(const std::array<int64_t, 3>& dims);

  /**
   * @brief Check if a neighbor direction is valid for a given voxel position
   * @param neighborIdx Neighbor direction index (0-5)
   * @param column X coordinate
   * @param row Y coordinate
   * @param plane Z coordinate
   * @param dims Image geometry dimensions
   * @return True if the neighbor is valid (not out of bounds)
   */
  static bool isValidNeighbor(int32 neighborIdx, int64_t column, int64_t row, int64_t plane, const std::array<int64_t, 3>& dims);

  DataStructure& m_DataStructure;
  const FillBadDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
