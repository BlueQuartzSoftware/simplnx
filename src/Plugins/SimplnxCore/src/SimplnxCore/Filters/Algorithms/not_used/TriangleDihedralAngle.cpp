#include "TriangleDihedralAngle.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
TriangleDihedralAngle::TriangleDihedralAngle(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             TriangleDihedralAngleInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
TriangleDihedralAngle::~TriangleDihedralAngle() noexcept = default;

// -----------------------------------------------------------------------------
Result<> TriangleDihedralAngle::operator()()
{

  return {};
}
