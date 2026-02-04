#include "CopyFeatureArrayToElementArray.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
CopyFeatureArrayToElementArray::CopyFeatureArrayToElementArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                               CopyFeatureArrayToElementArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CopyFeatureArrayToElementArray::~CopyFeatureArrayToElementArray() noexcept = default;

// -----------------------------------------------------------------------------
Result<> CopyFeatureArrayToElementArray::operator()()
{

  return {};
}
