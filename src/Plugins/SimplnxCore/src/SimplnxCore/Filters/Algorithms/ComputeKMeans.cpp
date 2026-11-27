#include "ComputeKMeans.hpp"

#include "ComputeKMeansDirect.hpp"
#include "ComputeKMeansScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeKMeans::ComputeKMeans(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeKMeansInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeKMeans::getCancel()
{
  return m_ShouldCancel;
}

Result<> ComputeKMeans::operator()()
{
  // Include every Scanline input/output; an OOC mask or means array must not
  // accidentally route execution to Direct's element-wise implementation.
  auto* clusteringArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->clustering_array_path);
  auto* featureIdsArrayPtr = m_DataStructure.getDataAs<Int32ArrayType>(m_InputValues->feature_ids_array_path);
  auto* meansArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->means_array_path);
  std::vector<const IArray*> targets = {clusteringArrayPtr, featureIdsArrayPtr, meansArrayPtr};
  if(m_InputValues->use_mask)
  {
    targets.push_back(m_DataStructure.getDataAs<IDataArray>(m_InputValues->mask_array_path));
  }
  return DispatchAlgorithm<ComputeKMeansDirect, ComputeKMeansScanline>(AlgorithmArrayTargets(std::move(targets)), m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
