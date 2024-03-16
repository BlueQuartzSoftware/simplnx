#include "ReadVtkStructuredPoints.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/Utilities/VtkLegacyFileReader.hpp"

using namespace nx::core;

namespace
{

// -----------------------------------------------------------------------------
int32_t readHeader()
{
  return 0;
}

} // namespace

// -----------------------------------------------------------------------------
ReadVtkStructuredPoints::ReadVtkStructuredPoints(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ReadVtkStructuredPointsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReadVtkStructuredPoints::~ReadVtkStructuredPoints() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ReadVtkStructuredPoints::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ReadVtkStructuredPoints::operator()()
{

  VtkLegacyFileReader legacyReader(m_InputValues->InputFile);
  legacyReader.setPreflight(false);
  int32 err = legacyReader.readFile();
  return {};
}
