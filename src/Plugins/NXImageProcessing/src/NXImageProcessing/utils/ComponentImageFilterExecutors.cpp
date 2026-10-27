#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "NXImageProcessing/utils/ImageFilterExecutorDetail.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/BinaryThinningEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ConnectedComponentEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ThresholdMaximumConnectedComponentsEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ZeroCrossingEngine.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <fmt/format.h>
#include <limits>
#include <memory>
#include <nonstd/span.hpp>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/combinable.h>
#endif

namespace nx::core::ImageProcessing
{
namespace detail
{
namespace
{
/**
 * @brief Dispatched execute body for the (ITK-free) ConnectedComponent filter: labels foreground (non-zero) voxels
 *        into consecutive uint32 components via the streaming scanline union-find @ref LabelConnectedComponents
 *        engine. Output is a FIXED uint32 label image, background 0. Input must be an integer scalar type (the
 *        caller's preflight enforces this via the IntegerOnly policy). Explicit legacy overflow can lose foreground
 *        or merge components through zero and reused IDs.
 */
struct ConnectedComponentExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  bool fullyConnected;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  ConnectedComponentLabelLimitHandling LabelLimitHandling;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<uint32>>(outputArrayPath);
    const auto pred = [](T v) { return v != T{}; }; // compile-time predicate (Pred deduced by LabelConnectedComponents)
    return LabelConnectedComponents<T>(inArray.getDataStoreRef(), outArray.getDataStoreRef(), dims, pred, fullyConnected, shouldCancel, messageHandler, k_ConnectedComponent2DTargetBytes,
                                       LabelLimitHandling);
  }
};

/**
 * @brief Dispatched execute body for the (ITK-free) ZeroCrossing filter: marks each voxel closest to a sign change
 *        (zero crossing) among its axial face neighbors with @c foregroundValue, all others with
 *        @c backgroundValue, via the streamed axial sign-change stencil @ref ApplyZeroCrossing engine. Input must
 *        be a SIGNED scalar type (the caller's preflight enforces this via the SignedScalar policy); output is a
 *        FIXED uint8 label image.
 */
struct ZeroCrossingExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  uint8 foregroundValue;
  uint8 backgroundValue;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outStore = dataStructure.getDataRefAs<DataArray<uint8>>(outputArrayPath).getDataStoreRef();
    return ApplyZeroCrossing<T>(inStore, outStore, dims, foregroundValue, backgroundValue, shouldCancel, messageHandler);
  }
};

/**
 * @brief Dispatched execute body for the (ITK-free) BinaryThinning filter: per-z-slice 2D sequential thinning
 *        (Gonzalez-Woods) via the @ref ApplyBinaryThinning engine, transcribed from itkBinaryThinningImageFilter.hxx.
 *        Foreground (non-zero) voxels are thinned to a 1-pixel skeleton (values 0/1); a 3D image is thinned per-z-slice
 *        independently (ITK's neighbor offsets are 2D). Output is SameAsInput (same type as input). Input must be an
 *        integer scalar type (the caller's preflight enforces this via the IntegerOnly policy).
 */
struct BinaryThinningExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outStore = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath).getDataStoreRef();
    return ApplyBinaryThinning<T>(inStore, outStore, dims, shouldCancel, messageHandler);
  }
};

