#include "AbaqusHexahedronWriter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
AbaqusHexahedronWriter::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AbaqusHexahedronWriterInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AbaqusHexahedronWriter::~AbaqusHexahedronWriter() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AbaqusHexahedronWriter::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AbaqusHexahedronWriter::operator()()
{

  return {};
}
