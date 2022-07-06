#include "ItkConvertArrayTo8BitImage.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ItkConvertArrayTo8BitImage::ItkConvertArrayTo8BitImage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       ItkConvertArrayTo8BitImageInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ItkConvertArrayTo8BitImage::~ItkConvertArrayTo8BitImage() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ItkConvertArrayTo8BitImage::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ItkConvertArrayTo8BitImage::operator()()
{

  return {};
}
