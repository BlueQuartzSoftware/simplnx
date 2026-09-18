#include "CopyFeatureArrayToElementArray.hpp"

#include "CopyFeatureArrayToElementArrayDirect.hpp"
#include "CopyFeatureArrayToElementArrayScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

CopyFeatureArrayToElementArray::CopyFeatureArrayToElementArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                               const CopyFeatureArrayToElementArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

CopyFeatureArrayToElementArray::~CopyFeatureArrayToElementArray() noexcept = default;

Result<> CopyFeatureArrayToElementArray::operator()()
{
  if(m_InputValues->SelectedFeatureArrayPaths.empty())
  {
    return {};
  }

  // Every source and created array participates because mixed storage is valid.
  std::vector<const IArray*> targets;
  targets.reserve(1 + (2 * m_InputValues->SelectedFeatureArrayPaths.size()));
  targets.push_back(m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath));
  for(const auto& selectedFeatureArrayPath : m_InputValues->SelectedFeatureArrayPaths)
  {
    targets.push_back(m_DataStructure.getDataAs<IDataArray>(selectedFeatureArrayPath));
    const DataPath createdArrayPath = m_InputValues->FeatureIdsPath.replaceName(selectedFeatureArrayPath.getTargetName() + m_InputValues->CreatedArraySuffix);
    targets.push_back(m_DataStructure.getDataAs<IDataArray>(createdArrayPath));
  }
  return DispatchAlgorithm<CopyFeatureArrayToElementArrayDirect, CopyFeatureArrayToElementArrayScanline>(AlgorithmArrayTargets{std::move(targets)}, m_DataStructure, m_MessageHandler, m_ShouldCancel,
                                                                                                         m_InputValues);
}
