#include "ComputeArrayHistogramByFeature.hpp"

#include "SimplnxCore/Filters/ComputeArrayHistogramByFeatureFilter.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/IO/Generic/IExternalSort.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/HistogramUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @brief Discovers the output feature count with a single bounded scan of FeatureIds.
 * @param featureIdsStore Feature identifier store.
 * @param shouldCancel Cancellation flag.
 * @return One greater than the largest nonnegative identifier, zero for cancellation
 * or no nonnegative identifiers, or a bulk-read or conversion error.
 *
 * Fixed pages avoid a FeatureIds-sized resident buffer.
 */
Result<usize> findFeatureCount(const AbstractDataStore<int32>& featureIdsStore, const std::atomic_bool& shouldCancel)
{
  constexpr usize k_ChunkTuples = 65536;
  const usize numTuples = featureIdsStore.getNumberOfTuples();
  std::vector<int32> buffer(k_ChunkTuples);
  int32 maxFeatureId = -1;
  for(usize offset = 0; offset < numTuples; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {usize{0}};
    }
    const usize count = std::min(k_ChunkTuples, numTuples - offset);
    Result<> readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(buffer.data(), count));
    if(readResult.invalid())
    {
      return ConvertInvalidResult<usize>(std::move(readResult));
    }
    for(usize index = 0; index < count; ++index)
    {
      maxFeatureId = std::max(maxFeatureId, buffer[index]);
    }
  }
  if(maxFeatureId < 0)
  {
    return {usize{0}};
  }
  if(static_cast<uint64>(maxFeatureId) >= std::numeric_limits<usize>::max() - 1)
  {
    return MakeErrorResult<usize>(-23802, fmt::format("ComputeArrayHistogramByFeature: largest non-negative FeatureId ({}) cannot be converted to a feature count on this platform.", maxFeatureId));
  }
  return {static_cast<usize>(maxFeatureId) + 1};
}
} // namespace

/**
 * @class GenerateFeatureHistogramImpl
 * @brief Computes resident histograms for one parallel feature range.
 * @tparam Type Input and bin-range value type.
 * @tparam SizeType Histogram count value type.
 *
 * Each worker borrows the resident stores and owns only its feature-scale
 * buffers. Precomputed bin geometry lets one bounded cell scan update all
 * features in the worker range.
 */
template <typename Type, std::integral SizeType>
class GenerateFeatureHistogramImpl
{
public:
  /**
   * @brief Creates a resident worker with modal-range output.
   * @param inputStore Input values.
   * @param binRangesStore Receives bin ranges.
   * @param modalBinRangesList Receives modal bin ranges.
   * @param featureIdsStore Maps tuples to features.
   * @param histMin User-defined histogram minimum.
   * @param histMax User-defined histogram maximum.
   * @param histFullRange True to derive each feature range from its values.
   * @param shouldCancel Cancellation flag.
   * @param numBins Number of histogram bins.
   * @param histogramStore Receives bin counts.
   * @param mostPopulatedStore Receives most-populated bin data.
   * @param mask Selects accepted tuples.
   * @param overflow Counts values outside configured ranges.
   * @param progressMessageHelper Reports progress.
   * @pre Referenced stores, mask, and progress helper outlive this worker.
   */
  GenerateFeatureHistogramImpl(const AbstractDataStore<Type>& inputStore, AbstractDataStore<Type>& binRangesStore, NeighborList<Type>* modalBinRangesList,
                               const AbstractDataStore<int32>& featureIdsStore, float64 histMin, float64 histMax, bool histFullRange, const std::atomic_bool& shouldCancel, const int32 numBins,
                               AbstractDataStore<SizeType>& histogramStore, AbstractDataStore<SizeType>& mostPopulatedStore, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask,
                               std::atomic<usize>& overflow, ProgressMessageHelper& progressMessageHelper)
  : m_InputStore(inputStore)
  , m_ShouldCancel(shouldCancel)
  , m_NumBins(numBins)
  , m_BinRangesStore(binRangesStore)
  , m_ModalBinRangesList(modalBinRangesList)
  , m_HistMin(histMin)
  , m_HistMax(histMax)
  , m_HistFullRange(histFullRange)
  , m_HistogramStore(histogramStore)
  , m_MostPopulatedStore(mostPopulatedStore)
  , m_FeatureIdsStore(featureIdsStore)
  , m_Mask(mask)
  , m_Overflow(overflow)
  , m_ProgressMessageHelper(progressMessageHelper)
  {
  }

  /**
   * @brief Creates a resident worker without modal-range output.
   * @param inputStore Input values.
   * @param binRangesStore Receives bin ranges.
   * @param featureIdsStore Maps tuples to features.
   * @param histMin User-defined histogram minimum.
   * @param histMax User-defined histogram maximum.
   * @param histFullRange True to derive each feature range from its values.
   * @param shouldCancel Cancellation flag.
   * @param numBins Number of histogram bins.
   * @param histogramStore Receives bin counts.
   * @param mostPopulatedStore Receives most-populated bin data.
   * @param mask Selects accepted tuples.
   * @param overflow Counts values outside configured ranges.
   * @param progressMessageHelper Reports progress.
   * @pre Referenced stores, mask, and progress helper outlive this worker.
   */
  GenerateFeatureHistogramImpl(const AbstractDataStore<Type>& inputStore, AbstractDataStore<Type>& binRangesStore, const AbstractDataStore<int32>& featureIdsStore, float64 histMin, float64 histMax,
                               bool histFullRange, const std::atomic_bool& shouldCancel, const int32 numBins, AbstractDataStore<SizeType>& histogramStore,
                               AbstractDataStore<SizeType>& mostPopulatedStore, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask, std::atomic<usize>& overflow,
                               ProgressMessageHelper& progressMessageHelper)
  : m_InputStore(inputStore)
  , m_ShouldCancel(shouldCancel)
  , m_NumBins(numBins)
  , m_BinRangesStore(binRangesStore)
  , m_ModalBinRangesList(nullptr)
  , m_HistMin(histMin)
  , m_HistMax(histMax)
  , m_HistFullRange(histFullRange)
  , m_HistogramStore(histogramStore)
  , m_MostPopulatedStore(mostPopulatedStore)
  , m_FeatureIdsStore(featureIdsStore)
  , m_Mask(mask)
  , m_Overflow(overflow)
  , m_ProgressMessageHelper(progressMessageHelper)
  {
  }

  /**
   * @brief Destroys the resident histogram worker.
   */
  ~GenerateFeatureHistogramImpl() = default;

  /**
   * @brief Computes one parallel feature range.
   * @param range Inclusive-exclusive feature range.
   */
  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

