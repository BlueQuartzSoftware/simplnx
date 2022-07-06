#include "DiscretizeDDDomain.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
DiscretizeDDDomain::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DiscretizeDDDomainInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
DiscretizeDDDomain::~DiscretizeDDDomain() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& DiscretizeDDDomain::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> DiscretizeDDDomain::operator()()
{

  return {};
}
