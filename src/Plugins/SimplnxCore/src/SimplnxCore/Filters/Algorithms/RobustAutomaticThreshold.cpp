#include "RobustAutomaticThreshold.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <algorithm>
#include <memory>

using namespace nx::core;

namespace
{
/**
 * @struct FindThresholdFunctor
 * @brief Computes a weighted threshold and writes a Bool mask in two passes.
 *
 * Sequential input order preserves established float rounding. The second pass
 * rereads only scalar values and writes complete mask chunks. No buffer scales
 * with the total tuple count.
 */
struct FindThresholdFunctor
{
  /**
   * @brief Computes the threshold and writes selected mask values.
   * @tparam T Specifies the scalar input type.
   * @param inputObject Supplies scalar values.
   * @param gradMag Supplies float32 gradient weights.
   * @param maskStore Receives Bool threshold results.
   * @param shouldCancel Signals cancellation between chunks.
   * @return Source, gradient, or mask bulk-I/O result.
   * @pre All stores have equal tuple counts and one component.
   *
   * The function does not guard a zero gradient sum. Cancellation during the
   * first pass leaves the mask unchanged. Later cancellation preserves completed
   * mask chunks.
   */
  template <class T>
  Result<> operator()(const IDataArray* inputObject, const Float32AbstractDataStore& gradMag, BoolAbstractDataStore& maskStore, const std::atomic_bool& shouldCancel)
  {
    const auto& inputData = inputObject->template getIDataStoreRefAs<AbstractDataStore<T>>();
    const usize numTuples = inputData.getNumberOfTuples();
    constexpr usize k_TuplesPerBatch = 65536;
    auto inputBuffer = std::make_unique<T[]>(k_TuplesPerBatch);
    auto gradientBuffer = std::make_unique<float32[]>(k_TuplesPerBatch);
    auto maskBuffer = std::make_unique<bool[]>(k_TuplesPerBatch);
    float numerator = 0;
    float denominator = 0;

    // Sequential tuple order preserves the established float accumulation result.
    for(usize start = 0; start < numTuples; start += k_TuplesPerBatch)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(k_TuplesPerBatch, numTuples - start);
      auto result = inputData.copyIntoBuffer(start, nonstd::span<T>(inputBuffer.get(), count));
      if(result.invalid())
      {
        return result;
      }
      result = gradMag.copyIntoBuffer(start, nonstd::span<float32>(gradientBuffer.get(), count));
      if(result.invalid())
      {
        return result;
      }
      for(usize i = 0; i < count; i++)
      {
        numerator += (inputBuffer[i] * gradientBuffer[i]);
        denominator += gradientBuffer[i];
      }
    }

    float threshold = numerator / denominator;

    for(usize start = 0; start < numTuples; start += k_TuplesPerBatch)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(k_TuplesPerBatch, numTuples - start);
      auto result = inputData.copyIntoBuffer(start, nonstd::span<T>(inputBuffer.get(), count));
      if(result.invalid())
      {
        return result;
      }
      for(usize i = 0; i < count; i++)
      {
        maskBuffer[i] = inputBuffer[i] >= threshold;
      }
      result = maskStore.copyFromBuffer(start, nonstd::span<const bool>(maskBuffer.get(), count));
      if(result.invalid())
      {
        return result;
      }
    }
    return {};
  }
};
} // namespace

RobustAutomaticThreshold::RobustAutomaticThreshold(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                   RobustAutomaticThresholdInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

Result<> RobustAutomaticThreshold::operator()()
{
  const auto* inputArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->InputArrayPath);
  const auto* gradientArrayPtr = m_DataStructure.getDataAs<Float32Array>(m_InputValues->GradientArrayPath);
  auto* maskArrayPtr = m_DataStructure.getDataAs<BoolArray>(m_InputValues->InputArrayPath.replaceName(m_InputValues->CreatedMaskName));
  const auto& gradientStoreRef = gradientArrayPtr->getDataStoreRef();
  auto& maskStoreRef = maskArrayPtr->getDataStoreRef();

  if(m_ShouldCancel)
  {
    return {};
  }

  const bool usesOutOfCoreStore = AnyOutOfCore({inputArrayPtr, gradientArrayPtr, maskArrayPtr});
  const bool useOutOfCorePath = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useOutOfCorePath ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);
  return ExecuteDataFunctionNoBool(FindThresholdFunctor{}, inputArrayPtr->getDataType(), inputArrayPtr, gradientStoreRef, maskStoreRef, m_ShouldCancel);
}
