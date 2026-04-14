#include "ComputeFeatureNeighbors.hpp"

#include "ComputeFeatureNeighborsDirect.hpp"
#include "ComputeFeatureNeighborsScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// =============================================================================
// ComputeFeatureNeighbors — Dispatcher
//
// This file contains only the dispatch logic. The actual algorithm implementations
// live in ComputeFeatureNeighborsDirect.cpp (in-core) and
// ComputeFeatureNeighborsScanline.cpp (out-of-core).
//
// The dispatch checks the FeatureIds array's storage type: if it uses chunked
// on-disk storage (OOC), the Scanline variant is selected to avoid chunk thrashing.
// Otherwise, the Direct variant is used for optimal in-memory performance.
// =============================================================================

// -----------------------------------------------------------------------------
ComputeFeatureNeighbors::ComputeFeatureNeighbors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ComputeFeatureNeighborsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureNeighbors::~ComputeFeatureNeighbors() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Dispatches to the appropriate algorithm variant based on storage type.
 *
 * Uses DispatchAlgorithm<Direct, Scanline>() to check whether the FeatureIds array
 * is backed by out-of-core (chunked) storage. If so, the Scanline variant is used;
 * otherwise, the Direct variant is selected.
 *
 * Both variants receive identical constructor arguments and produce identical output.
 */
Result<> ComputeFeatureNeighbors::operator()()
{
  // Check the FeatureIds array — this is the primary input array that both
  // variants iterate over, so its storage type determines which path to take.
  auto* featureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath);
  return DispatchAlgorithm<ComputeFeatureNeighborsDirect, ComputeFeatureNeighborsScanline>({featureIdsArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
