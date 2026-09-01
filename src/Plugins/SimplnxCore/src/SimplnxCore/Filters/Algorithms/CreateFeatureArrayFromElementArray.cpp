#include "CreateFeatureArrayFromElementArray.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <algorithm>
#include <memory>
#include <nonstd/span.hpp>

using namespace nx::core;

namespace
{
// Cell buffers stay fixed regardless of the total element count.
constexpr usize k_ChunkTuples = 65536;

/**
 * @brief Finds the largest nonnegative feature ID with bounded reads.
 * @param featureIdsStore Supplies feature IDs.
 * @param shouldCancel Signals cancellation between chunks.
 * @return Maximum ID, zero after cancellation, or an input or read error.
 */
Result<int32> findMaximumFeatureId(const Int32AbstractDataStore& featureIdsStore, const std::atomic_bool& shouldCancel)
{
  const usize totalTuples = featureIdsStore.getNumberOfTuples();
  auto featureIdsBuffer = std::make_unique<int32[]>(k_ChunkTuples);
  int32 maximumFeatureId = 0;

  for(usize chunkStart = 0; chunkStart < totalTuples; chunkStart += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {0};
    }

    const usize chunkTupleCount = std::min(k_ChunkTuples, totalTuples - chunkStart);
    Result<> readResult = featureIdsStore.copyIntoBuffer(chunkStart, nonstd::span<int32>(featureIdsBuffer.get(), chunkTupleCount));
    if(readResult.invalid())
    {
      return ConvertResultTo<int32>(std::move(readResult), int32{0});
    }

    for(usize cellIdx = 0; cellIdx < chunkTupleCount; cellIdx++)
    {
      const int32 featureId = featureIdsBuffer[cellIdx];
      if(featureId < 0)
      {
        return MakeErrorResult<int32>(-81880, "Invalid Input, Feature Ids Array must not contain negative values");
      }
      maximumFeatureId = std::max(maximumFeatureId, featureId);
    }
  }

  return {maximumFeatureId};
}

/**
 * @struct CopyCellDataFunctor
 * @brief Dispatches element-to-feature copying by runtime value type.
 */
struct CopyCellDataFunctor
{
  /**
   * @brief Copies element values through bounded bulk transfers.
   * @tparam T Element and feature value type.
   * @param selectedCellArray Supplies element values.
   * @param featureIdsStore Maps elements to feature tuples.
   * @param createdArray Receives final feature values.
   * @param shouldCancel Signals cancellation between chunks.
   * @return Success with an optional inconsistency warning, or a transfer error.
   *
   * The output write occurs only after all chunks complete. Cancellation does
   * not publish the partially assembled feature buffer.
   */
  template <typename T>
  Result<> operator()(const IDataArray* selectedCellArray, const Int32AbstractDataStore& featureIdsStore, IDataArray* createdArray, const std::atomic_bool& shouldCancel) const
  {
    const auto& selectedCellStore = selectedCellArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& createdDataStore = createdArray->template getIDataStoreRefAs<AbstractDataStore<T>>();

    const usize totalCellArrayComponents = selectedCellStore.getNumberOfComponents();
    const usize totalCellArrayTuples = selectedCellStore.getNumberOfTuples();
    const usize totalFeatures = createdDataStore.getNumberOfTuples();
    const usize featureValueCount = totalFeatures * totalCellArrayComponents;

    // These arrays scale with feature count, not cell count. Caching first and
    // final values keeps all hot-loop work local and preserves last-value-wins behavior.
    auto firstFeatureValues = std::make_unique<T[]>(featureValueCount);
    auto createdFeatureValues = std::make_unique<T[]>(featureValueCount);
    auto featureWasSeen = std::make_unique<bool[]>(totalFeatures);
    auto featureIdsBuffer = std::make_unique<int32[]>(k_ChunkTuples);
    auto cellValuesBuffer = std::make_unique<T[]>(k_ChunkTuples * totalCellArrayComponents);
    Result<> result;

    for(usize chunkStart = 0; chunkStart < totalCellArrayTuples; chunkStart += k_ChunkTuples)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize chunkTupleCount = std::min(k_ChunkTuples, totalCellArrayTuples - chunkStart);
      const usize chunkValueCount = chunkTupleCount * totalCellArrayComponents;
      Result<> readResult = featureIdsStore.copyIntoBuffer(chunkStart, nonstd::span<int32>(featureIdsBuffer.get(), chunkTupleCount));
      if(readResult.invalid())
      {
        return readResult;
      }
      readResult = selectedCellStore.copyIntoBuffer(chunkStart * totalCellArrayComponents, nonstd::span<T>(cellValuesBuffer.get(), chunkValueCount));
      if(readResult.invalid())
      {
        return readResult;
      }

      for(usize cellIdx = 0; cellIdx < chunkTupleCount; cellIdx++)
      {
        const usize featureIdx = static_cast<usize>(featureIdsBuffer[cellIdx]);
        const usize featureValueOffset = featureIdx * totalCellArrayComponents;
        const usize cellValueOffset = cellIdx * totalCellArrayComponents;

        if(!featureWasSeen[featureIdx])
        {
          std::copy_n(cellValuesBuffer.get() + cellValueOffset, totalCellArrayComponents, firstFeatureValues.get() + featureValueOffset);
          featureWasSeen[featureIdx] = true;
        }

        for(usize cellCompIdx = 0; cellCompIdx < totalCellArrayComponents; cellCompIdx++)
        {
          const T currentCellValue = cellValuesBuffer[cellValueOffset + cellCompIdx];
          if(currentCellValue != firstFeatureValues[featureValueOffset + cellCompIdx] && result.warnings().empty())
          {
            result.warnings().push_back(
                Warning{-1000, fmt::format("Elements from Feature {} do not all have the same value. The last value copied into Feature {} will be used", featureIdx, featureIdx)});
          }
          createdFeatureValues[featureValueOffset + cellCompIdx] = currentCellValue;
        }
      }
    }

