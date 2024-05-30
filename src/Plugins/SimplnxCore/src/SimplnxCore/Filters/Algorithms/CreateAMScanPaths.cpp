#include "CreateAMScanPaths.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
CreateAMScanPaths::CreateAMScanPaths(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateAMScanPathsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CreateAMScanPaths::~CreateAMScanPaths() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CreateAMScanPaths::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CreateAMScanPaths::operator()()
{

  return {};
}
