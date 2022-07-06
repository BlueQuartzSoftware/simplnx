#include "ComputeMomentInvariants2D.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ComputeMomentInvariants2D::ComputeMomentInvariants2D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     ComputeMomentInvariants2DInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeMomentInvariants2D::~ComputeMomentInvariants2D() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeMomentInvariants2D::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeMomentInvariants2D::operator()()
{

  return {};
}
