#include "VMFillLevelSetWithTetrahedra.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
VMFillLevelSetWithTetrahedra::VMFillLevelSetWithTetrahedra(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           VMFillLevelSetWithTetrahedraInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
VMFillLevelSetWithTetrahedra::~VMFillLevelSetWithTetrahedra() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& VMFillLevelSetWithTetrahedra::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> VMFillLevelSetWithTetrahedra::operator()()
{

  return {};
}
