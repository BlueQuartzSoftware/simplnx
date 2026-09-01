#include "Silhouette.hpp"

#include "SilhouetteDirect.hpp"
#include "SilhouetteScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

Silhouette::Silhouette(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SilhouetteInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

Silhouette::~Silhouette() noexcept = default;

void Silhouette::updateProgress(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

const std::atomic_bool& Silhouette::getCancel()
{
  return m_ShouldCancel;
}

Result<> Silhouette::operator()()
{
  const auto& clusteringArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  const auto& featureIdsArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->FeatureIdsArrayPath);
  const auto& outputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SilhouetteArrayPath);
  // Every participating store drives dispatch because any one can require bulk I/O.
  if(m_InputValues->UseMask)
  {
    const auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
    return DispatchAlgorithm<SilhouetteDirect, SilhouetteScanline>({&clusteringArray, &featureIdsArray, &maskArray, &outputArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
  }
  return DispatchAlgorithm<SilhouetteDirect, SilhouetteScanline>({&clusteringArray, &featureIdsArray, &outputArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
