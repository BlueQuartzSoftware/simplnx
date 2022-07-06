#include "ExportDAMASKFiles.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ExportDAMASKFiles::ExportDAMASKFiles(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExportDAMASKFilesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ExportDAMASKFiles::~ExportDAMASKFiles() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ExportDAMASKFiles::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ExportDAMASKFiles::operator()()
{

  return {};
}
