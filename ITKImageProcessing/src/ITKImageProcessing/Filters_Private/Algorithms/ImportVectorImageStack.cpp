#include "ImportVectorImageStack.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ImportVectorImageStack::ImportVectorImageStack(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               ImportVectorImageStackInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ImportVectorImageStack::~ImportVectorImageStack() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ImportVectorImageStack::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ImportVectorImageStack::operator()()
{

  return {};
}
