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
    constexpr usize kTuplesPerBatch = 65536;
    auto inputBuffer = std::make_unique<T[]>(kTuplesPerBatch);
    auto gradientBuffer = std::make_unique<float32[]>(kTuplesPerBatch);
    auto maskBuffer = std::make_unique<bool[]>(kTuplesPerBatch);
    float numerator = 0;
    float denominator = 0;

    // Sequential tuple order preserves the established float accumulation result.
    for(usize start = 0; start < numTuples; start += kTuplesPerBatch)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(kTuplesPerBatch, numTuples - start);
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

    for(usize start = 0; start < numTuples; start += kTuplesPerBatch)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(kTuplesPerBatch, numTuples - start);
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

RobustAutomaticThreshold::~RobustAutomaticThreshold() noexcept = default;

Result<> RobustAutomaticThreshold::operator()()
{
  const auto* inputArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->InputArrayPath);
  const auto* gradientArray = m_DataStructure.getDataAs<Float32Array>(m_InputValues->GradientArrayPath);
  auto* maskArray = m_DataStructure.getDataAs<BoolArray>(m_InputValues->InputArrayPath.replaceName(m_InputValues->CreatedMaskName));
  const auto& gradientStoreRef = gradientArray->getDataStoreRef();
  auto& maskStoreRef = maskArray->getDataStoreRef();

  if(m_ShouldCancel)
  {
    return {};
  }

  const bool usesOutOfCoreStore = AnyOutOfCore({inputArray, gradientArray, maskArray});
  const bool useOutOfCorePath = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useOutOfCorePath ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);
  return ExecuteDataFunction(FindThresholdFunctor{}, inputArray->getDataType(), inputArray, gradientStoreRef, maskStoreRef, m_ShouldCancel);
}
