#include "ComputeBoundaryCells.hpp"

#include "ComputeBoundaryCellsDirect.hpp"
#include "ComputeBoundaryCellsScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// ----------------------------------------------------------------------------
// ComputeBoundaryCells -- Dispatcher
//
// This file implements the thin dispatch layer for the ComputeBoundaryCells
// algorithm. No algorithm logic lives here; the sole responsibility is to
// inspect the storage type of the FeatureIds array and forward execution to
// either ComputeBoundaryCellsDirect (in-core) or ComputeBoundaryCellsScanline
// (out-of-core), via the DispatchAlgorithm template.
//
// The FeatureIds array is the critical input because it is a cell-level array
// with one entry per voxel. For a 500x500x500 volume that is ~125 million
// int32 values. When stored out-of-core in chunked format, random-access reads
// through operator[] trigger chunk load/evict cycles that make the algorithm
// catastrophically slow. The Scanline variant avoids this by using sequential
// bulk I/O (copyIntoBuffer/copyFromBuffer) one Z-slice at a time.
// ----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
ComputeBoundaryCells::ComputeBoundaryCells(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeBoundaryCellsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeBoundaryCells::~ComputeBoundaryCells() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeBoundaryCells::getCancel()
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
Result<> ComputeBoundaryCells::operator()()
{
  auto* featureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  return DispatchAlgorithm<ComputeBoundaryCellsDirect, ComputeBoundaryCellsScanline>({featureIdsArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
