#include "ExportOnScaleTableFile.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ExportOnScaleTableFile::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExportOnScaleTableFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ExportOnScaleTableFile::~ExportOnScaleTableFile() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ExportOnScaleTableFile::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ExportOnScaleTableFile::operator()()
{

  return {};
}
