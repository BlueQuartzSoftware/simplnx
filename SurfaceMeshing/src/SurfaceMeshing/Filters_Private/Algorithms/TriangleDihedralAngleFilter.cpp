#include "TriangleDihedralAngleFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
TriangleDihedralAngleFilter::TriangleDihedralAngleFilter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         TriangleDihedralAngleFilterInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
TriangleDihedralAngleFilter::~TriangleDihedralAngleFilter() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& TriangleDihedralAngleFilter::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> TriangleDihedralAngleFilter::operator()()
{

  return {};
}
