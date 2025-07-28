#include "ComputeFeaturePhasesBinary.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeFeaturePhasesBinary::ComputeFeaturePhasesBinary(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       ComputeFeaturePhasesBinaryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeaturePhasesBinary::~ComputeFeaturePhasesBinary() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeFeaturePhasesBinary::operator()()
{

  return {};
}
