#include "BadDataNeighborOrientationCheck.hpp"

#include "BadDataNeighborOrientationCheckScanline.hpp"
#include "BadDataNeighborOrientationCheckWorklist.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
BadDataNeighborOrientationCheck::BadDataNeighborOrientationCheck(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                 BadDataNeighborOrientationCheckInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
BadDataNeighborOrientationCheck::~BadDataNeighborOrientationCheck() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& BadDataNeighborOrientationCheck::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
/**
 * @brief Dispatches the neighbor orientation check to the appropriate algorithm
 *        based on the storage type of the quaternion, mask, and phase arrays.
 *
 * The in-core path (Worklist) is preferred when all data is in RAM because its
 * worklist-driven propagation has O(flipped) amortized cost. The OOC path
 * (Scanline) is used when any array is chunked on disk, because the Worklist
 * variant's random-access deque pattern would cause catastrophic chunk thrashing.
 *
 * Note: the in-core algorithm is BadDataNeighborOrientationCheckWorklist (not "Direct"),
 * because the original linear-scan approach was replaced with a more efficient
 * worklist-based propagation algorithm.
 */
Result<> BadDataNeighborOrientationCheck::operator()()
{
  // Retrieve raw IDataArray pointers for storage-type inspection by DispatchAlgorithm.
  // These are only used for the AnyOutOfCore() check -- the actual typed access
  // happens inside the selected algorithm class.
  auto* quatsArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->QuatsArrayPath);
  auto* maskArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath);
  auto* phasesArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->CellPhasesArrayPath);

  return DispatchAlgorithm<BadDataNeighborOrientationCheckWorklist, BadDataNeighborOrientationCheckScanline>({quatsArray, maskArray, phasesArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel,
                                                                                                             m_InputValues);
}
