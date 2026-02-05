#include "CreateImageGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
CreateImageGeometry::CreateImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateImageGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CreateImageGeometry::~CreateImageGeometry() noexcept = default;

// -----------------------------------------------------------------------------
Result<> CreateImageGeometry::operator()()
{

  return {};
}
