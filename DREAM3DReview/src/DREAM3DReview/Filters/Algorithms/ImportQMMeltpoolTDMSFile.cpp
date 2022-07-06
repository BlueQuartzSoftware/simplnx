#include "ImportQMMeltpoolTDMSFile.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ImportQMMeltpoolTDMSFile::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportQMMeltpoolTDMSFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ImportQMMeltpoolTDMSFile::~ImportQMMeltpoolTDMSFile() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ImportQMMeltpoolTDMSFile::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ImportQMMeltpoolTDMSFile::operator()()
{

  return {};
}
