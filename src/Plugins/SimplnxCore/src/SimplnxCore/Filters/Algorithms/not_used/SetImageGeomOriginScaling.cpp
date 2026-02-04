#include "SetImageGeomOriginScaling.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
SetImageGeomOriginScaling::SetImageGeomOriginScaling(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     SetImageGeomOriginScalingInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SetImageGeomOriginScaling::~SetImageGeomOriginScaling() noexcept = default;

// -----------------------------------------------------------------------------
Result<> SetImageGeomOriginScaling::operator()()
{

  return {};
}
