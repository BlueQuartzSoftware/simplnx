#include "ComputeDifferencesMap.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>

using namespace nx::core;
namespace
{
// The batch rounds down to whole tuples. This prevents out-of-core read-modify-write operations.
constexpr usize k_TargetChunkValues = 65536;

/**
 * @struct ExecuteFindDifferenceMapFunctor
 * @brief Computes differences for one selected element type.
 */
struct ExecuteFindDifferenceMapFunctor
{
  /**
   * @brief Computes absolute differences in bounded chunks.
   * @tparam DataType Specifies the array element type.
   * @param firstArray Supplies the first source values.
   * @param secondArray Supplies the second source values.
   * @param differenceMap Receives absolute differences.
   * @param shouldCancel Signals cancellation between chunks.
   * @return Success, or a source or output bulk-I/O error.
   * @pre All arrays have equal tuple and component shapes.
   * @pre Each same-type absolute difference is representable. Signed integer subtraction can otherwise overflow.
   *
   * Cancellation returns success after completed output chunks. Later chunks are not written.
   */
  template <typename DataType>
  Result<> operator()(const IDataArray& firstArray, const IDataArray& secondArray, IDataArray& differenceMap, const std::atomic_bool& shouldCancel) const
  {
    using store_type = AbstractDataStore<DataType>;

    const auto& firstStore = firstArray.template getIDataStoreRefAs<store_type>();
    const auto& secondStore = secondArray.template getIDataStoreRefAs<store_type>();
    auto& differenceStore = differenceMap.template getIDataStoreRefAs<store_type>();

    const usize numTuples = firstStore.getNumberOfTuples();
    const usize numComps = firstStore.getNumberOfComponents();
    const usize chunkTuples = std::max<usize>(1, k_TargetChunkValues / numComps);
    const usize bufferSize = chunkTuples * numComps;
    auto firstBuffer = std::make_unique<DataType[]>(bufferSize);
    auto secondBuffer = std::make_unique<DataType[]>(bufferSize);
    auto differenceBuffer = std::make_unique<DataType[]>(bufferSize);

    for(usize tupleOffset = 0; tupleOffset < numTuples; tupleOffset += chunkTuples)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize valueOffset = tupleOffset * numComps;
      const usize tupleCount = std::min(chunkTuples, numTuples - tupleOffset);
      const usize valueCount = tupleCount * numComps;
      Result<> result = firstStore.copyIntoBuffer(valueOffset, nonstd::span<DataType>(firstBuffer.get(), valueCount));
      if(result.invalid())
      {
        return result;
      }
      result = secondStore.copyIntoBuffer(valueOffset, nonstd::span<DataType>(secondBuffer.get(), valueCount));
      if(result.invalid())
      {
        return result;
      }

      for(usize index = 0; index < valueCount; index++)
      {
        const DataType firstValue = firstBuffer[index];
        const DataType secondValue = secondBuffer[index];
        differenceBuffer[index] = firstValue > secondValue ? firstValue - secondValue : secondValue - firstValue;
      }

      result = differenceStore.copyFromBuffer(valueOffset, nonstd::span<const DataType>(differenceBuffer.get(), valueCount));
      if(result.invalid())
      {
        return result;
      }
    }

    return {};
  }
};
} // namespace

ComputeDifferencesMap::ComputeDifferencesMap(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             ComputeDifferencesMapInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeDifferencesMap::~ComputeDifferencesMap() noexcept = default;

Result<> ComputeDifferencesMap::operator()()
{
  const auto& firstInputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->FirstInputArrayPath);
  const auto& secondInputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SecondInputArrayPath);
  auto& differenceMapArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->DifferenceMapArrayPath);

  if(m_ShouldCancel)
  {
    return {};
  }

  return ExecuteDataFunction(ExecuteFindDifferenceMapFunctor{}, firstInputArray.getDataType(), firstInputArray, secondInputArray, differenceMapArray, m_ShouldCancel);
}
