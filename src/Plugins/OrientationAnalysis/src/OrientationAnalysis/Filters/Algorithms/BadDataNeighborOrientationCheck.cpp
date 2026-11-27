#include "BadDataNeighborOrientationCheck.hpp"

#include "BadDataNeighborOrientationCheckScanline.hpp"
#include "BadDataNeighborOrientationCheckWorklist.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

BadDataNeighborOrientationCheck::BadDataNeighborOrientationCheck(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                 BadDataNeighborOrientationCheckInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

const std::atomic_bool& BadDataNeighborOrientationCheck::getCancel()
{
  return m_ShouldCancel;
}

Result<> BadDataNeighborOrientationCheck::operator()()
{
  // Dispatch checks only storage residency. The selected executor performs
  // typed array access.
  auto* quatsArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->quats_array_path);
  auto* maskArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->mask_array_path);
  auto* phasesArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->cell_phases_array_path);

  return DispatchAlgorithm<BadDataNeighborOrientationCheckWorklist, BadDataNeighborOrientationCheckScanline>({quatsArrayPtr, maskArrayPtr, phasesArrayPtr}, m_DataStructure, m_MessageHandler,
                                                                                                             m_ShouldCancel, m_InputValues);
}