  /**
   * @brief Computes histograms for an inclusive-exclusive feature range.
   * @param start First feature identifier.
   * @param end One past the last feature identifier.
   *
   * The method first builds feature-scale bin geometry. One fixed-page cell scan
   * then routes every accepted tuple to its feature. Finalization writes counts,
   * ranges, most-populated bins, and optional modal ranges.
   */
  void compute(usize start, usize end) const
  {
    ProgressMessenger progressMessenger = m_ProgressMessageHelper.createProgressMessenger();

    const usize numTuples = m_FeatureIdsStore.getNumberOfTuples();
    const usize numCurrentFeatures = end - start;

    auto [length, min, max, summation, modalMaps] = HistogramUtilities::concurrent::CalculateFeatureHasDataStats(m_InputStore, m_FeatureIdsStore, start, end, m_Mask, {}, m_ShouldCancel);
    if(m_ShouldCancel)
    {
      return;
    }

    // Precompute feature-scale bin data so one cell scan updates every histogram.
    std::vector<Type> histMinPerFeature(numCurrentFeatures, static_cast<Type>(0));
    std::vector<float32> incrementPerFeature(numCurrentFeatures, 0.0F);
    std::vector<std::vector<Type>> rangesPerFeature(numCurrentFeatures);
    std::vector<std::vector<uint64>> histogramPerFeature(numCurrentFeatures);

    for(usize localFeatureIndex = 0; localFeatureIndex < numCurrentFeatures; localFeatureIndex++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      rangesPerFeature[localFeatureIndex] = std::vector<Type>(m_NumBins * 2);
      histogramPerFeature[localFeatureIndex] = std::vector<uint64>(m_NumBins, 0);

      if(length[localFeatureIndex] == 0)
      {
        continue; // Empty features retain zeroed output values.
      }

      auto histMin = static_cast<Type>(m_HistMin);
      auto histMax = static_cast<Type>(m_HistMax);
      if(m_HistFullRange)
      {
        histMin = min[localFeatureIndex];
        histMax = max[localFeatureIndex] + static_cast<Type>(1.0);
      }

      HistogramUtilities::serial::FillBinRanges(rangesPerFeature[localFeatureIndex], std::make_pair(histMin, histMax), m_NumBins);

      const float32 increment = HistogramUtilities::serial::CalculateIncrement(histMin, histMax, m_NumBins);
      histMinPerFeature[localFeatureIndex] = histMin;
      incrementPerFeature[localFeatureIndex] = increment;

      // A near-zero increment places all accepted values in bin zero.
      if(std::fabs(increment) < 1E-10)
      {
        histogramPerFeature[localFeatureIndex][0] = length[localFeatureIndex];
      }
    }

    // Fixed pages route each accepted tuple without a per-feature cell rescan.
    constexpr usize k_ChunkTuples = 65536;
    auto featureIdsBuffer = std::make_unique<int32[]>(k_ChunkTuples);
    auto valueBuffer = std::make_unique<Type[]>(k_ChunkTuples);

    for(usize chunkStart = 0; chunkStart < numTuples; chunkStart += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const usize chunkTupleCount = std::min(k_ChunkTuples, numTuples - chunkStart);
      m_FeatureIdsStore.copyIntoBuffer(chunkStart, nonstd::span<int32>(featureIdsBuffer.get(), chunkTupleCount));
      m_InputStore.copyIntoBuffer(chunkStart, nonstd::span<Type>(valueBuffer.get(), chunkTupleCount));

      for(usize cellIdx = 0; cellIdx < chunkTupleCount; cellIdx++)
      {
        const usize globalIdx = chunkStart + cellIdx;
        if(m_Mask != nullptr && !m_Mask->isTrue(globalIdx))
        {
          continue;
        }

        const int32 featureId = featureIdsBuffer[cellIdx];
        if(featureId < static_cast<int32>(start) || featureId >= static_cast<int32>(end))
        {
          continue; // Another worker owns this feature.
        }

        const usize localFeatureIndex = static_cast<usize>(featureId) - start;
        if(length[localFeatureIndex] == 0)
        {
          continue;
        }

        const float32 increment = incrementPerFeature[localFeatureIndex];
        if(std::fabs(increment) < 1E-10)
        {
          continue; // The first pass already counted this degenerate feature in bin zero.
        }

        // vector<bool> yields a proxy, so CalculateBin needs a concrete Type value.
        const Type histMin = histMinPerFeature[localFeatureIndex];
        const Type value = valueBuffer[cellIdx];
        const auto bin = static_cast<int32>(HistogramUtilities::serial::CalculateBin(value, histMin, increment));
        if((bin >= 0) && (bin < m_NumBins))
        {
          histogramPerFeature[localFeatureIndex][bin]++;
        }
        else
        {
          m_Overflow++;
        }
      }
    }

    // Finalize feature-scale outputs after the cell scan completes.
    usize progressIncrement = numCurrentFeatures / 100;
    usize progressCount = 0;
    for(usize j = start; j < end; j++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      const usize localFeatureIndex = j - start;
      const std::vector<uint64>& histogram = histogramPerFeature[localFeatureIndex];
      const std::vector<Type>& ranges = rangesPerFeature[localFeatureIndex];

      if(length[localFeatureIndex] > 0)
      {
        const Type histMin = histMinPerFeature[localFeatureIndex];
        const float32 increment = incrementPerFeature[localFeatureIndex];

        // Boolean input cannot produce modal NeighborList output.
        if constexpr(!std::is_same_v<Type, bool>)
        {
          if(m_ModalBinRangesList != nullptr)
          {
            if(std::fabs(increment) < 1E-10)
            {
              // The modal contract uses [0, featureCount), independent of worker range.
              m_ModalBinRangesList->addEntry(j, static_cast<Type>(0));
              m_ModalBinRangesList->addEntry(j, static_cast<Type>(m_HistogramStore.getNumberOfTuples()));
            }
            else if(!modalMaps[localFeatureIndex].empty())
            {
              auto pr = std::max_element(modalMaps[localFeatureIndex].begin(), modalMaps[localFeatureIndex].end(), [](const auto& x, const auto& y) { return x.second < y.second; });
              int maxCount = pr->second;

              for(const auto& modalPair : modalMaps[localFeatureIndex])
              {
                if(modalPair.second == maxCount)
                {
                  const Type mode = modalPair.first;
                  const auto modalBin = HistogramUtilities::serial::CalculateBin(mode, histMin, increment);
                  if((modalBin >= 0) && (modalBin < m_NumBins))
                  {
                    m_ModalBinRangesList->addEntry(j, ranges[modalBin]);
                    m_ModalBinRangesList->addEntry(j, ranges[modalBin + 1]);
                  }
                }
              }
            }
          }
        }
      }

      for(usize k = 0; k < histogram.size(); k++)
      {
        m_HistogramStore.setComponent(j, k, histogram[k]);
      }
      for(usize k = 0; k < ranges.size(); k++)
      {
        m_BinRangesStore.setComponent(j, k, ranges[k]);
      }

      auto maxElementIt = std::max_element(histogram.begin(), histogram.end());
      uint64 index = std::distance(histogram.begin(), maxElementIt);
      m_MostPopulatedStore.setComponent(j, 0, index);
      m_MostPopulatedStore.setComponent(j, 1, histogram[index]);

      progressCount++;
      if(progressCount > progressIncrement)
      {
        progressMessenger.sendProgressMessage(progressCount,
                                              [&](usize currentProgress, usize maxProgress) { return fmt::format("Calculating feature histograms {}/{}", currentProgress, maxProgress); });
        progressCount = 0;
      }
    }

    progressMessenger.sendProgressMessage(progressCount);
  }

private:
  const std::atomic_bool& m_ShouldCancel;
  float64 m_HistMin;
  float64 m_HistMax;
  bool m_HistFullRange;
  int32 m_NumBins;
  const std::unique_ptr<MaskCompareUtilities::MaskCompare>& m_Mask;
  const AbstractDataStore<Type>& m_InputStore;
  const AbstractDataStore<int32>& m_FeatureIdsStore;
  AbstractDataStore<SizeType>& m_HistogramStore;
  AbstractDataStore<Type>& m_BinRangesStore;
  AbstractDataStore<uint64>& m_MostPopulatedStore;
  NeighborList<Type>* m_ModalBinRangesList;
  std::atomic<usize>& m_Overflow;
  ProgressMessageHelper& m_ProgressMessageHelper;
};

/**
 * @struct InstantiateHistogramByFeatureImplFunctor
 * @brief Creates a typed resident histogram worker for parallel execution.
 */
