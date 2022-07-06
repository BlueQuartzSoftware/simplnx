#include "ResampleRectGridToImageGeom.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ResampleRectGridToImageGeom::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                         ResampleRectGridToImageGeomInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ResampleRectGridToImageGeom::~ResampleRectGridToImageGeom() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ResampleRectGridToImageGeom::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ResampleRectGridToImageGeom::operator()()
{

  return {};
}
