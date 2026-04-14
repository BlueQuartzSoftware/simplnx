// -----------------------------------------------------------------------------
// IdentifySample.cpp -- Algorithm dispatcher for the IdentifySample filter
// -----------------------------------------------------------------------------
//
// This file contains only the dispatch logic. It inspects the storage type of
// the mask array and delegates to one of two algorithm implementations:
//
//   - IdentifySampleBFS:  BFS flood-fill, optimal for in-core (contiguous memory)
//   - IdentifySampleCCL:  Scanline CCL with Union-Find, optimal for out-of-core
//                          (chunked HDF5 storage on disk)
//
// When slice-by-slice mode is enabled, both variants delegate to the same
// shared IdentifySampleSliceBySliceFunctor (in IdentifySampleCommon.hpp),
// which uses BFS on individual 2D slices. Slice-by-slice BFS is always safe
// because a single slice fits in memory regardless of OOC storage.
//
// Both algorithm variants produce identical results for the same inputs.
// -----------------------------------------------------------------------------

#include "IdentifySample.hpp"

#include "IdentifySampleBFS.hpp"
#include "IdentifySampleCCL.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
IdentifySample::IdentifySample(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, IdentifySampleInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
IdentifySample::~IdentifySample() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Dispatches to either BFS or CCL based on the mask array's storage type.
 *
 * The mask array is the sole input/output: it is read during component discovery
 * and written during the masking/fill phases. If it uses OOC storage, the CCL
 * algorithm is selected because BFS flood-fill would trigger random chunk accesses
 * that cause severe performance degradation (chunk thrashing). The CCL algorithm
 * processes data in strictly sequential Z-slice order, which aligns with the
 * chunked storage layout and avoids thrashing.
 */
Result<> IdentifySample::operator()()
{
  // Check the mask array to determine storage type (in-core vs OOC).
  // This single array drives the dispatch decision because it is the only
  // data array accessed during the algorithm (unlike FillBadData which also
  // accesses FeatureIds and multiple cell arrays).
  auto* maskArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath);

  return DispatchAlgorithm<IdentifySampleBFS, IdentifySampleCCL>({maskArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
