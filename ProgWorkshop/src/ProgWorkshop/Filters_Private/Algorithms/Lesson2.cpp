#include "Lesson2.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
Lesson2::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, Lesson2InputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
Lesson2::~Lesson2() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& Lesson2::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> Lesson2::operator()()
{

  return {};
}
