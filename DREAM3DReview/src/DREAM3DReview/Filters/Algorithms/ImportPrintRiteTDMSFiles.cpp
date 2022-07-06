#include "ImportPrintRiteTDMSFiles.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ImportPrintRiteTDMSFiles::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportPrintRiteTDMSFilesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ImportPrintRiteTDMSFiles::~ImportPrintRiteTDMSFiles() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ImportPrintRiteTDMSFiles::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ImportPrintRiteTDMSFiles::operator()()
{

  return {};
}
