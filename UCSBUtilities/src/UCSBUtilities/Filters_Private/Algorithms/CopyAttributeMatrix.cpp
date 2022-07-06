#include "CopyAttributeMatrix.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
CopyAttributeMatrix::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CopyAttributeMatrixInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CopyAttributeMatrix::~CopyAttributeMatrix() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CopyAttributeMatrix::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CopyAttributeMatrix::operator()()
{

  return {};
}
