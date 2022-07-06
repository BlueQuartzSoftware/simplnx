#include "EbsdToH5Ebsd.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
EbsdToH5Ebsd::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EbsdToH5EbsdInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
EbsdToH5Ebsd::~EbsdToH5Ebsd() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& EbsdToH5Ebsd::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> EbsdToH5Ebsd::operator()()
{

  return {};
}
