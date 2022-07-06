#include "LosAlamosFFTWriter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
LosAlamosFFTWriter::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, LosAlamosFFTWriterInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
LosAlamosFFTWriter::~LosAlamosFFTWriter() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& LosAlamosFFTWriter::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> LosAlamosFFTWriter::operator()()
{

  return {};
}
