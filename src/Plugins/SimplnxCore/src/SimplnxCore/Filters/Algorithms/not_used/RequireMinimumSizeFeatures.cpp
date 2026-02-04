#include "RequireMinimumSizeFeatures.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
RequireMinimumSizeFeatures::RequireMinimumSizeFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       RequireMinimumSizeFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RequireMinimumSizeFeatures::~RequireMinimumSizeFeatures() noexcept = default;

// -----------------------------------------------------------------------------
Result<> RequireMinimumSizeFeatures::operator()()
{

  return {};
}
