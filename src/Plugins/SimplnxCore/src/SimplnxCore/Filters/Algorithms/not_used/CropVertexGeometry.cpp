#include "CropVertexGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
CropVertexGeometry::CropVertexGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CropVertexGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CropVertexGeometry::~CropVertexGeometry() noexcept = default;

// -----------------------------------------------------------------------------
Result<> CropVertexGeometry::operator()()
{

  return {};
}
