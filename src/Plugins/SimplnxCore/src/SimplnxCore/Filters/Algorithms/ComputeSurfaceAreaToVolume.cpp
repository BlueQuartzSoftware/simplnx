#include "ComputeSurfaceAreaToVolume.hpp"

#include "ComputeSurfaceAreaToVolumeDirect.hpp"
#include "ComputeSurfaceAreaToVolumeScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// ----------------------------------------------------------------------------
// ComputeSurfaceAreaToVolume -- Dispatcher
//
// This file implements the thin dispatch layer for the ComputeSurfaceAreaToVolume
// algorithm. No algorithm logic lives here; the sole responsibility is to
// inspect the storage type of the FeatureIds array and forward execution to
// either ComputeSurfaceAreaToVolumeDirect (in-core) or
// ComputeSurfaceAreaToVolumeScanline (out-of-core), via the DispatchAlgorithm
// template.
//
// The FeatureIds array is the critical input for dispatch because it is a
// cell-level array (one entry per voxel) that is accessed with 6-neighbor
// lookups. The feature-level arrays (NumCells, SurfaceAreaVolumeRatio,
// Sphericity) are small and do not drive the dispatch decision. The Scanline
// variant still caches these locally for efficiency, but the dispatch is based
// solely on FeatureIds.
// ----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
ComputeSurfaceAreaToVolume::ComputeSurfaceAreaToVolume(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       ComputeSurfaceAreaToVolumeInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeSurfaceAreaToVolume::~ComputeSurfaceAreaToVolume() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeSurfaceAreaToVolume::getCancel()
{
  return m_ShouldCancel;
}

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
Result<> ComputeSurfaceAreaToVolume::operator()()
{
  auto* featureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  return DispatchAlgorithm<ComputeSurfaceAreaToVolumeDirect, ComputeSurfaceAreaToVolumeScanline>({featureIdsArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
