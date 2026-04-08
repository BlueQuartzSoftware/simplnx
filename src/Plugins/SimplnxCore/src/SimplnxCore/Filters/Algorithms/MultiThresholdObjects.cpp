#include "MultiThresholdObjects.hpp"

#include "MultiThresholdObjectsDirect.hpp"
#include "MultiThresholdObjectsScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

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
Result<> MultiThresholdObjects::operator()()
{
  auto thresholdsObject = m_InputValues->ArrayThresholdsObject;
  const auto& requiredPaths = thresholdsObject.getRequiredPaths();
  const IDataArray* checkArray = nullptr;
  if(!requiredPaths.empty())
  {
    checkArray = m_DataStructure.getDataAs<IDataArray>(*requiredPaths.begin());
  }
  return DispatchAlgorithm<MultiThresholdObjectsDirect, MultiThresholdObjectsScanline>({checkArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
