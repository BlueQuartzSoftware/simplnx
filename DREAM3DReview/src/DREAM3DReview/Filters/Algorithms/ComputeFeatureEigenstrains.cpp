#include "ComputeFeatureEigenstrains.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ComputeFeatureEigenstrains::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                        ComputeFeatureEigenstrainsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureEigenstrains::~ComputeFeatureEigenstrains() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeFeatureEigenstrains::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeFeatureEigenstrains::operator()()
{

  return {};
}
