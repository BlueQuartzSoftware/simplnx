#include "ComputeFeatureSizes.hpp"

#include "ComputeFeatureSizesDirect.hpp"
#include "ComputeFeatureSizesScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

ComputeFeatureSizes::ComputeFeatureSizes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureSizesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFeatureSizes::~ComputeFeatureSizes() noexcept = default;

Result<> ComputeFeatureSizes::operator()()
{
  auto* featureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath);
  return DispatchAlgorithm<ComputeFeatureSizesDirect, ComputeFeatureSizesScanline>({featureIdsArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
