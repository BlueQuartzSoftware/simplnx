#include "FindMinkowskiBouligandDimension.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindMinkowskiBouligandDimension::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             FindMinkowskiBouligandDimensionInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindMinkowskiBouligandDimension::~FindMinkowskiBouligandDimension() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindMinkowskiBouligandDimension::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindMinkowskiBouligandDimension::operator()()
{

  return {};
}
