#include "ComputeIPFColors.hpp"

#include "ComputeIPFColorsDirect.hpp"
#include "ComputeIPFColorsScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeIPFColors::ComputeIPFColors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeIPFColorsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(mesgHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
ComputeIPFColors::~ComputeIPFColors() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Dispatches IPF color computation to the appropriate algorithm based on
 *        storage type.
 *
 * The three arrays checked for OOC status are the Euler angles (input, 3-component
 * float32), the cell phases (input, 1-component int32), and the IPF colors (output,
 * 3-component uint8). If any of them are backed by chunked OOC storage, the Scanline
 * path is selected to avoid chunk thrashing; otherwise the parallel Direct path is used.
 */
Result<> ComputeIPFColors::operator()()
{
  // Retrieve raw IDataArray pointers for storage-type inspection by DispatchAlgorithm.
  // These are only used for the AnyOutOfCore() check -- the actual typed access
  // happens inside ComputeIPFColorsDirect or ComputeIPFColorsScanline.
  auto* eulersArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->cellEulerAnglesArrayPath);
  auto* phasesArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->cellPhasesArrayPath);
  auto* ipfColorsArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->cellIpfColorsArrayPath);

  return DispatchAlgorithm<ComputeIPFColorsDirect, ComputeIPFColorsScanline>({eulersArray, phasesArray, ipfColorsArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
