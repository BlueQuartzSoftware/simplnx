#include "CreateFeatureArrayFromElementArray.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
CreateFeatureArrayFromElementArray::CreateFeatureArrayFromElementArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                       CreateFeatureArrayFromElementArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CreateFeatureArrayFromElementArray::~CreateFeatureArrayFromElementArray() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CreateFeatureArrayFromElementArray::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CreateFeatureArrayFromElementArray::operator()()
{

  return {};
}
