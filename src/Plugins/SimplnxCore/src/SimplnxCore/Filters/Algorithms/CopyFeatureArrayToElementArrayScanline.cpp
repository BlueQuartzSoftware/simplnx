#include "CopyFeatureArrayToElementArrayScanline.hpp"

#include "CopyFeatureArrayToElementArray.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include <nonstd/span.hpp>

using namespace nx::core;

namespace
{
// Feature Id and output buffers contain at most 65,536 cell tuples.
constexpr usize k_ChunkTuples = 65536;

/**
 * @struct CopyFeatureToElementScanlineFunctor
 * @brief Broadcasts one typed feature array through bulk I/O.
 *
 * The source cache is feature-scale. Cell reads and writes use fixed chunks.
 */
struct CopyFeatureToElementScanlineFunctor
{
  /**
   * @brief Copies one selected feature array to cells.
   * @tparam T Specifies the feature and output scalar type.
   * @param selectedFeatureArray Provides feature tuples.
   * @param featureIdsStore Provides one Feature Id per cell.
   * @param createdArray Receives cell tuples.
   * @param shouldCancel Stops later chunks when true.
   * @return Error from bulk I/O, or success after cancellation.
   */
  template <typename T>
  Result<> operator()(const IDataArray* selectedFeatureArray, const Int32AbstractDataStore& featureIdsStore, IDataArray* createdArray, const std::atomic_bool& shouldCancel)
  {
    const auto& selectedFeatureStore = selectedFeatureArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& createdStore = createdArray->template getIDataStoreRefAs<AbstractDataStore<T>>();

    const usize numComps = selectedFeatureStore.getNumberOfComponents();
    const usize numFeatures = selectedFeatureStore.getNumberOfTuples();
    const usize numCells = featureIdsStore.getNumberOfTuples();

    // A feature-scale cache avoids random source gathers. It can be large when
    // feature count approaches cell count. make_unique supports bool values.
    auto featureCache = std::make_unique<T[]>(numFeatures * numComps);
    auto featureReadResult = selectedFeatureStore.copyIntoBuffer(0, nonstd::span<T>(featureCache.get(), numFeatures * numComps));
    if(featureReadResult.invalid())
    {
      return featureReadResult;
    }

    auto featureIdsBuffer = std::make_unique<int32[]>(k_ChunkTuples);
    auto outputBuffer = std::make_unique<T[]>(k_ChunkTuples * numComps);

    for(usize chunkStart = 0; chunkStart < numCells; chunkStart += k_ChunkTuples)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize chunkTupleCount = std::min(k_ChunkTuples, numCells - chunkStart);
      auto featureIdsReadResult = featureIdsStore.copyIntoBuffer(chunkStart, nonstd::span<int32>(featureIdsBuffer.get(), chunkTupleCount));
      if(featureIdsReadResult.invalid())
      {
        return featureIdsReadResult;
      }

      for(usize cellIdx = 0; cellIdx < chunkTupleCount; cellIdx++)
      {
        if((cellIdx & 0xFFFULL) == 0 && shouldCancel)
        {
          return {};
        }
        const usize srcOffset = numComps * static_cast<usize>(featureIdsBuffer[cellIdx]);
        const usize dstOffset = numComps * cellIdx;
        for(usize compIdx = 0; compIdx < numComps; compIdx++)
        {
          outputBuffer[dstOffset + compIdx] = featureCache[srcOffset + compIdx];
        }
      }

      if(shouldCancel)
      {
        return {};
      }

      auto outputWriteResult = createdStore.copyFromBuffer(chunkStart * numComps, nonstd::span<const T>(outputBuffer.get(), chunkTupleCount * numComps));
      if(outputWriteResult.invalid())
      {
        return outputWriteResult;
      }
    }

    return {};
  }
};
} // namespace

CopyFeatureArrayToElementArrayScanline::CopyFeatureArrayToElementArrayScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                               const CopyFeatureArrayToElementArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

CopyFeatureArrayToElementArrayScanline::~CopyFeatureArrayToElementArrayScanline() noexcept = default;

Result<> CopyFeatureArrayToElementArrayScanline::operator()()
{
  if(m_InputValues->SelectedFeatureArrayPaths.empty())
  {
    return {};
  }

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsPath);

  auto validateResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->SelectedFeatureArrayPaths[0], featureIds, false, m_MessageHandler, &m_ShouldCancel);
  if(validateResult.invalid())
  {
    return validateResult;
  }

  for(const auto& selectedFeatureArrayPath : m_InputValues->SelectedFeatureArrayPaths)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    DataPath createdArrayPath = m_InputValues->FeatureIdsPath.replaceName(selectedFeatureArrayPath.getTargetName() + m_InputValues->CreatedArraySuffix);
    const auto* selectedFeatureArray = m_DataStructure.getDataAs<IDataArray>(selectedFeatureArrayPath);

    m_MessageHandler(IFilter::ProgressMessage{IFilter::ProgressMessage::Type::Info, fmt::format("Copying data into target array '{}'...", createdArrayPath.toString())});

    auto result = ExecuteDataFunction(CopyFeatureToElementScanlineFunctor{}, selectedFeatureArray->getDataType(), selectedFeatureArray, featureIds.getDataStoreRef(),
                                      m_DataStructure.getDataAs<IDataArray>(createdArrayPath), m_ShouldCancel);
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}
