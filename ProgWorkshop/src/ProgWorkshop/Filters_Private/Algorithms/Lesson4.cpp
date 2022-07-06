#include "Lesson4.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
Lesson4::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, Lesson4InputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
Lesson4::~Lesson4() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& Lesson4::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> Lesson4::operator()()
{

  return {};
}
