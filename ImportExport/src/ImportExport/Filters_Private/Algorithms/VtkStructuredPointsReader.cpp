#include "VtkStructuredPointsReader.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
VtkStructuredPointsReader::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                       VtkStructuredPointsReaderInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
VtkStructuredPointsReader::~VtkStructuredPointsReader() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& VtkStructuredPointsReader::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> VtkStructuredPointsReader::operator()()
{

  return {};
}