template <class T>
Result<> StreamDoubleThresholdMarkers(const AbstractDataStore<T>& inStore, AbstractDataStore<uint8>& markerStore, AbstractDataStore<uint8>& maskStore, T markerLower, T markerUpper, T maskLower,
                                      T maskUpper, uint8 insideValue, uint8 outsideValue, const std::atomic_bool& shouldCancel)
{
  constexpr usize k_ChunkValues = std::max<usize>(1, (16ULL * 1024ULL * 1024ULL) / (sizeof(T) + 2 * sizeof(uint8)));
  const usize total = inStore.getSize();
  const usize chunk = std::min(total, k_ChunkValues);
  auto inputBuffer = std::make_unique<T[]>(chunk);
  auto markerBuffer = std::make_unique<uint8[]>(chunk);
  auto maskBuffer = std::make_unique<uint8[]>(chunk);
  for(usize offset = 0; offset < total; offset += k_ChunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkValues, total - offset);
    if(Result<> r = inStore.copyIntoBuffer(offset, nonstd::span<T>(inputBuffer.get(), count)); r.invalid())
    {
      return r;
    }
    for(usize index = 0; index < count; ++index)
    {
      const T value = inputBuffer[index];
      markerBuffer[index] = (markerLower <= value && value <= markerUpper) ? insideValue : outsideValue;
      maskBuffer[index] = (maskLower <= value && value <= maskUpper) ? insideValue : outsideValue;
    }
    if(Result<> r = markerStore.copyFromBuffer(offset, nonstd::span<const uint8>(markerBuffer.get(), count)); r.invalid())
    {
      return r;
    }
    if(Result<> r = maskStore.copyFromBuffer(offset, nonstd::span<const uint8>(maskBuffer.get(), count)); r.invalid())
    {
      return r;
    }
  }
  return {};
}

/**
 * @brief Dispatched execute body for the (ITK-free) DoubleThreshold filter: a 3-stage mini-pipeline transcribed
 *        from itkDoubleThresholdImageFilter.hxx::GenerateData. (1) marker = BinaryThreshold(input, [T2,T3]); (2)
 *        mask = BinaryThreshold(input, [T1,T4]); (3) output = ReconstructionByDilation(marker, mask,
 *        FullyConnected) run to convergence. The two uint8 scratch stores inherit the input's storage format (an
 *        out-of-core input yields out-of-core scratch), keeping the filter memory-bounded through the streamed
 *        reconstruction sweep. ITK stores the four thresholds as the INPUT pixel type T, so the Float64 params are
 *        cast to T FIRST and the band test is done in T (byte-exact vs live ITK). Output is a FIXED uint8 label
 *        image regardless of the input element type.
 */
struct DoubleThresholdExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  float64 threshold1;
  float64 threshold2;
  float64 threshold3;
  float64 threshold4;
  uint8 insideValue;
  uint8 outsideValue;
  bool fullyConnected;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    auto& outArray = dataStructure.getDataRefAs<DataArray<uint8>>(outputArrayPath);
    AbstractDataStore<uint8>& outStore = outArray.getDataStoreRef();

    // ITK stores m_Threshold1..4 as the INPUT pixel type T -> cast double->T, then compare in T (Parity model).
    // NOTE: the float64->T narrowing is IMPLEMENTATION-DEFINED when the Float64 value falls outside T's range
    // (e.g. the default Threshold3/Threshold4 = 254/255 exceed int8's max of 127). This is INTENTIONAL and matches
    // ITK's identical static_cast<InputPixelType> in itkDoubleThresholdImageFilter -- both this facade and the
    // legacy ITK filter perform the SAME cast on the SAME compiler, so they narrow identically and stay byte-exact.
    // A saturating/clamping cast would DIVERGE from ITK and break the parity gate, so it must stay a plain cast.
    const T t1 = static_cast<T>(threshold1);
    const T t2 = static_cast<T>(threshold2);
    const T t3 = static_cast<T>(threshold3);
    const T t4 = static_cast<T>(threshold4);

    // marker (in [t2,t3]) + mask (in [t1,t4]) uint8 scratch stores, inheriting the input's storage format (OOC-inheriting).
    auto markerStorePtr = DataStoreUtilities::CreateDataStoreWithFormat<uint8>(inArray.getDataFormat(), inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
    auto maskStorePtr = DataStoreUtilities::CreateDataStoreWithFormat<uint8>(inArray.getDataFormat(), inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
    AbstractDataStore<uint8>& markerStore = *markerStorePtr;
    AbstractDataStore<uint8>& maskStore = *maskStorePtr;

    if(Result<> r = StreamDoubleThresholdMarkers<T>(inStore, markerStore, maskStore, t2, t3, t1, t4, insideValue, outsideValue, shouldCancel); r.invalid())
    {
      return r;
    }

    // reconstruction-by-dilation: marker under mask -> out. inArray + outArray supply the OOC storage-path detection
    // (the marker/mask are scratch, not IDataArrays); the data is read from markerStore/maskStore.
    return ApplyMorphologicalReconstruction<uint8>(markerStore, maskStore, outStore, dims, ReconstructOp::Dilation, fullyConnected, inArray, outArray, shouldCancel, messageHandler);
  }
};

