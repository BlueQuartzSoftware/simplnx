#include "ItkConvertArrayTo8BitImageAttributeMatrix.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ItkConvertArrayTo8BitImageAttributeMatrix::ItkConvertArrayTo8BitImageAttributeMatrix(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                                     ItkConvertArrayTo8BitImageAttributeMatrixInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ItkConvertArrayTo8BitImageAttributeMatrix::~ItkConvertArrayTo8BitImageAttributeMatrix() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ItkConvertArrayTo8BitImageAttributeMatrix::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ItkConvertArrayTo8BitImageAttributeMatrix::operator()()
{

  return {};
}
