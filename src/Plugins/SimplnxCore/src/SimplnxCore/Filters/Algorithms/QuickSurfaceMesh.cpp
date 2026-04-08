#include "QuickSurfaceMesh.hpp"
#include "QuickSurfaceMeshDirect.hpp"
#include "QuickSurfaceMeshScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
QuickSurfaceMesh::QuickSurfaceMesh(DataStructure& dataStructure, QuickSurfaceMeshInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
QuickSurfaceMesh::~QuickSurfaceMesh() noexcept = default;

// -----------------------------------------------------------------------------
Result<> QuickSurfaceMesh::operator()()
{
  auto* featureIds = m_DataStructure.getDataAs<IDataArray>(m_InputValues->FeatureIdsArrayPath);
  return DispatchAlgorithm<QuickSurfaceMeshDirect, QuickSurfaceMeshScanline>({featureIds}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
