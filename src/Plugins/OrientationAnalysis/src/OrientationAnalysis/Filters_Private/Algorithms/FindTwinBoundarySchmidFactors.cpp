#include "FindTwinBoundarySchmidFactors.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindTwinBoundarySchmidFactors::FindTwinBoundarySchmidFactors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                             FindTwinBoundarySchmidFactorsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindTwinBoundarySchmidFactors::~FindTwinBoundarySchmidFactors() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindTwinBoundarySchmidFactors::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindTwinBoundarySchmidFactors::operator()()
{

  return {};
}
