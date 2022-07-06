#include "ExportMultiOnScaleTableFile.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ExportMultiOnScaleTableFile::ExportMultiOnScaleTableFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         ExportMultiOnScaleTableFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ExportMultiOnScaleTableFile::~ExportMultiOnScaleTableFile() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ExportMultiOnScaleTableFile::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ExportMultiOnScaleTableFile::operator()()
{

  return {};
}
