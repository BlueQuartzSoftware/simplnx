#include "GBCDTriangleDumper.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
GBCDTriangleDumper::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GBCDTriangleDumperInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
GBCDTriangleDumper::~GBCDTriangleDumper() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GBCDTriangleDumper::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> GBCDTriangleDumper::operator()()
{

  return {};
}
