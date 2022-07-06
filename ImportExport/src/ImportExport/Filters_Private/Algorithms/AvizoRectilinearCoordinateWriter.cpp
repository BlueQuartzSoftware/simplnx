#include "AvizoRectilinearCoordinateWriter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
AvizoRectilinearCoordinateWriter::AvizoRectilinearCoordinateWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                   AvizoRectilinearCoordinateWriterInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AvizoRectilinearCoordinateWriter::~AvizoRectilinearCoordinateWriter() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AvizoRectilinearCoordinateWriter::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AvizoRectilinearCoordinateWriter::operator()()
{

  return {};
}
