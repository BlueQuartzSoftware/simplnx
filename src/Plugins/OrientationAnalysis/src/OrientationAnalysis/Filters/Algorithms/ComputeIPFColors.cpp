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
Result<> ComputeIPFColors::operator()()
{
  auto* eulersArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->cellEulerAnglesArrayPath);
  auto* phasesArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->cellPhasesArrayPath);
  auto* ipfColorsArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->cellIpfColorsArrayPath);

  return DispatchAlgorithm<ComputeIPFColorsDirect, ComputeIPFColorsScanline>({eulersArray, phasesArray, ipfColorsArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