/**
 * @brief Dispatched execute body for the (ITK-free) RelabelComponent filter: given an ALREADY-LABELED integer scalar
 *        image, computes each label's pixel count, optionally sorts the labels by size (descending, ties broken by
 *        ASCENDING original label -- transcribed VERBATIM from itk::RelabelComponentImageFilter::GenerateData's
 *        std::sort comparator, since std::sort is not stable and the comparator fully determines the output label
 *        VALUES), drops components smaller than @c minimumObjectSize (remapped to background), and reassigns
 *        consecutive labels 1,2,... in that order. Output is SameAsInput (same type as input). This is a
 *        self-contained streaming size-sort-relabel -- it does NOT use the ConnectedComponent union-find engine (the
 *        input is already labeled). Input must be an integer scalar type (the caller's preflight enforces this via
 *        the IntegerOnly policy).
 */
#ifdef SIMPLNX_ENABLE_MULTICORE
template <class ValueT>
using RelabelThreadLocal = tbb::combinable<ValueT>;
#else
template <class ValueT>
class RelabelThreadLocal
{
public:
  RelabelThreadLocal() = default;

  template <class FactoryT>
  explicit RelabelThreadLocal(FactoryT&& factory)
  : m_Value(std::forward<FactoryT>(factory)())
  {
  }

  ValueT& local()
  {
    return m_Value;
  }

  template <class FunctionT>
  void combine_each(FunctionT&& function) const
  {
    function(m_Value);
  }

private:
  ValueT m_Value{};
};
#endif

constexpr int32 k_RelabelComponentResidentStoreMismatch = -8592;

