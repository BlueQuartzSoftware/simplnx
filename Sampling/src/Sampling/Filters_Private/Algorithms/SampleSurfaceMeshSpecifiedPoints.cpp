#include "SampleSurfaceMeshSpecifiedPoints.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
SampleSurfaceMeshSpecifiedPoints::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                              SampleSurfaceMeshSpecifiedPointsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SampleSurfaceMeshSpecifiedPoints::~SampleSurfaceMeshSpecifiedPoints() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& SampleSurfaceMeshSpecifiedPoints::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> SampleSurfaceMeshSpecifiedPoints::operator()()
{

  return {};
}
