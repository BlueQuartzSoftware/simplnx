#include "ComputeSurfaceFeatures.hpp"

#include "ComputeSurfaceFeaturesDirect.hpp"
#include "ComputeSurfaceFeaturesScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// ----------------------------------------------------------------------------
// ComputeSurfaceFeatures -- Dispatcher
//
// This file implements the thin dispatch layer for the ComputeSurfaceFeatures
// algorithm. No algorithm logic lives here; the sole responsibility is to
// inspect the storage type of the FeatureIds array and forward execution to
// either ComputeSurfaceFeaturesDirect (in-core) or ComputeSurfaceFeaturesScanline
// (out-of-core), via the DispatchAlgorithm template.
//
// The FeatureIds array is the critical input for dispatch because it is a
// cell-level array (one entry per voxel), which can be very large when stored
// out-of-core. The SurfaceFeatures output is a small feature-level array and
// does not drive the dispatch decision.
// ----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
ComputeSurfaceFeatures::ComputeSurfaceFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               ComputeSurfaceFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeSurfaceFeatures::~ComputeSurfaceFeatures() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Inspects the FeatureIds array's storage type and dispatches to the
 * appropriate algorithm variant.
 *
 * The dispatch decision is made by DispatchAlgorithm, which checks:
 *   1. ForceInCoreAlgorithm() -- test override, always selects Direct
 *   2. AnyOutOfCore({featureIdsArray}) -- runtime detection of chunked storage
 *   3. ForceOocAlgorithm() -- test override, forces Scanline
 *   4. Default -- selects Direct (in-core)
 */
Result<> ComputeSurfaceFeatures::operator()()
{
  auto* featureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath);
  return DispatchAlgorithm<ComputeSurfaceFeaturesDirect, ComputeSurfaceFeaturesScanline>({featureIdsArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
