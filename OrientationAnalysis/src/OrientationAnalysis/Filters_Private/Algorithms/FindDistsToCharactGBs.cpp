#include "FindDistsToCharactGBs.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindDistsToCharactGBs::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindDistsToCharactGBsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindDistsToCharactGBs::~FindDistsToCharactGBs() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindDistsToCharactGBs::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindDistsToCharactGBs::operator()()
{

  return {};
}
