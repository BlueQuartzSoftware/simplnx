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

const std::atomic_bool& Silhouette::getCancel()
{
  return m_ShouldCancel;
}

Result<> Silhouette::operator()()
{
  const auto& clusteringArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->clustering_array_path);
  const auto& featureIdsArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->feature_ids_array_path);
  const auto& outputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->silhouette_array_path);
  // Every participating store drives dispatch because any one can require bulk I/O.
  if(m_InputValues->use_mask)
  {
    const auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->mask_array_path);
    return DispatchAlgorithm<SilhouetteDirect, SilhouetteScanline>({&clusteringArray, &featureIdsArray, &maskArray, &outputArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
  }
  return DispatchAlgorithm<SilhouetteDirect, SilhouetteScanline>({&clusteringArray, &featureIdsArray, &outputArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