struct InstantiateHistogramByFeatureImplFunctor
{
  /**
   * @brief Creates a worker with modal-range output.
   * @tparam T Input and bin-range value type.
   * @tparam ArgsT Forwarded worker argument types.
   * @param modalBinRangesNL Untyped modal-range list.
   * @param inputArray Input array.
   * @param binRangesArray Bin-range output array.
   * @param args Forwarded worker arguments.
   * @return Typed resident histogram worker.
   */
  template <typename T, class... ArgsT>
  auto operator()(INeighborList* modalBinRangesNL, const IDataArray* inputArray, IDataArray* binRangesArray, ArgsT&&... args)
  {
    return GenerateFeatureHistogramImpl(inputArray->template getIDataStoreRefAs<AbstractDataStore<T>>(), binRangesArray->template getIDataStoreRefAs<AbstractDataStore<T>>(),
                                        dynamic_cast<NeighborList<T>*>(modalBinRangesNL), std::forward<ArgsT>(args)...);
  }

  /**
   * @brief Creates a worker without modal-range output.
   * @tparam T Input and bin-range value type.
   * @tparam ArgsT Forwarded worker argument types.
   * @param inputArray Input array.
   * @param binRangesArray Bin-range output array.
   * @param args Forwarded worker arguments.
   * @return Typed resident histogram worker.
   */
  template <typename T, class... ArgsT>
  auto operator()(const IDataArray* inputArray, IDataArray* binRangesArray, ArgsT&&... args)
  {
    return GenerateFeatureHistogramImpl(inputArray->template getIDataStoreRefAs<AbstractDataStore<T>>(), binRangesArray->template getIDataStoreRefAs<AbstractDataStore<T>>(),
                                        std::forward<ArgsT>(args)...);
  }
};

