#include "MultiThresholdObjects.hpp"

#include "MultiThresholdObjectsDirect.hpp"
#include "MultiThresholdObjectsScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// =============================================================================
// MultiThresholdObjects — Dispatcher
//
// This file contains only the dispatch logic. The actual algorithm implementations
// live in MultiThresholdObjectsDirect.cpp (in-core) and
// MultiThresholdObjectsScanline.cpp (out-of-core).
//
// The dispatch checks the first required input array's storage type: if it uses
// chunked on-disk storage (OOC), the Scanline variant is selected. Since all
// input arrays in a threshold set come from the same Attribute Matrix, if one is
// OOC then all are OOC, so checking the first is sufficient.
// =============================================================================

// -----------------------------------------------------------------------------
MultiThresholdObjects::MultiThresholdObjects(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             MultiThresholdObjectsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
MultiThresholdObjects::~MultiThresholdObjects() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Dispatches to the appropriate algorithm variant based on storage type.
 *
 * Checks the first input array referenced by the threshold configuration to determine
 * if OOC storage is in use. All threshold input arrays come from the same Attribute
 * Matrix, so if one is OOC, all are OOC.
 *
 * Both variants receive identical constructor arguments and produce identical output.
 */
Result<> MultiThresholdObjects::operator()()
{
  auto thresholdsObject = m_InputValues->ArrayThresholdsObject;
  const auto& requiredPaths = thresholdsObject.getRequiredPaths();
  // Check the first input array — since all arrays in a threshold set share the
  // same Attribute Matrix, they all use the same storage type.
  const IDataArray* checkArray = nullptr;
  if(!requiredPaths.empty())
  {
    checkArray = m_DataStructure.getDataAs<IDataArray>(*requiredPaths.begin());
  }
  return DispatchAlgorithm<MultiThresholdObjectsDirect, MultiThresholdObjectsScanline>({checkArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
