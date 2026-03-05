#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/UnionFind.hpp"

namespace nx::core
{

// Forward declarations
template <typename T>
class DataArray;
using Int32Array = DataArray<int32>;

template <typename T>
class AbstractDataStore;
using Int32AbstractDataStore = AbstractDataStore<int32>;

struct FillBadDataInputValues;

/**
 * @class FillBadDataCCL
 * @brief CCL-based algorithm for filling bad data regions, optimized for out-of-core.
 *
 * Uses chunk-sequential connected component labeling with a 2-slice rolling buffer
 * to avoid O(N) memory allocations. Designed for datasets that may exceed available RAM.
 *
 * @see FillBadDataBFS for the in-core-optimized alternative.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism that selects between them.
 */
class SIMPLNXCORE_EXPORT FillBadDataCCL
{
public:
  FillBadDataCCL(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const FillBadDataInputValues* inputValues);
  ~FillBadDataCCL() noexcept;

  FillBadDataCCL(const FillBadDataCCL&) = delete;
  FillBadDataCCL(FillBadDataCCL&&) noexcept = delete;
  FillBadDataCCL& operator=(const FillBadDataCCL&) = delete;
  FillBadDataCCL& operator=(FillBadDataCCL&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel() const;

private:
  static void phaseOneCCL(Int32AbstractDataStore& featureIdsStore, UnionFind& unionFind, int32& nextLabel, const std::array<int64_t, 3>& dims);
  static void phaseTwoGlobalResolution(UnionFind& unionFind);
  void phaseThreeRelabeling(Int32AbstractDataStore& featureIdsStore, Int32Array* cellPhasesPtr, int32 startLabel, int32 nextLabel, UnionFind& unionFind, size_t maxPhase) const;
  Result<> phaseFourIterativeFill(Int32AbstractDataStore& featureIdsStore, const std::array<int64_t, 3>& dims, size_t numFeatures) const;

  DataStructure& m_DataStructure;
  const FillBadDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
