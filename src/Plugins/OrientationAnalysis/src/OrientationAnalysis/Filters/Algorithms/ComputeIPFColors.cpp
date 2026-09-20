#include "ComputeIPFColors.hpp"

#include "ComputeIPFColorsDirect.hpp"
#include "ComputeIPFColorsScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

ComputeIPFColors::ComputeIPFColors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeIPFColorsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(mesgHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

ComputeIPFColors::~ComputeIPFColors() noexcept = default;

Result<> ComputeIPFColors::operator()()
{
  // Dispatch checks storage residency. The selected executor performs typed access.
  auto* eulersArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->cellEulerAnglesArrayPath);
  auto* phasesArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->cellPhasesArrayPath);
  auto* crystalStructuresArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->crystalStructuresArrayPath);
  auto* ipfColorsArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->cellIpfColorsArrayPath);
  IDataArray* maskArray = nullptr;
  if(m_InputValues->useMask)
  {
    maskArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->maskArrayPath);
  }

  return DispatchAlgorithm<ComputeIPFColorsDirect, ComputeIPFColorsScanline>({eulersArray, phasesArray, crystalStructuresArray, maskArray, ipfColorsArray}, m_DataStructure, m_MessageHandler,
                                                                             m_ShouldCancel, m_InputValues);
}
