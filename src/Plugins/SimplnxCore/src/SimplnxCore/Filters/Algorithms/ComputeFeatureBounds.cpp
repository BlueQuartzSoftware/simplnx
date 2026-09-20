#include "ComputeFeatureBounds.hpp"

#include "ComputeFeatureBoundsDirect.hpp"
#include "ComputeFeatureBoundsScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

ComputeFeatureBounds::ComputeFeatureBounds(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureBoundsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFeatureBounds::~ComputeFeatureBounds() noexcept = default;

Result<> ComputeFeatureBounds::operator()()
{
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  return DispatchAlgorithm<ComputeFeatureBoundsDirect, ComputeFeatureBoundsScanline>({&featureIds}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
