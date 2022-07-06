#include "IdentifySample.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
IdentifySample::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, IdentifySampleInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
IdentifySample::~IdentifySample() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& IdentifySample::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> IdentifySample::operator()()
{

  return {};
}
