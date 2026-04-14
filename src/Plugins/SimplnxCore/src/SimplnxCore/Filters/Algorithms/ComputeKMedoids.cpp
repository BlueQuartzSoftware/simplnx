#include "ComputeKMedoids.hpp"

#include "ComputeKMedoidsDirect.hpp"
#include "ComputeKMedoidsScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// =============================================================================
// ComputeKMedoids — Dispatcher
//
// This file contains only the dispatch logic. The actual algorithm implementations
// live in ComputeKMedoidsDirect.cpp (in-core) and ComputeKMedoidsScanline.cpp
// (out-of-core).
//
// The dispatch checks both the ClusteringArray and FeatureIds array storage types:
// if either uses chunked on-disk storage (OOC), the Scanline variant is selected
// to avoid chunk thrashing during the iterative distance computations.
// =============================================================================

// -----------------------------------------------------------------------------
ComputeKMedoids::ComputeKMedoids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, KMedoidsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeKMedoids::~ComputeKMedoids() noexcept = default;

// -----------------------------------------------------------------------------
void ComputeKMedoids::updateProgress(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeKMedoids::getCancel()
{
  return m_ShouldCancel;
}

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
Result<> ComputeKMedoids::operator()()
{
  // Check both arrays — the clustering array is read repeatedly during distance
  // computation, and featureIds is read/written during cluster assignment.
  auto* clusteringArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  auto* featureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  return DispatchAlgorithm<ComputeKMedoidsDirect, ComputeKMedoidsScanline>({clusteringArray, featureIdsArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
