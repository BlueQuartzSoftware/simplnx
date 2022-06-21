#include "MatchCrystallography.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
MatchCrystallography::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MatchCrystallographyInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
MatchCrystallography::~MatchCrystallography() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& MatchCrystallography::getCancel()
{
  return m_ShouldCancel;
}


// -----------------------------------------------------------------------------
Result<> MatchCrystallography::operator()()
{


  return {};
}
