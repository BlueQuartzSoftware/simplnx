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
ComputeKMeans::~ComputeKMeans() noexcept = default;

// -----------------------------------------------------------------------------
void ComputeKMeans::updateProgress(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeKMeans::getCancel()
{
  return m_ShouldCancel;
}

Result<> ComputeKMeans::operator()()
{
  // Include every Scanline input/output; an OOC mask or means array must not
  // accidentally route execution to Direct's element-wise implementation.
  auto* clusteringArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  auto* featureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto* meansArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MeansArrayPath);
  std::vector<const IArray*> targets = {clusteringArray, featureIdsArray, meansArray};
  if(m_InputValues->UseMask)
  {
    targets.push_back(m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath));
  }
  return DispatchAlgorithm<ComputeKMeansDirect, ComputeKMeansScanline>(AlgorithmArrayTargets(std::move(targets)), m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
