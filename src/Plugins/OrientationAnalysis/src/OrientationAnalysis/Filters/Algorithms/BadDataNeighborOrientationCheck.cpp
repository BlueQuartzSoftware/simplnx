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

BadDataNeighborOrientationCheck::~BadDataNeighborOrientationCheck() noexcept = default;

const std::atomic_bool& BadDataNeighborOrientationCheck::getCancel()
{
  return m_ShouldCancel;
}

Result<> BadDataNeighborOrientationCheck::operator()()
{
  // Dispatch checks only storage residency. The selected executor performs
  // typed array access.
  auto* quatsArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->QuatsArrayPath);
  auto* maskArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath);
  auto* phasesArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->CellPhasesArrayPath);

  return DispatchAlgorithm<BadDataNeighborOrientationCheckWorklist, BadDataNeighborOrientationCheckScanline>({quatsArray, maskArray, phasesArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel,
                                                                                                             m_InputValues);
}