namespace
{
constexpr usize k_HistogramChunkTuples = 65536; // Bounds each scanline input page.

/**
 * @brief Multiplies two allocation dimensions without usize overflow.
 * @param lhs First dimension.
 * @param rhs Second dimension.
 * @param product Receives the product when representable.
 * @return True when the product fits usize.
 */
bool checkedMultiply(usize lhs, usize rhs, usize& product)
{
  if(lhs != 0 && rhs > std::numeric_limits<usize>::max() / lhs)
  {
    return false;
  }
  product = lhs * rhs;
  return true;
}

template <typename T>
constexpr uint64 k_ModalRecordSize = sizeof(int32) + sizeof(T) + sizeof(uint64); // Feature, value, and tuple identifier bytes.

/**
 * @brief Serializes a modal candidate as (feature ID, value, original tuple).
 * @tparam T Modal value type.
 * @param bytes Destination record bytes.
 * @param featureId Feature identifier.
 * @param value Modal candidate value.
 * @param originalTupleIndex Source tuple identifier.
 *
 * The tuple identifier provides deterministic ordering for equal values.
 */
template <typename T>
void encodeModalRecord(nonstd::span<std::byte> bytes, int32 featureId, T value, uint64 originalTupleIndex)
{
  std::memcpy(bytes.data(), &featureId, sizeof(featureId));
  std::memcpy(bytes.data() + sizeof(featureId), &value, sizeof(value));
  std::memcpy(bytes.data() + sizeof(featureId) + sizeof(value), &originalTupleIndex, sizeof(originalTupleIndex));
}

/**
 * @brief Deserializes one modal candidate record.
 * @tparam T Modal value type.
 * @param bytes Source record bytes.
 * @param featureId Receives the feature identifier.
 * @param value Receives the modal candidate value.
 * @param originalTupleIndex Receives the source tuple identifier.
 */
template <typename T>
void decodeModalRecord(nonstd::span<const std::byte> bytes, int32& featureId, T& value, uint64& originalTupleIndex)
{
  std::memcpy(&featureId, bytes.data(), sizeof(featureId));
  std::memcpy(&value, bytes.data() + sizeof(featureId), sizeof(value));
  std::memcpy(&originalTupleIndex, bytes.data() + sizeof(featureId) + sizeof(value), sizeof(originalTupleIndex));
}

/**
 * @brief Orders modal records by feature, value, and tuple identifier.
 * @tparam T Modal value type.
 * @param left First serialized modal record.
 * @param right Second serialized modal record.
 * @return Negative, zero, or positive lexical comparison result.
 *
 * Equal values become contiguous while the tuple identifier keeps the order deterministic.
 */
template <typename T>
int32 compareModalRecords(nonstd::span<const std::byte> left, nonstd::span<const std::byte> right)
{
  int32 leftFeature = 0;
  int32 rightFeature = 0;
  T leftValue{};
  T rightValue{};
  uint64 leftIndex = 0;
  uint64 rightIndex = 0;
  decodeModalRecord(left, leftFeature, leftValue, leftIndex);
  decodeModalRecord(right, rightFeature, rightValue, rightIndex);
  if(leftFeature != rightFeature)
  {
    return leftFeature < rightFeature ? -1 : 1;
  }
  if(leftValue < rightValue)
  {
    return -1;
  }
  if(rightValue < leftValue)
  {
    return 1;
  }
  if(leftIndex == rightIndex)
  {
    return 0;
  }
  return leftIndex < rightIndex ? -1 : 1;
}

/**
 * @brief Tests value equality with only operator<.
 * @tparam T Numeric value type.
 * @param left First value.
 * @param right Second value.
 * @return True when neither value is less than the other.
 */
template <typename T>
bool equivalentModalValues(const T& left, const T& right)
{
  return !(left < right) && !(right < left);
}

/**
 * @brief Tests the histogram half-open range.
 * @tparam T Numeric value type.
 * @param value Value to test.
 * @param minimum Inclusive lower bound.
 * @param maximum Exclusive upper bound.
 * @return True when value is in [minimum, maximum).
 */
template <typename T>
bool isInHistogramRange(const T& value, const T& minimum, const T& maximum)
{
  return !(value < minimum) && value < maximum;
}

/**
 * @brief Calculates a bin index without overflowing signed integral subtraction.
 * @tparam T Numeric value type.
 * @param value Value to bin.
 * @param minimum Histogram minimum.
 * @param increment Bin width.
 * @return Calculated bin index.
 *
 * The floating-point path handles signed differences that T cannot represent.
 */
template <typename T>
int32 calculateSafeBin(const T& value, const T& minimum, float32 increment)
{
  if constexpr(std::is_signed_v<T> && std::is_integral_v<T>)
  {
    if(value > 0 && minimum < 0 && value > std::numeric_limits<T>::max() + minimum)
    {
      return static_cast<int32>(std::floor((static_cast<float32>(value) - static_cast<float32>(minimum)) / increment));
    }
  }
  return static_cast<int32>(HistogramUtilities::serial::CalculateBin(value, minimum, increment));
}

/**
 * @brief Appends the bin-range pair associated with one modal value.
 * @tparam T Modal value type.
 * @param modalBinRanges Receives modal bin-range pairs.
 * @param feature Feature identifier.
 * @param value Modal value.
 * @param ranges Flat bin-range values.
 * @param bins Number of bins per feature.
 * @param minimum Inclusive histogram lower bound.
 * @param maximum Exclusive histogram upper bound.
 * @param increment Bin width.
 *
 * Values outside the histogram range are omitted. The Direct-compatible layout
 * indexes the feature's flat range buffer by bin.
 */
template <typename T>
void appendModalRange(NeighborList<T>& modalBinRanges, usize feature, const T& value, const T* ranges, usize bins, const T& minimum, const T& maximum, float32 increment)
{
  if(!isInHistogramRange(value, minimum, maximum))
  {
    return;
  }
  const int32 bin = calculateSafeBin(value, minimum, increment);
  if(bin >= 0 && static_cast<usize>(bin) < bins)
  {
    const usize rangeOffset = feature * bins * 2 + static_cast<usize>(bin);
    modalBinRanges.addEntry(static_cast<int32>(feature), ranges[rangeOffset]);
    modalBinRanges.addEntry(static_cast<int32>(feature), ranges[rangeOffset + 1]);
  }
}

/**
 * @brief Streams sorted modal records and reports each feature/value run.
 * @tparam T Modal value type.
 * @tparam GroupFunction Callable that accepts feature, value, and count.
 * @param externalSort Finished modal-record sorter.
 * @param shouldCancel Cancellation flag.
 * @param groupFunction Receives each contiguous modal-value run.
 * @return Sort I/O or cancellation result.
 *
 * Reading in fixed record pages bounds memory while the external sorter keeps
 * exact modal grouping from requiring the full input in RAM.
 */
template <typename T, typename GroupFunction>
Result<> scanSortedModalRecords(const IExternalSort& externalSort, const std::atomic_bool& shouldCancel, GroupFunction&& groupFunction)
{
  constexpr uint64 k_RecordsPerRead = k_HistogramChunkTuples;
  std::vector<std::byte> bytes(static_cast<usize>(k_RecordsPerRead * k_ModalRecordSize<T>));
  std::optional<int32> currentFeature;
  std::optional<T> currentValue;
  uint64 currentCount = 0;
  const uint64 totalRecords = externalSort.recordCount();
  for(uint64 offset = 0; offset < totalRecords; offset += k_RecordsPerRead)
  {
    if(shouldCancel)
    {
      return {};
    }
    const uint64 count = std::min(k_RecordsPerRead, totalRecords - offset);
    Result<uint64> readResult = externalSort.read(offset, count, nonstd::span<std::byte>(bytes.data(), static_cast<usize>(count * k_ModalRecordSize<T>)), shouldCancel);
    if(readResult.invalid())
    {
      return ConvertResult(std::move(readResult));
    }
    if(readResult.value() != count)
    {
      return MakeErrorResult(
          -23811, fmt::format("ComputeArrayHistogramByFeature: external modal sort short read at record offset {}: requested {} records but received {}.", offset, count, readResult.value()));
    }
    for(uint64 index = 0; index < count; ++index)
    {
      int32 feature = 0;
      T value{};
      uint64 originalIndex = 0;
      const usize byteOffset = static_cast<usize>(index * k_ModalRecordSize<T>);
      decodeModalRecord(nonstd::span<const std::byte>(bytes.data() + byteOffset, k_ModalRecordSize<T>), feature, value, originalIndex);
      if(currentFeature.has_value() && *currentFeature == feature && equivalentModalValues(*currentValue, value))
      {
        if(currentCount == std::numeric_limits<uint64>::max())
        {
          return MakeErrorResult(-23812, fmt::format("ComputeArrayHistogramByFeature: modal-value count for FeatureId {} and value {} exceeds uint64.", *currentFeature, *currentValue));
        }
        ++currentCount;
      }
      else
      {
        if(currentFeature.has_value())
        {
          groupFunction(*currentFeature, *currentValue, currentCount);
        }
        currentFeature = feature;
        currentValue = value;
        currentCount = 1;
      }
    }
  }
  if(currentFeature.has_value())
  {
    groupFunction(*currentFeature, *currentValue, currentCount);
  }
  return {};
}

/**
 * @brief Exact provider-free modal fallback for one feature.
 * @tparam T Input value type.
 * @tparam MaskT Mask value type.
 * @tparam GroupFunction Callable that accepts modal value and count.
 * @param inputStore Input values.
 * @param featureIdsStore Maps tuples to features.
 * @param maskStore Optional mask store.
 * @param tupleCount Input tuple count.
 * @param featureId Feature to scan.
 * @param shouldCancel Cancellation flag.
 * @param groupFunction Receives each distinct value and count.
 * @return Bulk-I/O, count-overflow, or cancellation result.
 *
 * Repeated bounded scans select the next distinct value and count it. This can
 * be slower than external sorting, but it preserves exact results without an
 * input-sized scratch allocation when no external-sort provider is registered.
 */
template <typename T, typename MaskT, typename GroupFunction>
Result<> scanFallbackModalGroups(const AbstractDataStore<T>& inputStore, const AbstractDataStore<int32>& featureIdsStore, const AbstractDataStore<MaskT>* maskStore, usize tupleCount, int32 featureId,
                                 const std::atomic_bool& shouldCancel, GroupFunction&& groupFunction)
{
  auto values = std::make_unique<T[]>(k_HistogramChunkTuples);
  std::vector<int32> features(k_HistogramChunkTuples);
  auto masks = maskStore == nullptr ? nullptr : std::make_unique<MaskT[]>(k_HistogramChunkTuples);
  const auto scan = [&](auto&& valueFunction) -> Result<> {
    for(usize offset = 0; offset < tupleCount; offset += k_HistogramChunkTuples)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(k_HistogramChunkTuples, tupleCount - offset);
      Result<> result = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(features.data(), count));
      if(result.invalid())
      {
        return result;
      }
      result = inputStore.copyIntoBuffer(offset, nonstd::span<T>(values.get(), count));
      if(result.invalid())
      {
        return result;
      }
      if(maskStore != nullptr)
      {
        result = maskStore->copyIntoBuffer(offset, nonstd::span<MaskT>(masks.get(), count));
        if(result.invalid())
        {
          return result;
        }
      }
      for(usize index = 0; index < count; ++index)
      {
        if(features[index] == featureId && (maskStore == nullptr || static_cast<bool>(masks[index])))
        {
          valueFunction(values[index]);
        }
      }
    }
    return {};
  };

  std::optional<T> previous;
  while(!shouldCancel)
  {
    std::optional<T> next;
    Result<> result = scan([&](const T& value) {
      if((!previous.has_value() || *previous < value) && (!next.has_value() || value < *next))
      {
        next = value;
      }
    });
    if(result.invalid() || shouldCancel)
    {
      return result;
    }
    if(!next.has_value())
    {
      break;
    }
    uint64 count = 0;
    bool countOverflow = false;
    result = scan([&](const T& value) {
      if(equivalentModalValues(value, *next))
      {
        if(count == std::numeric_limits<uint64>::max())
        {
          countOverflow = true;
          return;
        }
        ++count;
      }
    });
    if(result.invalid() || shouldCancel)
    {
      return result;
    }
    if(countOverflow)
    {
      return MakeErrorResult(-23812, fmt::format("ComputeArrayHistogramByFeature: modal-value count for FeatureId {} and value {} exceeds uint64.", featureId, *next));
    }
    groupFunction(*next, count);
    previous = next;
  }
  return {};
}

/**
 * @brief Computes one bounded histogram for a type-dispatched input array.
 * @tparam T Input and bin-range value type.
 * @tparam MaskT Mask value type.
 * @param inputArray Input array.
 * @param binRangesArray Receives bin ranges.
 * @param featureIdsStore Maps tuples to features.
 * @param maskStore Optional mask store.
 * @param countsArray Receives histogram counts.
 * @param mostPopulatedArray Receives most-populated bin data.
 * @param modalBinRanges Optional modal-range list.
 * @param inputValues Histogram parameters.
 * @param numFeatures Number of discovered features.
 * @param overflow Counts values outside histogram ranges.
 * @param shouldCancel Cancellation flag.
 * @return Validation, bulk-I/O, sort, overflow, or cancellation result.
 *
 * The algorithm separates discovery, bin construction, counting, modal reduction,
 * and output writes. Cell scans use fixed pages. Feature-scale arrays depend on
 * feature and bin counts, not input tuple count. Exact modal values use external
 * sorting when available or repeated bounded scans otherwise. Cancellation before
 * final bulk writes leaves feature-scale output contents unwritten.
 */
