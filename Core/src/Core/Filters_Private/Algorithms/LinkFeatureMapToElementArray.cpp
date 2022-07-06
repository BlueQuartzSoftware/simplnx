#include "LinkFeatureMapToElementArray.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
LinkFeatureMapToElementArray::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                          LinkFeatureMapToElementArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
LinkFeatureMapToElementArray::~LinkFeatureMapToElementArray() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& LinkFeatureMapToElementArray::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> LinkFeatureMapToElementArray::operator()()
{

  return {};
}
