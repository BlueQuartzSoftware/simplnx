#include "AxioVisionV4ToTileConfiguration.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
AxioVisionV4ToTileConfiguration::AxioVisionV4ToTileConfiguration(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                 AxioVisionV4ToTileConfigurationInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AxioVisionV4ToTileConfiguration::~AxioVisionV4ToTileConfiguration() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AxioVisionV4ToTileConfiguration::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AxioVisionV4ToTileConfiguration::operator()()
{

  return {};
}