template <typename T, typename MaskT>
Result<> generateScanlineHistogram(const IDataArray& inputArray, IDataArray& binRangesArray, const AbstractDataStore<int32>& featureIdsStore, const AbstractDataStore<MaskT>* maskStore,
                                   DataArray<uint64>& countsArray, DataArray<uint64>& mostPopulatedArray, INeighborList* modalBinRanges, const ComputeArrayHistogramByFeatureInputValues& inputValues,
                                   usize numFeatures, std::atomic<usize>& overflow, const std::atomic_bool& shouldCancel)
{
  if constexpr(std::is_same_v<T, bool>)
  {
    if(modalBinRanges != nullptr)
    {
      return MakeErrorResult(-23813,
                             fmt::format("ComputeArrayHistogramByFeature: Boolean input array '{}' cannot produce modal NeighborList output '{}'.", inputArray.getName(), modalBinRanges->getName()));
    }
  }
  else if(modalBinRanges != nullptr && dynamic_cast<NeighborList<T>*>(modalBinRanges) == nullptr)
  {
    return MakeErrorResult(-23813, fmt::format("ComputeArrayHistogramByFeature: modal NeighborList '{}' is incompatible with input array '{}' and cannot store its modal bin ranges.",
                                               modalBinRanges->getName(), inputArray.getName()));
  }
  const int32 numBins = inputValues.NumberOfBins;
  if(numBins <= 0)
  {
    return MakeErrorResult(-23803, fmt::format("ComputeArrayHistogramByFeature: NumberOfBins ({}) must be greater than zero for input array '{}'.", numBins, inputArray.getName()));
  }
  const usize bins = static_cast<usize>(numBins);
  usize featureBins = 0;
  usize rangeValues = 0;
  if(!checkedMultiply(numFeatures, bins, featureBins) || !checkedMultiply(featureBins, 2, rangeValues))
  {
    return MakeErrorResult(
        -23804, fmt::format("ComputeArrayHistogramByFeature: output shape for input array '{}' overflows the platform size type ({} features, {} bins).", inputArray.getName(), numFeatures, bins));
  }

  binRangesArray.resizeTuples({numFeatures});
  countsArray.resizeTuples({numFeatures});
  mostPopulatedArray.resizeTuples({numFeatures});

  const auto& inputStore = inputArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
  auto& binRangesStore = binRangesArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
  auto& countsStore = countsArray.getDataStoreRef();
  auto& mostPopulatedStore = mostPopulatedArray.getDataStoreRef();
  const usize tupleCount = featureIdsStore.getNumberOfTuples();
  if(inputStore.getNumberOfTuples() != tupleCount || (maskStore != nullptr && maskStore->getNumberOfTuples() != tupleCount))
  {
    return MakeErrorResult(-23805, fmt::format("ComputeArrayHistogramByFeature: input array '{}' has {} tuples, FeatureIds has {} tuples, and mask {} has {} tuples. These tuple counts must match.",
                                               inputArray.getName(), inputStore.getNumberOfTuples(), tupleCount, maskStore == nullptr ? "is disabled" : "array",
                                               maskStore == nullptr ? 0 : maskStore->getNumberOfTuples()));
  }

  std::vector<uint64> lengths(numFeatures, 0);
  auto minimums = std::make_unique<T[]>(numFeatures);
  auto maximums = std::make_unique<T[]>(numFeatures);
  std::vector<uint64> counts(featureBins, 0);
  auto ranges = std::make_unique<T[]>(rangeValues);
  std::fill_n(ranges.get(), rangeValues, T{});
  auto valueBuffer = std::make_unique<T[]>(k_HistogramChunkTuples);
  std::vector<int32> featureBuffer(k_HistogramChunkTuples);
  auto maskBuffer = maskStore == nullptr ? nullptr : std::make_unique<MaskT[]>(k_HistogramChunkTuples);

  std::unique_ptr<IExternalSort> externalSort;
  std::vector<std::byte> modalRecordBytes;
  if constexpr(!std::is_same_v<T, bool>)
  {
    if(modalBinRanges != nullptr && DataStoreUtilities::GetIOCollection().hasExternalSortCapability())
    {
      ExternalSortConfig config;
      config.recordSize = k_ModalRecordSize<T>;
      config.maxRecordsPerBatch = k_HistogramChunkTuples;
      config.compare = compareModalRecords<T>;
      Result<std::unique_ptr<IExternalSort>> sortResult = DataStoreUtilities::GetIOCollection().createExternalSort(config);
      if(sortResult.invalid())
      {
        return ConvertResult(std::move(sortResult));
      }
      externalSort = std::move(sortResult.value());
      modalRecordBytes.resize(k_HistogramChunkTuples * k_ModalRecordSize<T>);
    }
  }

  const auto readChunk = [&](usize offset, usize count) -> Result<> {
    auto result = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(featureBuffer.data(), count));
    if(result.invalid())
    {
      return result;
    }
    result = inputStore.copyIntoBuffer(offset, nonstd::span<T>(valueBuffer.get(), count));
    if(result.invalid())
    {
      return result;
    }
    if(maskStore != nullptr)
    {
      result = maskStore->copyIntoBuffer(offset, nonstd::span<MaskT>(maskBuffer.get(), count));
    }
    return result;
  };

  // Discover each feature's sample count and extrema before range construction.
  for(usize offset = 0; offset < tupleCount; offset += k_HistogramChunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_HistogramChunkTuples, tupleCount - offset);
    auto readResult = readChunk(offset, count);
    if(readResult.invalid())
    {
      return readResult;
    }
    for(usize index = 0; index < count; ++index)
    {
      const int32 featureId = featureBuffer[index];
      if(featureId < 0 || (maskStore != nullptr && !static_cast<bool>(maskBuffer[index])))
      {
        continue;
      }
      const usize feature = static_cast<usize>(featureId);
      if(feature >= numFeatures)
      {
        return MakeErrorResult(-23806, fmt::format("ComputeArrayHistogramByFeature: FeatureId {} at tuple {} in input array '{}' exceeds the discovered feature count {}.", featureId, offset + index,
                                                   inputArray.getName(), numFeatures));
      }
      const T value = valueBuffer[index];
      if(lengths[feature] == 0)
      {
        minimums[feature] = value;
        maximums[feature] = value;
      }
      else
      {
        minimums[feature] = std::min(static_cast<T>(minimums[feature]), value);
        maximums[feature] = std::max(static_cast<T>(maximums[feature]), value);
      }
      if(lengths[feature] == std::numeric_limits<uint64>::max())
      {
        return MakeErrorResult(-23807, fmt::format("ComputeArrayHistogramByFeature: sample count for FeatureId {} in input array '{}' exceeds uint64.", feature, inputArray.getName()));
      }
      ++lengths[feature];
    }
  }

  // Build per-feature bin geometry once, before the counting pass. Feature-scale
  // state stays resident because it is independent of the potentially huge cell count.
  auto histogramMinimums = std::make_unique<T[]>(numFeatures);
  std::vector<float32> increments(numFeatures, 0.0F);
  for(usize feature = 0; feature < numFeatures; ++feature)
  {
    if(shouldCancel)
    {
      return {};
    }
    if(lengths[feature] == 0)
    {
      continue;
    }
    T histMin = static_cast<T>(inputValues.MinRange);
    T histMax = static_cast<T>(inputValues.MaxRange);
    if(!inputValues.UserDefinedRange)
    {
      histMin = minimums[feature];
      if constexpr(std::is_integral_v<T> && !std::is_same_v<T, bool>)
      {
        if(maximums[feature] == std::numeric_limits<T>::max())
        {
          return MakeErrorResult(-23808,
                                 fmt::format("ComputeArrayHistogramByFeature: full-range maximum {} for FeatureId {} in input array '{}' cannot be incremented without overflowing the input type.",
                                             maximums[feature], feature, inputArray.getName()));
        }
      }
      histMax = maximums[feature] + static_cast<T>(1);
    }
    if constexpr(std::is_signed_v<T> && std::is_integral_v<T>)
    {
      if(histMax < histMin || (histMin < 0 && histMax > std::numeric_limits<T>::max() + histMin))
      {
        return MakeErrorResult(-23810, fmt::format("ComputeArrayHistogramByFeature: histogram range [{}, {}) for FeatureId {} in input array '{}' cannot be represented without signed overflow.",
                                                   histMin, histMax, feature, inputArray.getName()));
      }
    }
    if constexpr(std::is_unsigned_v<T> && !std::is_same_v<T, bool>)
    {
      if(histMax < histMin)
      {
        return MakeErrorResult(-23810, fmt::format("ComputeArrayHistogramByFeature: histogram maximum {} is smaller than minimum {} for FeatureId {} in input array '{}'.", histMax, histMin, feature,
                                                   inputArray.getName()));
      }
    }
    const float32 increment = HistogramUtilities::serial::CalculateIncrement(histMin, histMax, numBins);
    histogramMinimums[feature] = histMin;
    increments[feature] = increment;
    auto range = nonstd::span<T>(ranges.get() + feature * bins * 2, bins * 2);
    HistogramUtilities::serial::FillBinRanges(range, std::make_pair(histMin, histMax), numBins, increment);
    if(std::fabs(increment) < 1.0E-10F)
    {
      counts[feature * bins] = lengths[feature];
    }
  }

  // Route each accepted tuple to its feature/bin and optionally append a modal record.
  for(usize offset = 0; offset < tupleCount; offset += k_HistogramChunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_HistogramChunkTuples, tupleCount - offset);
    auto readResult = readChunk(offset, count);
    if(readResult.invalid())
    {
      return readResult;
    }
    for(usize index = 0; index < count; ++index)
    {
      const int32 featureId = featureBuffer[index];
      if(featureId < 0 || (maskStore != nullptr && !static_cast<bool>(maskBuffer[index])))
      {
        continue;
      }
      const usize feature = static_cast<usize>(featureId);
      const float32 increment = increments[feature];
      const T& value = valueBuffer[index];
      if constexpr(!std::is_same_v<T, bool>)
      {
        if(externalSort != nullptr && modalBinRanges != nullptr)
        {
          encodeModalRecord<T>(nonstd::span<std::byte>(modalRecordBytes.data() + index * k_ModalRecordSize<T>, k_ModalRecordSize<T>), featureId, value, static_cast<uint64>(offset + index));
        }
      }
      if(std::fabs(increment) < 1.0E-10F)
      {
        continue;
      }
      if(!isInHistogramRange(value, histogramMinimums[feature], inputValues.UserDefinedRange ? static_cast<T>(inputValues.MaxRange) : static_cast<T>(maximums[feature] + static_cast<T>(1))))
      {
        ++overflow;
        continue;
      }
      const auto bin = calculateSafeBin(value, histogramMinimums[feature], increment);
      if(bin >= 0 && bin < numBins)
      {
        uint64& binCount = counts[feature * bins + static_cast<usize>(bin)];
        if(binCount == std::numeric_limits<uint64>::max())
        {
          return MakeErrorResult(-23809,
                                 fmt::format("ComputeArrayHistogramByFeature: histogram count for FeatureId {}, bin {}, in input array '{}' exceeds uint64.", feature, bin, inputArray.getName()));
        }
        ++binCount;
      }
      else
      {
        ++overflow;
      }
    }
    if(externalSort != nullptr)
    {
      uint64 recordCount = 0;
      for(usize index = 0; index < count; ++index)
      {
        const int32 featureId = featureBuffer[index];
        if(featureId >= 0 && (maskStore == nullptr || static_cast<bool>(maskBuffer[index])))
        {
          const usize sourceOffset = index * k_ModalRecordSize<T>;
          const usize destinationOffset = static_cast<usize>(recordCount) * k_ModalRecordSize<T>;
          if(sourceOffset != destinationOffset)
          {
            std::memcpy(modalRecordBytes.data() + destinationOffset, modalRecordBytes.data() + sourceOffset, k_ModalRecordSize<T>);
          }
          ++recordCount;
        }
      }
      if(recordCount > 0)
      {
        Result<> appendResult = externalSort->append(recordCount, nonstd::span<const std::byte>(modalRecordBytes.data(), static_cast<usize>(recordCount * k_ModalRecordSize<T>)), shouldCancel, {});
        if(appendResult.invalid())
        {
          return appendResult;
        }
      }
    }
  }

  // Finish all sort runs before scanning them; IExternalSort does not expose a
  // globally ordered read view until finish() succeeds.
  if(externalSort != nullptr)
  {
    Result<> finishResult = externalSort->finish(shouldCancel, {});
    if(finishResult.invalid() || shouldCancel)
    {
      return finishResult;
    }
  }

  std::vector<uint64> mostPopulated(numFeatures * 2, 0);
  for(usize feature = 0; feature < numFeatures; ++feature)
  {
    const auto first = counts.begin() + static_cast<ptrdiff_t>(feature * bins);
    const auto highest = std::max_element(first, first + static_cast<ptrdiff_t>(bins));
    mostPopulated[feature * 2] = static_cast<uint64>(std::distance(first, highest));
    mostPopulated[feature * 2 + 1] = *highest;
  }

  // Modal reduction follows counting because it needs globally grouped values.
  if constexpr(!std::is_same_v<T, bool>)
  {
    if(modalBinRanges != nullptr)
    {
      auto& typedModalBinRanges = *dynamic_cast<NeighborList<T>*>(modalBinRanges);
      typedModalBinRanges.resizeTuples({numFeatures});
      for(usize feature = 0; feature < numFeatures; ++feature)
      {
        if(lengths[feature] > 0 && std::fabs(increments[feature]) < 1.0E-10F)
        {
          // Degenerate modal ranges use [0, numFeatures) for every populated feature.
          typedModalBinRanges.addEntry(static_cast<int32>(feature), static_cast<T>(0));
          typedModalBinRanges.addEntry(static_cast<int32>(feature), static_cast<T>(numFeatures));
        }
      }
      if(externalSort != nullptr)
      {
        std::vector<uint64> maxCounts(numFeatures, 0);
        Result<> scanResult = scanSortedModalRecords<T>(*externalSort, shouldCancel, [&](int32 featureId, T, uint64 count) {
          if(featureId >= 0 && static_cast<usize>(featureId) < numFeatures)
          {
            maxCounts[static_cast<usize>(featureId)] = std::max(maxCounts[static_cast<usize>(featureId)], count);
          }
        });
        if(scanResult.invalid() || shouldCancel)
        {
          return scanResult;
        }
        scanResult = scanSortedModalRecords<T>(*externalSort, shouldCancel, [&](int32 featureId, T value, uint64 count) {
          const usize feature = static_cast<usize>(featureId);
          if(featureId >= 0 && feature < numFeatures && std::fabs(increments[feature]) >= 1.0E-10F && count == maxCounts[feature])
          {
            const T maximum = inputValues.UserDefinedRange ? static_cast<T>(inputValues.MaxRange) : static_cast<T>(maximums[feature] + static_cast<T>(1));
            appendModalRange(typedModalBinRanges, feature, value, ranges.get(), bins, histogramMinimums[feature], maximum, increments[feature]);
          }
        });
        if(scanResult.invalid() || shouldCancel)
        {
          return scanResult;
        }
      }
      else
      {
        for(usize feature = 0; feature < numFeatures; ++feature)
        {
          if(lengths[feature] == 0 || std::fabs(increments[feature]) < 1.0E-10F)
          {
            continue;
          }
          uint64 maxCount = 0;
          Result<> scanResult =
              scanFallbackModalGroups(inputStore, featureIdsStore, maskStore, tupleCount, static_cast<int32>(feature), shouldCancel, [&](T, uint64 count) { maxCount = std::max(maxCount, count); });
          if(scanResult.invalid() || shouldCancel)
          {
            return scanResult;
          }
          scanResult = scanFallbackModalGroups(inputStore, featureIdsStore, maskStore, tupleCount, static_cast<int32>(feature), shouldCancel, [&](T value, uint64 count) {
            if(count == maxCount)
            {
              const T maximum = inputValues.UserDefinedRange ? static_cast<T>(inputValues.MaxRange) : static_cast<T>(maximums[feature] + static_cast<T>(1));
              appendModalRange(typedModalBinRanges, feature, value, ranges.get(), bins, histogramMinimums[feature], maximum, increments[feature]);
            }
          });
          if(scanResult.invalid() || shouldCancel)
          {
            return scanResult;
          }
        }
      }
    }
  }
  // Commit feature-scale outputs in bulk to avoid one disk transaction per bin.
  auto writeResult = binRangesStore.copyFromBuffer(0, nonstd::span<const T>(ranges.get(), rangeValues));
  if(writeResult.invalid())
  {
    return writeResult;
  }
  writeResult = countsStore.copyFromBuffer(0, nonstd::span<const uint64>(counts.data(), counts.size()));
  if(writeResult.invalid())
  {
    return writeResult;
  }
  return mostPopulatedStore.copyFromBuffer(0, nonstd::span<const uint64>(mostPopulated.data(), mostPopulated.size()));
}

