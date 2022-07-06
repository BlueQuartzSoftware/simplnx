#include "CleaveTetVolumeMesh.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
CleaveTetVolumeMesh::CleaveTetVolumeMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CleaveTetVolumeMeshInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CleaveTetVolumeMesh::~CleaveTetVolumeMesh() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CleaveTetVolumeMesh::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CleaveTetVolumeMesh::operator()()
{

  return {};
}
