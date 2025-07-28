#include "CreateAttributeMatrix.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
CreateAttributeMatrix::CreateAttributeMatrix(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             CreateAttributeMatrixInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CreateAttributeMatrix::~CreateAttributeMatrix() noexcept = default;

// -----------------------------------------------------------------------------
Result<> CreateAttributeMatrix::operator()()
{

  return {};
}
