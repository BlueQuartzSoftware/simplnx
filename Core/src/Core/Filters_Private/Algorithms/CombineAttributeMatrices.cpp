#include "CombineAttributeMatrices.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
CombineAttributeMatrices::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CombineAttributeMatricesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CombineAttributeMatrices::~CombineAttributeMatrices() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CombineAttributeMatrices::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CombineAttributeMatrices::operator()()
{

  return {};
}
