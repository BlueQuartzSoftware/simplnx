#include "DBSCAN.hpp"

#include "DBSCANDirect.hpp"
#include "DBSCANScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// =============================================================================
// DBSCAN — Dispatcher
//
// This file contains only the dispatch logic. The actual algorithm implementations
// live in DBSCANDirect.cpp (in-core) and DBSCANScanline.cpp (out-of-core).
//
// The dispatch checks both the ClusteringArray and FeatureIds array storage types:
// if either uses chunked on-disk storage (OOC), the Scanline variant is selected
// to avoid chunk thrashing during the multi-pass grid construction and distance
// computation phases.
// =============================================================================

// -----------------------------------------------------------------------------
DBSCAN::DBSCAN(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DBSCANInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
DBSCAN::~DBSCAN() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Dispatches to the appropriate algorithm variant based on storage type.
 *
 * Uses DispatchAlgorithm<Direct, Scanline>() to check whether the ClusteringArray
 * or FeatureIds array is backed by out-of-core (chunked) storage. If so, the
 * Scanline variant is used; otherwise, the Direct variant is selected.
 *
 * Both variants receive identical constructor arguments and produce identical output.
 */
Result<> DBSCAN::operator()()
{
  // Check both arrays — the clustering array is read multiple times during grid
  // construction (bounds, binning, filling), and featureIds is written during labeling.
  auto* clusteringArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  auto* featureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  return DispatchAlgorithm<DBSCANDirect, DBSCANScanline>({clusteringArray, featureIdsArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