    Result<> writeResult = createdDataStore.copyFromBuffer(0, nonstd::span<const T>(createdFeatureValues.get(), featureValueCount));
    if(writeResult.invalid())
    {
      return writeResult;
    }

    return result;
  }
};
} // namespace

// -----------------------------------------------------------------------------
CreateFeatureArrayFromElementArray::CreateFeatureArrayFromElementArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                       CreateFeatureArrayFromElementArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CreateFeatureArrayFromElementArray::~CreateFeatureArrayFromElementArray() noexcept = default;

// -----------------------------------------------------------------------------
Result<> CreateFeatureArrayFromElementArray::operator()()
{
  const DataPath createdArrayPath = m_InputValues->CellFeatureAttributeMatrixPath.createChildPath(m_InputValues->CreatedArrayName);
  const auto* selectedCellArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->SelectedCellArrayPath);
  const auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();
  auto* createdArray = m_DataStructure.getDataAs<IDataArray>(createdArrayPath);

  const usize totalCellTuples = featureIdsStore.getNumberOfTuples();
  if(totalCellTuples == 0)
  {
    return MakeErrorResult(-81882, "Invalid Input, Feature Ids Array must not be empty");
  }

  auto maximumFeatureIdResult = findMaximumFeatureId(featureIdsStore, m_ShouldCancel);
  if(maximumFeatureIdResult.invalid())
  {
    return ConvertResult(std::move(maximumFeatureIdResult));
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  const usize requiredFeatureTuples = static_cast<usize>(maximumFeatureIdResult.value()) + 1;
  auto& cellFeatureAttrMat = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->CellFeatureAttributeMatrixPath);

  // Validate that growing the attribute matrix will not shrink a larger child array.
  if(requiredFeatureTuples > cellFeatureAttrMat.getNumberOfTuples())
  {
    for(const auto& childObject : cellFeatureAttrMat)
    {
      const auto* iArray = dynamic_cast<IArray*>(childObject.second.get());
      if(iArray != nullptr && iArray->getNumberOfTuples() > requiredFeatureTuples)
      {
        return MakeErrorResult(-81881, fmt::format("Resizing would cause data loss in {}. Make sure all objects in {} have tuple counts equal to or less then the max Feature ID {}!",
                                                   iArray->getName(), m_InputValues->CellFeatureAttributeMatrixPath.toString(), requiredFeatureTuples));
      }
    }

    cellFeatureAttrMat.resizeTuples({requiredFeatureTuples});
  }

  return ExecuteDataFunction(CopyCellDataFunctor{}, selectedCellArray->getDataType(), selectedCellArray, featureIdsStore, createdArray, m_ShouldCancel);
}
