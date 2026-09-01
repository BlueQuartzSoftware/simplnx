#include "ComputeBoundaryCells.hpp"

#include "ComputeBoundaryCellsDirect.hpp"
#include "ComputeBoundaryCellsScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

ComputeBoundaryCells::ComputeBoundaryCells(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeBoundaryCellsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeBoundaryCells::~ComputeBoundaryCells() noexcept = default;

const std::atomic_bool& ComputeBoundaryCells::getCancel()
{
  return m_ShouldCancel;
}

Result<> ComputeBoundaryCells::operator()()
{
  auto* featureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  // Dispatch intentionally observes only FeatureIds. The output is not a target.
  return DispatchAlgorithm<ComputeBoundaryCellsDirect, ComputeBoundaryCellsScanline>({featureIdsArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
