/**
 * @file SurfaceNets.cpp
 * @brief Dispatcher implementation for the SurfaceNets algorithm.
 *
 * This file contains the thin dispatch layer that examines the backing
 * storage of the FeatureIds array and forwards execution to either:
 *   - SurfaceNetsDirect   -- when all arrays are in-memory (uses MMSurfaceNet library)
 *   - SurfaceNetsScanline -- when any array uses chunked OOC storage
 *
 * The dispatch decision is made by DispatchAlgorithm, which inspects whether
 * the DataStore is a chunked format. This avoids the severe performance
 * penalty of random element access through virtual operator[] on OOC stores.
 */

#include "SurfaceNets.hpp"
#include "SurfaceNetsDirect.hpp"
#include "SurfaceNetsScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
SurfaceNets::SurfaceNets(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SurfaceNetsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SurfaceNets::~SurfaceNets() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& SurfaceNets::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
/**
 * @brief Dispatches to the correct algorithm variant based on DataStore type.
 *
 * The FeatureIds array is the primary input whose access pattern determines
 * whether OOC optimization is needed. The Direct variant passes the raw
 * DataStore to MMSurfaceNet which accesses it via operator[]. The Scanline
 * variant reads FeatureIds via copyIntoBuffer() in Z-slices and builds its
 * own O(surface) cell classification data structures.
 */
Result<> SurfaceNets::operator()()
{
  auto* featureIds = m_DataStructure.getDataAs<IDataArray>(m_InputValues->FeatureIdsArrayPath);
  return DispatchAlgorithm<SurfaceNetsDirect, SurfaceNetsScanline>({featureIds}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
