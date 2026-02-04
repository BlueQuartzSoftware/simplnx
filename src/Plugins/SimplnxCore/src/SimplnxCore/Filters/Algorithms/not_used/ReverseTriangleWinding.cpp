#include "ReverseTriangleWinding.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ReverseTriangleWinding::ReverseTriangleWinding(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               ReverseTriangleWindingInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReverseTriangleWinding::~ReverseTriangleWinding() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReverseTriangleWinding::operator()()
{

  return {};
}
