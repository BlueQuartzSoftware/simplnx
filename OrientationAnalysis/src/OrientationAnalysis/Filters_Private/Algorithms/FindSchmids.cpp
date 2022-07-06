#include "FindSchmids.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindSchmids::FindSchmids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindSchmidsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindSchmids::~FindSchmids() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindSchmids::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindSchmids::operator()()
{

  return {};
}
