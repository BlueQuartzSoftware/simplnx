#include "ReadBinaryCTNorthStar.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ReadBinaryCTNorthStar::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadBinaryCTNorthStarInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReadBinaryCTNorthStar::~ReadBinaryCTNorthStar() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ReadBinaryCTNorthStar::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ReadBinaryCTNorthStar::operator()()
{

  return {};
}
