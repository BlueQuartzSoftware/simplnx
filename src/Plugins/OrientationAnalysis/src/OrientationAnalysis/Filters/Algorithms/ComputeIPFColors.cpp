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

Result<> ComputeIPFColors::operator()()
{
  // Dispatch checks storage residency. The selected executor performs typed access.
  auto* eulersArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->cell_euler_angles_array_path);
  auto* phasesArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->cell_phases_array_path);
  auto* crystalStructuresArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->crystal_structures_array_path);
  auto* ipfColorsArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->cell_ipf_colors_array_path);
  IDataArray* maskArrayPtr = nullptr;
  if(m_InputValues->use_mask)
  {
    maskArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->mask_array_path);
  }

  return DispatchAlgorithm<ComputeIPFColorsDirect, ComputeIPFColorsScanline>({eulersArrayPtr, phasesArrayPtr, crystalStructuresArrayPtr, maskArrayPtr, ipfColorsArrayPtr}, m_DataStructure,
                                                                             m_MessageHandler, m_ShouldCancel, m_InputValues);
}
