#include "Export3dSolidMesh.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
Export3dSolidMesh::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, Export3dSolidMeshInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
Export3dSolidMesh::~Export3dSolidMesh() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& Export3dSolidMesh::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> Export3dSolidMesh::operator()()
{

  return {};
}
