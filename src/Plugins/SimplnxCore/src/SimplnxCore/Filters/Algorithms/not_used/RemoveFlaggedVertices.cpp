#include "RemoveFlaggedVertices.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
RemoveFlaggedVertices::RemoveFlaggedVertices(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             RemoveFlaggedVerticesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RemoveFlaggedVertices::~RemoveFlaggedVertices() noexcept = default;

// -----------------------------------------------------------------------------
Result<> RemoveFlaggedVertices::operator()()
{

  return {};
}
