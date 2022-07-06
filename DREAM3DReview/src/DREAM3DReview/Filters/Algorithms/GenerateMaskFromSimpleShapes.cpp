#include "GenerateMaskFromSimpleShapes.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
GenerateMaskFromSimpleShapes::GenerateMaskFromSimpleShapes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           GenerateMaskFromSimpleShapesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
GenerateMaskFromSimpleShapes::~GenerateMaskFromSimpleShapes() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GenerateMaskFromSimpleShapes::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> GenerateMaskFromSimpleShapes::operator()()
{

  return {};
}
