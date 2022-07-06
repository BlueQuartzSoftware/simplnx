#include "FFTHDFWriterFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FFTHDFWriterFilter::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FFTHDFWriterFilterInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FFTHDFWriterFilter::~FFTHDFWriterFilter() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FFTHDFWriterFilter::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FFTHDFWriterFilter::operator()()
{

  return {};
}
