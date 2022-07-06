#include "RenameAttributeMatrix.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
RenameAttributeMatrix::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RenameAttributeMatrixInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RenameAttributeMatrix::~RenameAttributeMatrix() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& RenameAttributeMatrix::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> RenameAttributeMatrix::operator()()
{

  return {};
}
