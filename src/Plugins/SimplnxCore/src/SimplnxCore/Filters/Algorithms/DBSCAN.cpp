#include "DBSCAN.hpp"

#include "DBSCANDirect.hpp"
#include "DBSCANScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
DBSCAN::DBSCAN(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DBSCANInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
DBSCAN::~DBSCAN() noexcept = default;

Result<> DBSCAN::operator()()
{
  // Include every scanline input and output in storage-path selection.
  auto* clusteringArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  auto* featureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  std::vector<const IArray*> targets = {clusteringArray, featureIdsArray};
  if(m_InputValues->UseMask)
  {
    targets.push_back(m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath));
  }
  return DispatchAlgorithm<DBSCANDirect, DBSCANScanline>(AlgorithmArrayTargets(std::move(targets)), m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