struct RelabelComponentExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  usize numTuples;
  uint64 minimumObjectSize;
  bool sortByObjectSize;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    auto& outStore = outArray.getDataStoreRef();
    constexpr usize k_TargetChunkBytes = 32 * 1024 * 1024;
    constexpr bool k_UseDenseLabels = sizeof(T) <= sizeof(uint16);
    const auto denseIndex = [](T value) -> usize { return static_cast<usize>(static_cast<int64>(value) - static_cast<int64>(std::numeric_limits<T>::lowest())); };
    const bool bothStoresInMemory = inStore.getStoreType() == IDataStore::StoreType::InMemory && outStore.getStoreType() == IDataStore::StoreType::InMemory;
    const bool usesOutOfCoreStore = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || outStore.getStoreType() == IDataStore::StoreType::OutOfCore;
    const auto* residentInputStore = bothStoresInMemory ? dynamic_cast<const DataStore<T>*>(&inStore) : nullptr;
    auto* residentOutputStore = bothStoresInMemory ? dynamic_cast<DataStore<T>*>(&outStore) : nullptr;
    const bool useResidentSpans = residentInputStore != nullptr && residentOutputStore != nullptr;
    const bool inputByteCountFits = numTuples <= std::numeric_limits<usize>::max() / sizeof(T);
    const usize inputBytes = inputByteCountFits ? numTuples * sizeof(T) : 0;
    const uint64 usefulStagingBytes = inputByteCountFits ? static_cast<uint64>(inputBytes) : std::numeric_limits<uint64>::max();
    auto workingMemoryReservation = usesOutOfCoreStore ? ReserveWorkingMemory(k_TargetChunkBytes, usefulStagingBytes) : CacheMemoryBudgetManager::WorkingMemoryReservation{};
    const uint64 grantedBytes = usesOutOfCoreStore ? workingMemoryReservation.sizeBytes() : k_TargetChunkBytes;
    if(numTuples > 0 && grantedBytes < sizeof(T))
    {
      return MakeErrorResult(k_RelabelComponentInsufficientWorkingMemory, fmt::format("Relabel Component cannot reserve one {}-byte staging value for input array '{}'. Input values: {}; cache "
                                                                                      "budget: {}; maximum working memory: {}; granted bytes: {}. Increase the cache-memory budget.",
                                                                                      sizeof(T), inputArrayPath.toString(), numTuples, CacheMemoryBudgetManager::instance().budgetBytes(),
                                                                                      CacheMemoryBudgetManager::instance().maximumWorkingMemoryBytes(), grantedBytes));
    }
    const auto slabPlan = usesOutOfCoreStore ? CreateRelabelChunkSlabPlan(outArray.getTupleShape(), outStore.getChunkShape(), numTuples, sizeof(T)) : std::nullopt;
    const usize stagingValues = useResidentSpans ? 0 : SelectRelabelStagingValues(numTuples, grantedBytes, sizeof(T), slabPlan);
    if(usesOutOfCoreStore)
    {
      workingMemoryReservation.shrinkTo(static_cast<uint64>(stagingValues) * sizeof(T));
    }
    auto buffer = stagingValues == 0 ? std::unique_ptr<T[]>{} : std::make_unique_for_overwrite<T[]>(stagingValues);
    // Pass 1 counts each non-background label.
    // Narrow labels use a dense table. Wider labels use a sparse table that scales with the number of labels.
    using LabelSizePair = std::pair<T, uint64>;
    std::vector<LabelSizePair> sizeVector;
    {
      if constexpr(k_UseDenseLabels)
      {
        constexpr usize k_DomainSize = usize{1} << (sizeof(T) * 8);
        constexpr usize k_CountLanes = sizeof(T) == 1 ? 8 : 1;
        using DenseCounts = std::array<uint64, k_DomainSize * k_CountLanes>;
        RelabelThreadLocal<DenseCounts> threadCounts([] { return DenseCounts{}; });
        const auto countBuffer = [&](const T* input, usize count) {
          auto countRange = [&](const Range& range) {
            if(shouldCancel)
            {
              return;
            }
            auto& counts = threadCounts.local();
            const auto incrementCount = [&](T value, usize lane) {
              if(value != T{})
              {
                ++counts[denseIndex(value) * k_CountLanes + lane];
              }
            };
            usize i = range.min();
            if constexpr(k_CountLanes == 8)
            {
              for(; i + 8 <= range.max(); i += 8)
              {
#ifndef SIMPLNX_ENABLE_MULTICORE
                if((i % 65536) == 0 && shouldCancel)
                {
                  return;
                }
#endif
                uint64 packedValues = 0;
                std::memcpy(&packedValues, input + i, sizeof(packedValues));
                if(packedValues == 0)
                {
                  continue;
                }
                incrementCount(input[i], 0);
                incrementCount(input[i + 1], 1);
                incrementCount(input[i + 2], 2);
                incrementCount(input[i + 3], 3);
                incrementCount(input[i + 4], 4);
                incrementCount(input[i + 5], 5);
                incrementCount(input[i + 6], 6);
                incrementCount(input[i + 7], 7);
              }
            }
            for(; i < range.max(); ++i)
            {
#ifndef SIMPLNX_ENABLE_MULTICORE
              if((i % 65536) == 0 && shouldCancel)
              {
                return;
              }
#endif
              incrementCount(input[i], i % k_CountLanes);
            }
          };
          ParallelDataAlgorithm parallelAlgorithm;
          parallelAlgorithm.setRange(0, count);
          parallelAlgorithm.execute(countRange);
        };
        if(useResidentSpans)
        {
          const auto inputSpan = residentInputStore->createSpan();
          if(inputSpan.size() != numTuples)
          {
            return MakeErrorResult(k_RelabelComponentResidentStoreMismatch, fmt::format("Relabel Component resident input array '{}' has {} values, but the filter expected {} values.",
                                                                                        inputArrayPath.toString(), inputSpan.size(), numTuples));
          }
          countBuffer(inputSpan.data(), inputSpan.size());
        }
        else
        {
          usize start = 0;
          while(start < numTuples)
          {
            if(shouldCancel)
            {
              return {};
            }
            const usize count = std::min(stagingValues, numTuples - start);
            if(Result<> r = inStore.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
            {
              return r;
            }
            countBuffer(buffer.get(), count);
            if(shouldCancel)
            {
              return {};
            }
            start += count;
          }
        }
        std::vector<uint64> counts(k_DomainSize, 0);
        threadCounts.combine_each([&](const DenseCounts& localCounts) {
          for(usize index = 0; index < k_DomainSize; ++index)
          {
            for(usize lane = 0; lane < k_CountLanes; ++lane)
            {
              counts[index] += localCounts[index * k_CountLanes + lane];
            }
          }
        });
        if(shouldCancel)
        {
          return {};
        }
        for(usize index = 0; index < counts.size(); ++index)
        {
          if(counts[index] != 0)
          {
            const T label = static_cast<T>(static_cast<int64>(std::numeric_limits<T>::lowest()) + static_cast<int64>(index));
            if(label != T{})
            {
              sizeVector.emplace_back(label, counts[index]);
            }
          }
        }
      }
      else
      {
        std::unordered_map<T, uint64> counts;
        const auto countBuffer = [&](const T* input, usize count) {
          for(usize i = 0; i < count; ++i)
          {
            if((i % 65536) == 0 && shouldCancel)
            {
              return;
            }
            const T value = input[i];
            if(value != T{})
            {
              ++counts[value];
            }
          }
        };
        if(useResidentSpans)
        {
          const auto inputSpan = residentInputStore->createSpan();
          if(inputSpan.size() != numTuples)
          {
            return MakeErrorResult(k_RelabelComponentResidentStoreMismatch, fmt::format("Relabel Component resident input array '{}' has {} values, but the filter expected {} values.",
                                                                                        inputArrayPath.toString(), inputSpan.size(), numTuples));
          }
          countBuffer(inputSpan.data(), inputSpan.size());
        }
        else
        {
          usize start = 0;
          while(start < numTuples)
          {
            if(shouldCancel)
            {
              return {};
            }
            const usize count = std::min(stagingValues, numTuples - start);
            if(Result<> r = inStore.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
            {
              return r;
            }
            countBuffer(buffer.get(), count);
            if(shouldCancel)
            {
              return {};
            }
            start += count;
          }
        }
        sizeVector.assign(counts.begin(), counts.end());
      }
    }

    if(shouldCancel)
    {
      return {};
    }
    if(sortByObjectSize)
    {
      // Transcribed VERBATIM from itk::RelabelComponentImageFilter::GenerateData's std::sort comparator
      // (itkRelabelComponentImageFilter.hxx): size DESCENDING; ties (equal size) broken by ASCENDING original
      // label. std::sort is NOT stable, so this tie-break -- not "keep relative order" -- is what makes the
      // sorted order (and therefore the output label values) deterministic.
      std::sort(sizeVector.begin(), sizeVector.end(), [](const LabelSizePair& a, const LabelSizePair& b) -> bool { return a.second > b.second || (!(a.second < b.second) && a.first < b.first); });
    }
    else if constexpr(!k_UseDenseLabels)
    {
      // Dense labels were emitted in ascending numeric order. Restore that same ITK m_SizeMap order after the
      // unordered wide-label count path.
      std::sort(sizeVector.begin(), sizeVector.end(), [](const LabelSizePair& a, const LabelSizePair& b) -> bool { return a.first < b.first; });
    }

    if(shouldCancel)
    {
      return {};
    }
    // Build the label -> output-label remap (itkRelabelComponentImageFilter.hxx::GenerateData's relabelMap loop):
    // size < minimumObjectSize -> background (T{}); else the next consecutive label (1, 2, 3, ...).
    std::vector<T> denseRemap;
    std::unordered_map<T, T> sparseRemap;
    if constexpr(k_UseDenseLabels)
    {
      constexpr usize k_DomainSize = usize{1} << (sizeof(T) * 8);
      denseRemap.assign(k_DomainSize, T{});
    }
    else
    {
      sparseRemap.reserve(sizeVector.size());
    }
    T outputLabel{};
    usize survivingObjectCount = 0;
    for(const auto& [label, size] : sizeVector)
    {
      static_cast<void>(label);
      if(minimumObjectSize == 0 || size >= minimumObjectSize)
      {
        ++survivingObjectCount;
      }
    }
    for(const auto& [label, size] : sizeVector)
    {
      if(shouldCancel)
      {
        return {};
      }
      if(minimumObjectSize > 0 && size < minimumObjectSize)
      {
        if constexpr(!k_UseDenseLabels)
        {
          sparseRemap.emplace(label, T{});
        }
      }
      else
      {
        if(outputLabel == std::numeric_limits<T>::max())
        {
          return MakeErrorResult(k_RelabelComponentTooManyObjects,
                                 fmt::format("RelabelComponentImageFilter: too many objects of sufficient size ({}) for the output element type '{}' (max label {}). Reduce the object count or use a "
                                             "wider input/output type.",
                                             survivingObjectCount, DataTypeToString(GetDataType<T>()), static_cast<uint64>(std::numeric_limits<T>::max())));
        }
        outputLabel = static_cast<T>(outputLabel + 1);
        if constexpr(k_UseDenseLabels)
        {
          denseRemap[denseIndex(label)] = outputLabel;
        }
        else
        {
          sparseRemap.emplace(label, outputLabel);
        }
      }
    }

    // Pass 2: resident DataStores copy directly into the final output span, then remap it in place. Other stores
    // stream inStore -> outStore. Each worker touches only its disjoint local-buffer range; abstract DataStore I/O
    // stays serial. Dense remapping is direct-indexed and vectorizable. Wide labels use concurrent const lookups in
    // the immutable sparse table.
    {
      const auto& denseRemapLookup = denseRemap;
      const auto& sparseRemapLookup = sparseRemap;
      const auto remapBuffer = [&](const T* input, T* output, usize count) {
        auto remapRange = [&](const Range& range) {
          if(shouldCancel)
          {
            return;
          }
          usize i = range.min();
          if constexpr(sizeof(T) == 1)
          {
            for(; i + 8 <= range.max(); i += 8)
            {
#ifndef SIMPLNX_ENABLE_MULTICORE
              if((i % 65536) == 0 && shouldCancel)
              {
                return;
              }
#endif
              uint64 packedValues = 0;
              std::memcpy(&packedValues, input + i, sizeof(packedValues));
              if(packedValues == 0)
              {
                if(input != output)
                {
                  std::fill_n(output + i, 8, T{});
                }
                continue;
              }
              output[i] = denseRemapLookup[denseIndex(input[i])];
              output[i + 1] = denseRemapLookup[denseIndex(input[i + 1])];
              output[i + 2] = denseRemapLookup[denseIndex(input[i + 2])];
              output[i + 3] = denseRemapLookup[denseIndex(input[i + 3])];
              output[i + 4] = denseRemapLookup[denseIndex(input[i + 4])];
              output[i + 5] = denseRemapLookup[denseIndex(input[i + 5])];
              output[i + 6] = denseRemapLookup[denseIndex(input[i + 6])];
              output[i + 7] = denseRemapLookup[denseIndex(input[i + 7])];
            }
          }
          for(; i < range.max(); ++i)
          {
#ifndef SIMPLNX_ENABLE_MULTICORE
            if((i % 65536) == 0 && shouldCancel)
            {
              return;
            }
#endif
            const T value = input[i];
            if constexpr(k_UseDenseLabels)
            {
              output[i] = denseRemapLookup[denseIndex(value)];
            }
            else
            {
              output[i] = value == T{} ? T{} : sparseRemapLookup.at(value);
            }
          }
        };
        ParallelDataAlgorithm parallelAlgorithm;
        parallelAlgorithm.setRange(0, count);
        parallelAlgorithm.execute(remapRange);
      };
      if(useResidentSpans)
      {
        const auto inputSpan = residentInputStore->createSpan();
        auto outputSpan = residentOutputStore->createSpan();
        if(inputSpan.size() != numTuples || outputSpan.size() != numTuples)
        {
          return MakeErrorResult(k_RelabelComponentResidentStoreMismatch,
                                 fmt::format("Relabel Component resident stores do not match the expected value count. Input array '{}': {}; output array '{}': {}; expected: {}.",
                                             inputArrayPath.toString(), inputSpan.size(), outputArrayPath.toString(), outputSpan.size(), numTuples));
        }
        if(shouldCancel)
        {
          return {};
        }
        remapBuffer(inputSpan.data(), outputSpan.data(), outputSpan.size());
      }
      else
      {
        for(usize start = 0; start < numTuples;)
        {
          if(shouldCancel)
          {
            return {};
          }
          const usize count = std::min(stagingValues, numTuples - start);
          if(Result<> r = inStore.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
          {
            return r;
          }
          remapBuffer(buffer.get(), buffer.get(), count);
          if(shouldCancel)
          {
            return {};
          }
          if(Result<> r = outStore.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); r.invalid())
          {
            return r;
          }
          start += count;
        }
      }
    }
    return {};
  }
};

