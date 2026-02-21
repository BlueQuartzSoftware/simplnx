
#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/UnionFind.hpp"

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
   * Uses an in-memory provisionalLabels buffer to avoid backward neighbor
   * reads from the OOC featureIdsStore.
   * @param featureIdsStore Feature IDs data store
   * @param unionFind Union-find structure for tracking equivalences
   * @param provisionalLabels Dense buffer mapping voxel index to provisional label (0 = not bad data)
   * @param nextLabel Output: next available label after Phase 1
   * @param dims Image geometry dimensions
   */
  static void phaseOneCCL(Int32AbstractDataStore& featureIdsStore, UnionFind& unionFind, std::vector<int32>& provisionalLabels, int32& nextLabel, const std::array<int64_t, 3>& dims);

  /**
   * @brief Phase 2: Global resolution of equivalences
   * @param unionFind Union-find structure to flatten
   */
  static void phaseTwoGlobalResolution(UnionFind& unionFind);

  /**
   * @brief Phase 3: Classify regions by size and relabel in featureIdsStore
   * Uses direct vector lookups instead of hash maps for classification.
   * @param featureIdsStore Feature IDs data store
   * @param cellPhasesPtr Cell phases array (could be null)
   * @param provisionalLabels Dense buffer mapping voxel index to provisional label
   * @param nextLabel Number of provisional labels assigned
   * @param unionFind Union-find with resolved equivalences
   * @param maxPhase Maximum phase value (for new phase assignment)
   */
  void phaseThreeRelabeling(Int32AbstractDataStore& featureIdsStore, Int32Array* cellPhasesPtr, const std::vector<int32>& provisionalLabels, int32 nextLabel, UnionFind& unionFind,
                            size_t maxPhase) const;

  /**
   * @brief Phase 4: Iterative morphological fill
   * @param featureIdsStore Feature IDs data store
   * @param dims Image geometry dimensions
   * @param numFeatures Number of features
   */
  void phaseFourIterativeFill(Int32AbstractDataStore& featureIdsStore, const std::array<int64_t, 3>& dims, size_t numFeatures) const;

  DataStructure& m_DataStructure;
  const FillBadDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
