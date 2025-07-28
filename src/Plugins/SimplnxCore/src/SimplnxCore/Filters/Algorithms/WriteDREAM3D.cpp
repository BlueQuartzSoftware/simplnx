#include "WriteDREAM3D.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
WriteDREAM3D::WriteDREAM3D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteDREAM3DInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WriteDREAM3D::~WriteDREAM3D() noexcept = default;

// -----------------------------------------------------------------------------
Result<> WriteDREAM3D::operator()()
{

  return {};
}