/** @brief Native threshold search adapter; the scoped engine owns validation, warnings and bounded I/O. */
struct ThresholdMaxCCExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  ThresholdMaxCCOptions Options;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& input = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& output = dataStructure.getDataRefAs<DataArray<uint8>>(outputArrayPath).getDataStoreRef();
    return ApplyThresholdMaximumConnectedComponents(input, output, dims, Options, shouldCancel, messageHandler);
  }
};
} // namespace
} // namespace detail

// -----------------------------------------------------------------------------
Result<> ExecuteConnectedComponentImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, bool fullyConnected,
                                              const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, ConnectedComponentLabelLimitHandling labelLimitHandling)
{
  using TypeSetPolicy = IntegerOnly;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  detail::ConnectedComponentExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, imageGeom.getDimensions(), fullyConnected, shouldCancel, messageHandler, labelLimitHandling};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteZeroCrossingImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, uint8 foregroundValue,
                                        uint8 backgroundValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = SignedScalar;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  detail::ZeroCrossingExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, imageGeom.getDimensions(), foregroundValue, backgroundValue, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteDoubleThresholdImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, float64 threshold1,
                                           float64 threshold2, float64 threshold3, float64 threshold4, uint8 insideValue, uint8 outsideValue, bool fullyConnected, const std::atomic_bool& shouldCancel,
                                           const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  detail::DoubleThresholdExecuteFn executeFn{dataStructure,  inputArrayPath, outputArrayPath, imageGeom.getDimensions(), threshold1, threshold2, threshold3, threshold4, insideValue, outsideValue,
                                             fullyConnected, shouldCancel,   messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteRelabelComponentImageFilter(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& outputArrayPath, uint64 minimumObjectSize, bool sortByObjectSize,
                                            const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = IntegerOnly;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  detail::RelabelComponentExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, inputArray.getNumberOfTuples(), minimumObjectSize, sortByObjectSize, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteBinaryThinningImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                          const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = IntegerOnly;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  detail::BinaryThinningExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, imageGeom.getDimensions(), shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteThresholdMaximumConnectedComponentsImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                               uint32 minimumObjectSizeInPixels, float64 upperBoundary, uint8 insideValue, uint8 outsideValue, const std::atomic_bool& shouldCancel,
                                                               const IFilter::MessageHandler& messageHandler, bool warnOnNonfinite)
{
  using TypeSetPolicy = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const ThresholdMaxCCOptions options{
      .MinimumObjectSize = minimumObjectSizeInPixels, .UpperBoundary = upperBoundary, .InsideValue = insideValue, .OutsideValue = outsideValue, .WarnOnNonfinite = warnOnNonfinite};
  const detail::ThresholdMaxCCExecuteFn executeFn{.dataStructure = dataStructure,
                                                  .inputArrayPath = inputArrayPath,
                                                  .outputArrayPath = outputArrayPath,
                                                  .dims = imageGeom.getDimensions(),
                                                  .Options = options,
                                                  .shouldCancel = shouldCancel,
                                                  .messageHandler = messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

} // namespace nx::core::ImageProcessing
