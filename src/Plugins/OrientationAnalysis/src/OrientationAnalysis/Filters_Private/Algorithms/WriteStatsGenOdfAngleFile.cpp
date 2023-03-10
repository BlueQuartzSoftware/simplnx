#include "WriteStatsGenOdfAngleFile.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
WriteStatsGenOdfAngleFile::WriteStatsGenOdfAngleFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     WriteStatsGenOdfAngleFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WriteStatsGenOdfAngleFile::~WriteStatsGenOdfAngleFile() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& WriteStatsGenOdfAngleFile::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> WriteStatsGenOdfAngleFile::operator()()
{

  return {};
}
