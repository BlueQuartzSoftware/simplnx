#include "ComputeKMedoids.hpp"

#include "ComputeKMedoidsDirect.hpp"
#include "ComputeKMedoidsScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeKMedoids::ComputeKMedoids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, KMedoidsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeKMedoids::getCancel()
{
  return m_ShouldCancel;
}

Result<> ComputeKMedoids::operator()()
{
  // Include every scanline input and output in storage-path selection.
  auto* clusteringArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  auto* featureIdsArrayPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto* medoidsArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MedoidsArrayPath);
  if(m_InputValues->UseMask)
  {
    auto* maskArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath);
    return DispatchAlgorithm<ComputeKMedoidsDirect, ComputeKMedoidsScanline>({clusteringArrayPtr, featureIdsArrayPtr, medoidsArrayPtr, maskArrayPtr}, m_DataStructure, m_MessageHandler, m_ShouldCancel,
                                                                             m_InputValues);
  }
  return DispatchAlgorithm<ComputeKMedoidsDirect, ComputeKMedoidsScanline>({clusteringArrayPtr, featureIdsArrayPtr, medoidsArrayPtr}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
