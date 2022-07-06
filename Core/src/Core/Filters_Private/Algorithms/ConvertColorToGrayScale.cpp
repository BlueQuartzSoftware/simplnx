#include "ConvertColorToGrayScale.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ConvertColorToGrayScale::ConvertColorToGrayScale(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ConvertColorToGrayScaleInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ConvertColorToGrayScale::~ConvertColorToGrayScale() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ConvertColorToGrayScale::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ConvertColorToGrayScale::operator()()
{

  return {};
}
