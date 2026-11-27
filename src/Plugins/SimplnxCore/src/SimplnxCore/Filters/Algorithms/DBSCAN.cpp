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

Result<> DBSCAN::operator()()
{
  // Include every scanline input and output in storage-path selection.
  auto* clusteringArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->clustering_array_path);
  auto* featureIdsArrayPtr = m_DataStructure.getDataAs<Int32ArrayType>(m_InputValues->feature_ids_array_path);
  std::vector<const IArray*> targets = {clusteringArrayPtr, featureIdsArrayPtr};
  if(m_InputValues->use_mask)
  {
    targets.push_back(m_DataStructure.getDataAs<IDataArray>(m_InputValues->mask_array_path));
  }
  return DispatchAlgorithm<DBSCANDirect, DBSCANScanline>(AlgorithmArrayTargets(std::move(targets)), m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
