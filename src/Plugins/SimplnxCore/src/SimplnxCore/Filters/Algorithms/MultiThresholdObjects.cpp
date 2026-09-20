#include "MultiThresholdObjects.hpp"

#include "MultiThresholdObjectsDirect.hpp"
#include "MultiThresholdObjectsScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// The dispatch checks every threshold input and the output mask. Valid adaptive
// storage combinations may put any one of these arrays on disk.

MultiThresholdObjects::MultiThresholdObjects(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             MultiThresholdObjectsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

MultiThresholdObjects::~MultiThresholdObjects() noexcept = default;

Result<> MultiThresholdObjects::operator()()
{
  auto thresholdsObject = m_InputValues->ArrayThresholdsObject;
  const auto& requiredPaths = thresholdsObject.getRequiredPaths();
  std::vector<const IArray*> targets;
  targets.reserve(requiredPaths.size() + 1);
  for(const auto& path : requiredPaths)
  {
    targets.push_back(m_DataStructure.getDataAs<IDataArray>(path));
  }
  const DataPath maskArrayPath = (*requiredPaths.begin()).replaceName(m_InputValues->OutputDataArrayName);
  targets.push_back(m_DataStructure.getDataAs<IDataArray>(maskArrayPath));
  return DispatchAlgorithm<MultiThresholdObjectsDirect, MultiThresholdObjectsScanline>(AlgorithmArrayTargets(std::move(targets)), m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