/**
 * @struct HistogramScanlineFunctor
 * @brief Dispatches bounded histograms by input and mask type.
 */
struct HistogramScanlineFunctor
{
  /**
   * @brief Selects Boolean, UInt8, or absent mask storage for a bounded scan.
   * @tparam T Input and bin-range value type.
   * @param inputArray Input array.
   * @param binRangesArray Receives bin ranges.
   * @param featureIdsStore Maps tuples to features.
   * @param maskArray Optional mask array.
   * @param countsArray Receives histogram counts.
   * @param mostPopulatedArray Receives most-populated bin data.
   * @param modalBinRanges Optional modal-range list.
   * @param inputValues Histogram parameters.
   * @param numFeatures Number of discovered features.
   * @param overflow Counts values outside histogram ranges.
   * @param shouldCancel Cancellation flag.
   * @return Result from the typed bounded scan.
   */
  template <typename T>
  Result<> operator()(const IDataArray& inputArray, IDataArray& binRangesArray, const AbstractDataStore<int32>& featureIdsStore, const IDataArray* maskArray, DataArray<uint64>& countsArray,
                      DataArray<uint64>& mostPopulatedArray, INeighborList* modalBinRanges, const ComputeArrayHistogramByFeatureInputValues& inputValues, usize numFeatures,
                      std::atomic<usize>& overflow, const std::atomic_bool& shouldCancel) const
  {
    if(maskArray == nullptr)
    {
      return generateScanlineHistogram<T, uint8>(inputArray, binRangesArray, featureIdsStore, nullptr, countsArray, mostPopulatedArray, modalBinRanges, inputValues, numFeatures, overflow,
                                                 shouldCancel);
    }
    if(maskArray->getDataType() == DataType::boolean)
    {
      return generateScanlineHistogram<T, bool>(inputArray, binRangesArray, featureIdsStore, &maskArray->template getIDataStoreRefAs<AbstractDataStore<bool>>(), countsArray, mostPopulatedArray,
                                                modalBinRanges, inputValues, numFeatures, overflow, shouldCancel);
    }
    return generateScanlineHistogram<T, uint8>(inputArray, binRangesArray, featureIdsStore, &maskArray->template getIDataStoreRefAs<AbstractDataStore<uint8>>(), countsArray, mostPopulatedArray,
                                               modalBinRanges, inputValues, numFeatures, overflow, shouldCancel);
  }
};

