#include "RobustAutomaticThreshold.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

namespace
{
struct FindThresholdFunctor
{
  template <class T>
  void operator()(const IDataArray* inputObject, const Float32AbstractDataStore& gradMag, BoolAbstractDataStore& maskStore)
  {
    const auto& inputData = inputObject->template getIDataStoreRefAs<AbstractDataStore<T>>();
    usize numTuples = inputData.getNumberOfTuples();
    float numerator = 0;
    float denominator = 0;

    for(usize i = 0; i < numTuples; i++)
    {
      numerator += (inputData.getValue(i) * gradMag.getValue(i));
      denominator += gradMag.getValue(i);
    }

    float threshold = numerator / denominator;

    for(usize i = 0; i < numTuples; i++)
    {
      maskStore.setValue(i, inputData.getValue(i) >= threshold);
    }
  }
};
} // namespace

// -----------------------------------------------------------------------------
RobustAutomaticThreshold::RobustAutomaticThreshold(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                   RobustAutomaticThresholdInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RobustAutomaticThreshold::~RobustAutomaticThreshold() noexcept = default;

// -----------------------------------------------------------------------------
Result<> RobustAutomaticThreshold::operator()()
{
  const auto* inputArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->InputArrayPath);
  const auto& gradientStoreRef = m_DataStructure.getDataAs<Float32Array>(m_InputValues->GradientArrayPath)->getDataStoreRef();
  auto& maskStoreRef = m_DataStructure.getDataAs<BoolArray>(m_InputValues->InputArrayPath.replaceName(m_InputValues->CreatedMaskName))->getDataStoreRef();

  ExecuteNeighborFunction(FindThresholdFunctor{}, inputArray->getDataType(), inputArray, gradientStoreRef, maskStoreRef);

  return {};
}
