// -----------------------------------------------------------------------------
// FillBadData.cpp -- Algorithm dispatcher for the FillBadData filter
// -----------------------------------------------------------------------------
//
// This file contains only the dispatch logic. It inspects the storage type of
// the FeatureIds array and delegates to one of two algorithm implementations:
//
//   - FillBadDataBFS:  BFS flood-fill, optimal for in-core (contiguous memory)
//   - FillBadDataCCL:  Scanline CCL with Union-Find, optimal for out-of-core
//                       (chunked HDF5 storage on disk)
//
// The dispatch is performed by DispatchAlgorithm<BFS, CCL>(), which checks
// the data store type of each array in the initializer list. If any array
// uses OOC storage (or if the test override ForceOocAlgorithm() is set),
// the CCL variant is selected. Otherwise, the BFS variant is used.
//
// Both algorithm variants produce identical results for the same inputs.
// The only difference is their data access pattern and memory strategy.
// -----------------------------------------------------------------------------

#include "FillBadData.hpp"

#include "FillBadDataBFS.hpp"
#include "FillBadDataCCL.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
FillBadData::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const FillBadDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FillBadData::~FillBadData() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Dispatches to either BFS or CCL based on the FeatureIds array's storage type.
 *
 * The FeatureIds array is the primary input: it is read during region discovery
 * and written during the fill phase. If it uses OOC storage, the CCL algorithm
 * is selected because BFS flood-fill would trigger random chunk accesses that
 * cause severe performance degradation (chunk thrashing). The CCL algorithm
 * processes data in strictly sequential Z-slice order, which aligns with the
 * chunked storage layout and avoids thrashing.
 */
Result<> FillBadData::operator()()
{
  // Check the FeatureIds array to determine storage type (in-core vs OOC).
  // This single array drives the dispatch decision because it is the most
  // heavily accessed array during both region discovery and fill phases.
  auto* featureIdsArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->featureIdsArrayPath);

  return DispatchAlgorithm<FillBadDataBFS, FillBadDataCCL>({featureIdsArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
