#include "RodriguesConvertor.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>

using namespace nx::core;

namespace
{
/**
 * @class RodriguesConvertorImpl
 * @brief Converts disjoint resident tuple ranges through direct array indexing.
 *
 * Parallel workers share the array objects and write disjoint output tuples.
 * This design does not establish generic DataArray or DataStore thread safety.
 */
class RodriguesConvertorImpl
{
private:
  const Float32Array* m_Input;
  Float32Array* m_Output;
  const std::atomic_bool* m_ShouldCancel;

public:
  RodriguesConvertorImpl(const Float32Array* inputQuat, Float32Array* outputQuat, const std::atomic_bool* shouldCancel)
  : m_Input(inputQuat)
  , m_Output(outputQuat)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void convert(size_t start, size_t end) const
  {
    for(size_t i = start; i < end; i++)
    {
      if(*m_ShouldCancel)
      {
        return;
      }
      const float r0 = (*m_Input)[i * 3];
      const float r1 = (*m_Input)[i * 3 + 1];
      const float r2 = (*m_Input)[i * 3 + 2];
      // The filter contract requires nonzero triples. A zero magnitude produces
      // nonfinite axis components in the current implementation.
      const float length = sqrtf(r0 * r0 + r1 * r1 + r2 * r2);

      (*m_Output)[i * 4] = r0 / length;
      (*m_Output)[i * 4 + 1] = r1 / length;
      (*m_Output)[i * 4 + 2] = r2 / length;
      (*m_Output)[i * 4 + 3] = length;
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }
};

/**
 * @brief Converts Rodrigues triples with bounded three-component reads and four-component writes.
 * @param input Supplies three-component Rodrigues tuples.
 * @param output Receives unit-axis and magnitude tuples.
 * @param shouldCancel Signals cancellation between pages and before each write.
 * @return Input or output bulk-I/O errors. Cancellation returns success after completed pages.
 * @pre Input and output tuple counts match, with three and four components respectively.
 * @pre Each Rodrigues triple has nonzero magnitude.
 *
 * The mathematical operation and tuple order match the direct worker. Local
 * buffers hold at most 65,536 tuples.
 */
Result<> ConvertRodriguesBulk(const Float32Array& input, Float32Array& output, const std::atomic_bool& shouldCancel)
{
  constexpr usize k_ChunkTuples = 65536;
  auto inputValues = std::make_unique<float32[]>(k_ChunkTuples * 3);
  auto outputValues = std::make_unique<float32[]>(k_ChunkTuples * 4);
  const auto& inputStore = input.getDataStoreRef();
  auto& outputStore = output.getDataStoreRef();
  const usize tupleCount = input.getNumberOfTuples();
  for(usize tupleOffset = 0; tupleOffset < tupleCount; tupleOffset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkTuples, tupleCount - tupleOffset);
    auto readResult = inputStore.copyIntoBuffer(tupleOffset * 3, nonstd::span<float32>(inputValues.get(), count * 3));
    if(readResult.invalid())
    {
      return readResult;
    }
    for(usize localTuple = 0; localTuple < count; ++localTuple)
    {
      const usize inputOffset = localTuple * 3;
      const usize outputOffset = localTuple * 4;
      const float32 r0 = inputValues[inputOffset];
      const float32 r1 = inputValues[inputOffset + 1];
      const float32 r2 = inputValues[inputOffset + 2];
      // The filter contract requires nonzero triples.
      const float32 length = sqrtf(r0 * r0 + r1 * r1 + r2 * r2);
      outputValues[outputOffset] = r0 / length;
      outputValues[outputOffset + 1] = r1 / length;
      outputValues[outputOffset + 2] = r2 / length;
      outputValues[outputOffset + 3] = length;
    }
    if(shouldCancel)
    {
      return {};
    }
    auto writeResult = outputStore.copyFromBuffer(tupleOffset * 4, nonstd::span<const float32>(outputValues.get(), count * 4));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  return {};
}

} // namespace

RodriguesConvertor::RodriguesConvertor(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RodriguesConvertorInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

RodriguesConvertor::~RodriguesConvertor() noexcept = default;

const std::atomic_bool& RodriguesConvertor::getCancel()
{
  return m_ShouldCancel;
}

Result<> RodriguesConvertor::operator()()
{
  const auto& input = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->RodriguesDataArrayPath);
  auto& output = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->OutputDataArrayPath);

  const bool usesOutOfCoreStore = IsOutOfCore(input) || IsOutOfCore(output);
  const bool useOutOfCoreAlgorithm = !ForceInCoreAlgorithm() && (ForceOocAlgorithm() || usesOutOfCoreStore);
  RecordAlgorithmPathExecution(useOutOfCoreAlgorithm ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);
  if(useOutOfCoreAlgorithm)
  {
    return ConvertRodriguesBulk(input, output, m_ShouldCancel);
  }

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, input.getNumberOfTuples());
  dataAlg.execute(RodriguesConvertorImpl(&input, &output, &m_ShouldCancel));

  return {};
}
