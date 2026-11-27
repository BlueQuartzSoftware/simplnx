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

Result<> ComputeFeatureSizes::operator()()
{
  auto* featureIdsArrayPtr = m_DataStructure.getDataAs<Int32ArrayType>(m_InputValues->feature_ids_path);
  return DispatchAlgorithm<ComputeFeatureSizesDirect, ComputeFeatureSizesScanline>({featureIdsArrayPtr}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