/**
 * @class ComputeArrayHistogramByFeatureDirect
 * @brief Invokes the existing resident histogram implementation.
 */
class ComputeArrayHistogramByFeatureDirect
{
public:
  /**
   * @brief Stores the direct callback for synchronous dispatch.
   * @tparam ArgsT Additional dispatch argument types.
   * @param executeDirect Resident implementation callback.
   * @param args Forwarded arguments ignored by this wrapper.
   * @pre executeDirect outlives this wrapper.
   */
  template <typename... ArgsT>
  explicit ComputeArrayHistogramByFeatureDirect(const std::function<Result<>()>& executeDirect, ArgsT&&... args)
  : m_ExecuteDirect(executeDirect)
  {
  }

  /**
   * @brief Executes the resident implementation callback.
   * @return Callback result.
   */
  Result<> operator()() const
  {
    return m_ExecuteDirect();
  }

private:
  const std::function<Result<>()>& m_ExecuteDirect;
};

/**
 * @class ComputeArrayHistogramByFeatureScanline
 * @brief Stores borrowed inputs for the bounded histogram implementation.
 *
 * References remain owned by ComputeArrayHistogramByFeature and are used synchronously.
 */
class ComputeArrayHistogramByFeatureScanline
{
public:
  /**
   * @brief Stores inputs and outputs for one bounded typed scan.
   * @param executeDirect Resident callback ignored by the scanline wrapper.
   * @param inputArray Input array.
   * @param binRangesArray Receives bin ranges.
   * @param featureIdsStore Maps tuples to features.
   * @param maskArray Optional mask array.
   * @param countsArray Receives histogram counts.
   * @param mostPopulatedArray Receives most-populated bin data.
   * @param modalBinRanges Optional modal-range list.
   * @param inputValues Histogram parameters.
   * @param numFeatures Number of discovered features.
   * @param overflow Counts values outside histogram ranges.
   * @param shouldCancel Cancellation flag.
   * @pre Referenced arrays, stores, inputValues, overflow, and shouldCancel outlive this wrapper.
   */
  ComputeArrayHistogramByFeatureScanline(const std::function<Result<>()>& executeDirect, const IDataArray& inputArray, IDataArray& binRangesArray, const AbstractDataStore<int32>& featureIdsStore,
                                         const IDataArray* maskArray, DataArray<uint64>& countsArray, DataArray<uint64>& mostPopulatedArray, INeighborList* modalBinRanges,
                                         const ComputeArrayHistogramByFeatureInputValues& inputValues, usize numFeatures, std::atomic<usize>& overflow, const std::atomic_bool& shouldCancel)
  : m_InputArray(inputArray)
  , m_BinRangesArray(binRangesArray)
  , m_FeatureIdsStore(featureIdsStore)
  , m_MaskArray(maskArray)
  , m_CountsArray(countsArray)
  , m_MostPopulatedArray(mostPopulatedArray)
  , m_ModalBinRanges(modalBinRanges)
  , m_InputValues(inputValues)
  , m_NumFeatures(numFeatures)
  , m_Overflow(overflow)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Runtime-dispatches the bounded implementation by input value type.
   * @return Result from the bounded typed scan.
   */
  Result<> operator()() const
  {
    return ExecuteDataFunction(HistogramScanlineFunctor{}, m_InputArray.getDataType(), m_InputArray, m_BinRangesArray, m_FeatureIdsStore, m_MaskArray, m_CountsArray, m_MostPopulatedArray,
                               m_ModalBinRanges, m_InputValues, m_NumFeatures, m_Overflow, m_ShouldCancel);
  }

private:
  const IDataArray& m_InputArray;
  IDataArray& m_BinRangesArray;
  const AbstractDataStore<int32>& m_FeatureIdsStore;
  const IDataArray* m_MaskArray = nullptr;
  DataArray<uint64>& m_CountsArray;
  DataArray<uint64>& m_MostPopulatedArray;
  INeighborList* m_ModalBinRanges = nullptr;
  const ComputeArrayHistogramByFeatureInputValues& m_InputValues;
  usize m_NumFeatures = 0;
  std::atomic<usize>& m_Overflow;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

ComputeArrayHistogramByFeature::ComputeArrayHistogramByFeature(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                                               ComputeArrayHistogramByFeatureInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

ComputeArrayHistogramByFeature::~ComputeArrayHistogramByFeature() noexcept = default;

Result<> ComputeArrayHistogramByFeature::operator()()
{
  const int32 numBins = m_InputValues->NumberOfBins;
  const std::vector<DataPath> selectedArrayPaths = m_InputValues->SelectedArrayPaths;
  std::atomic<usize> overflow = 0;

  const auto& featureIdsArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStore = featureIdsArray.getDataStoreRef();

  // Discover the feature count before output changes so entry cancellation preserves existing output values.
  Result<usize> featureCountResult = findFeatureCount(featureIdsStore, m_ShouldCancel);
  if(featureCountResult.invalid())
  {
    return ConvertResult(std::move(featureCountResult));
  }
  if(m_ShouldCancel)
  {
    return {};
  }
  const usize numFeatures = featureCountResult.value();

  MessageHelper messageHelper(m_MessageHandler);

  for(int32 i = 0; i < selectedArrayPaths.size(); i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto* inputData = m_DataStructure.getDataAs<IDataArray>(selectedArrayPaths[i]);
    const IDataArray* maskArray = m_InputValues->UseMask ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath) : nullptr;
    auto* binRanges = m_DataStructure.getDataAs<IDataArray>(m_InputValues->CreatedBinRangeDataPaths.at(i));
    auto* countsArray = m_DataStructure.getDataAs<DataArray<uint64>>(m_InputValues->CreatedHistogramCountsDataPaths.at(i));
    auto* mostPopulatedArray = m_DataStructure.getDataAs<DataArray<uint64>>(m_InputValues->CreatedBinMostPopulatedDataPaths.at(i));
    auto& counts = countsArray->getDataStoreRef();
    auto& mostPopulated = mostPopulatedArray->getDataStoreRef();
    INeighborList* modalBinRanges = nullptr;
    if(m_InputValues->CreatedBinModalRangesDataPaths.has_value())
    {
      modalBinRanges = m_DataStructure.getDataAs<INeighborList>(m_InputValues->CreatedBinModalRangesDataPaths->at(i));
    }

    const std::function<Result<>()> executeDirect = [&]() -> Result<> {
      binRanges->resizeTuples({numFeatures});
      counts.resizeTuples({numFeatures});
      mostPopulated.resizeTuples({numFeatures});

      std::unique_ptr<MaskCompareUtilities::MaskCompare> mask = nullptr;
      if(m_InputValues->UseMask)
      {
        mask = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
      }

      ParallelDataAlgorithm dataAlg;
      dataAlg.setRange(0, numFeatures);
      const bool histFullRange = !m_InputValues->UserDefinedRange;
      ProgressMessageHelper progressMessageHelper = messageHelper.createProgressMessageHelper();
      progressMessageHelper.setMaxProgresss(numFeatures);

      if(m_InputValues->CreatedBinModalRangesDataPaths.has_value())
      {
        modalBinRanges->resizeTuples({numFeatures});
        ExecuteParallelFunctor<InstantiateHistogramByFeatureImplFunctor, NoBooleanType>(InstantiateHistogramByFeatureImplFunctor{}, inputData->getDataType(), dataAlg, modalBinRanges, inputData,
                                                                                        binRanges, featureIdsStore, m_InputValues->MinRange, m_InputValues->MaxRange, histFullRange, m_ShouldCancel,
                                                                                        numBins, counts, mostPopulated, mask, overflow, progressMessageHelper);
      }
      else
      {
        ExecuteParallelFunctor(InstantiateHistogramByFeatureImplFunctor{}, inputData->getDataType(), dataAlg, inputData, binRanges, featureIdsStore, m_InputValues->MinRange, m_InputValues->MaxRange,
                               histFullRange, m_ShouldCancel, numBins, counts, mostPopulated, mask, overflow, progressMessageHelper);
      }
      return {};
    };

    // Every read and created write target participates in storage-based dispatch.
    // A disk-backed participant requires the bounded implementation.
    Result<> executeResult;
    if(maskArray != nullptr && modalBinRanges != nullptr)
    {
      executeResult = DispatchAlgorithm<ComputeArrayHistogramByFeatureDirect, ComputeArrayHistogramByFeatureScanline>(
          AlgorithmArrayTargets{inputData, &featureIdsArray, maskArray, binRanges, countsArray, mostPopulatedArray, modalBinRanges}, executeDirect, *inputData, *binRanges, featureIdsStore, maskArray,
          *countsArray, *mostPopulatedArray, modalBinRanges, *m_InputValues, numFeatures, overflow, m_ShouldCancel);
    }
    else if(maskArray != nullptr)
    {
      executeResult = DispatchAlgorithm<ComputeArrayHistogramByFeatureDirect, ComputeArrayHistogramByFeatureScanline>(
          AlgorithmArrayTargets{inputData, &featureIdsArray, maskArray, binRanges, countsArray, mostPopulatedArray}, executeDirect, *inputData, *binRanges, featureIdsStore, maskArray, *countsArray,
          *mostPopulatedArray, nullptr, *m_InputValues, numFeatures, overflow, m_ShouldCancel);
    }
    else if(modalBinRanges != nullptr)
    {
      executeResult = DispatchAlgorithm<ComputeArrayHistogramByFeatureDirect, ComputeArrayHistogramByFeatureScanline>(
          AlgorithmArrayTargets{inputData, &featureIdsArray, binRanges, countsArray, mostPopulatedArray, modalBinRanges}, executeDirect, *inputData, *binRanges, featureIdsStore, nullptr, *countsArray,
          *mostPopulatedArray, modalBinRanges, *m_InputValues, numFeatures, overflow, m_ShouldCancel);
    }
    else
    {
      executeResult = DispatchAlgorithm<ComputeArrayHistogramByFeatureDirect, ComputeArrayHistogramByFeatureScanline>(
          AlgorithmArrayTargets{inputData, &featureIdsArray, binRanges, countsArray, mostPopulatedArray}, executeDirect, *inputData, *binRanges, featureIdsStore, nullptr, *countsArray,
          *mostPopulatedArray, nullptr, *m_InputValues, numFeatures, overflow, m_ShouldCancel);
    }
    if(executeResult.invalid())
    {
      return executeResult;
    }

    messageHelper.sendMessage(fmt::format("Calculated {} feature histograms!", numFeatures));

    if(overflow > 0)
    {
      messageHelper.sendMessage(fmt::format("{} values not categorized into bin for array {}", overflow.load(), inputData->getName()));
    }
  }

  return {};
}
