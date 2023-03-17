#include "GenerateOrientationMatrixTranspose.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
GenerateOrientationMatrixTranspose::GenerateOrientationMatrixTranspose(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                       GenerateOrientationMatrixTransposeInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
GenerateOrientationMatrixTranspose::~GenerateOrientationMatrixTranspose() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GenerateOrientationMatrixTranspose::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> GenerateOrientationMatrixTranspose::operator()()
{

  return {};
}
