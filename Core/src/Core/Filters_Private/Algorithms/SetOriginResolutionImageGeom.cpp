#include "SetOriginResolutionImageGeom.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
SetOriginResolutionImageGeom::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                          SetOriginResolutionImageGeomInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SetOriginResolutionImageGeom::~SetOriginResolutionImageGeom() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& SetOriginResolutionImageGeom::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> SetOriginResolutionImageGeom::operator()()
{

  return {};
}
