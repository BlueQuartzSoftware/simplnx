#include "ApplyDewarpParameters.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ApplyDewarpParameters::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ApplyDewarpParametersInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ApplyDewarpParameters::~ApplyDewarpParameters() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ApplyDewarpParameters::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ApplyDewarpParameters::operator()()
{

  return {};
}
