#include "WaveFrontObjectFileWriter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
WaveFrontObjectFileWriter::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                       WaveFrontObjectFileWriterInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WaveFrontObjectFileWriter::~WaveFrontObjectFileWriter() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& WaveFrontObjectFileWriter::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> WaveFrontObjectFileWriter::operator()()
{

  return {};
}
