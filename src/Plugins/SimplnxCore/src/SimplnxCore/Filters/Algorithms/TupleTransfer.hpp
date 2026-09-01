#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"

#include <array>
#include <nonstd/span.hpp>

namespace nx::core
{
/**
 * @namespace nx::core::detail
 * @brief Contains checked bounded tuple-transfer implementations.
 */
namespace detail
{
// Bounded transfers use four 65,536-value pages and 4,096-face destination runs.
constexpr usize kTransferPageValues = 65536;
constexpr usize kTransferPageCount = 4;
constexpr usize kTransferFaceRun = 4096;

/**
 * @brief Copies cell tuples to two-sided face tuples with bounded pages.
 * @tparam T Specifies source and destination value type.
 * @tparam Record Specifies one face-transfer record.
 * @tparam SourceIndices Maps a record to two source tuple indexes.
 * @param source Supplies cell tuples.
 * @param destination Receives two source tuples per face.
 * @param numComps Number of components in one source tuple.
 * @param records Supplies contiguous destination face records.
 * @param sourceIndices Returns two source indexes or usize max for an exterior side.
 * @return Allocation, shape, range, ordering, or bulk-I/O result.
 *
 * Four 65,536-value LRU pages serve sparse source indexes. Destination runs
 * target 4,096 faces and 65,536 values but retain at least one complete face.
 * A wide face can exceed that target. A tuple wider than one page streams one
 * side and component range at a time. The function does not inspect cancellation.
 * An all-exterior batch is a no-op. An error does not restore prior writes.
 */
template <typename T, typename Record, typename SourceIndices>
Result<> TransferBoundedCellTuples(AbstractDataStore<T>& source, AbstractDataStore<T>& destination, usize numComps, nonstd::span<const Record> records, SourceIndices sourceIndices)
{
  if(records.empty())
  {
    return {};
  }
  if(numComps == 0 || numComps > std::numeric_limits<usize>::max() / 2)
  {
    return MakeErrorResult(-62060, "Tuple transfer component count is zero or overflows its two-sided destination shape.");
  }
  const usize valuesPerFace = numComps * 2;
  if(source.getNumberOfComponents() != numComps || destination.getNumberOfComponents() != valuesPerFace)
  {
    return MakeErrorResult(-62066, "Tuple transfer array component shapes are incompatible.");
  }
  const usize tuplesPerPage = kTransferPageValues / numComps;
  const usize facesPerRun = std::min(kTransferFaceRun, std::max(usize{1}, kTransferPageValues / valuesPerFace));
  const usize sourceTuples = source.getNumberOfTuples();
  const usize destinationTuples = destination.getNumberOfTuples();
  bool hasSource = false;
  for(const auto& record : records)
  {
    const auto indices = sourceIndices(record);
    hasSource = indices[0] != std::numeric_limits<usize>::max() || indices[1] != std::numeric_limits<usize>::max();
    if(hasSource)
    {
      break;
    }
  }
  if(!hasSource)
  {
    return {};
  }

  // A tuple wider than one page streams component ranges without tuple-sized scratch.
  if(numComps > kTransferPageValues)
  {
    std::unique_ptr<T[]> values;
    try
    {
      values = std::make_unique<T[]>(kTransferPageValues);
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult(-62061, "Tuple transfer failed to allocate its bounded component buffer.");
    } catch(const std::length_error&)
    {
      return MakeErrorResult(-62061, "Tuple transfer failed to allocate its bounded component buffer.");
    }

    const usize firstFace = records.front().faceIndex;
    if(firstFace > destinationTuples || records.size() > destinationTuples - firstFace)
    {
      return MakeErrorResult(-62065, "Tuple transfer destination face range exceeds its destination array.");
    }
    for(usize localRecord = 0; localRecord < records.size(); localRecord++)
    {
      const auto& record = records[localRecord];
      if(record.faceIndex != firstFace + localRecord || record.faceIndex > std::numeric_limits<usize>::max() / valuesPerFace)
      {
        return MakeErrorResult(-62063, "Tuple transfer records must be contiguous in destination face order.");
      }
      const auto indices = sourceIndices(record);
      const usize destinationFaceOffset = record.faceIndex * valuesPerFace;
      for(usize side = 0; side < 2; side++)
      {
        const usize sourceTuple = indices[side];
        if(sourceTuple != std::numeric_limits<usize>::max() && (sourceTuple >= sourceTuples || sourceTuple > std::numeric_limits<usize>::max() / numComps))
        {
          return MakeErrorResult(-62062, "Tuple transfer source tuple index is outside the source array.");
        }
        const usize sourceTupleOffset = sourceTuple == std::numeric_limits<usize>::max() ? 0 : sourceTuple * numComps;
        if(destinationFaceOffset > std::numeric_limits<usize>::max() - side * numComps)
        {
          return MakeErrorResult(-62065, "Tuple transfer destination face offset overflows usize.");
        }
        const usize destinationSideOffset = destinationFaceOffset + side * numComps;
        for(usize componentStart = 0; componentStart < numComps;)
        {
          const usize componentCount = std::min(kTransferPageValues, numComps - componentStart);
          if(sourceTuple == std::numeric_limits<usize>::max())
          {
            std::fill_n(values.get(), componentCount, T{});
          }
          else
          {
            if(sourceTupleOffset > std::numeric_limits<usize>::max() - componentStart)
            {
              return MakeErrorResult(-62064, "Tuple transfer source component offset overflows usize.");
            }
            auto readResult = source.copyIntoBuffer(sourceTupleOffset + componentStart, nonstd::span<T>(values.get(), componentCount));
            if(readResult.invalid())
            {
              return readResult;
            }
          }
          if(destinationSideOffset > std::numeric_limits<usize>::max() - componentStart)
          {
            return MakeErrorResult(-62065, "Tuple transfer destination component offset overflows usize.");
          }
          auto writeResult = destination.copyFromBuffer(destinationSideOffset + componentStart, nonstd::span<const T>(values.get(), componentCount));
          if(writeResult.invalid())
          {
            return writeResult;
          }
          componentStart += componentCount;
        }
      }
    }
    return {};
  }

  std::array<std::unique_ptr<T[]>, kTransferPageCount> pages;
  std::array<usize, kTransferPageCount> pageStarts;
  pageStarts.fill(std::numeric_limits<usize>::max());
  std::array<usize, kTransferPageCount> pageUse{};
  usize useCounter = 0;
  try
  {
    for(auto& page : pages)
    {
      page = std::make_unique<T[]>(kTransferPageValues);
    }
  } catch(const std::bad_alloc&)
  {
    return MakeErrorResult(-62061, "Tuple transfer failed to allocate its bounded source pages.");
  } catch(const std::length_error&)
  {
    return MakeErrorResult(-62061, "Tuple transfer failed to allocate its bounded source pages.");
  }

  const auto sourceValue = [&](usize tupleIndex, usize component) -> Result<const T*> {
    if(tupleIndex >= sourceTuples)
    {
      return MakeErrorResult<const T*>(-62062, "Tuple transfer source tuple index is outside the source array.");
    }
    const usize pageStart = (tupleIndex / tuplesPerPage) * tuplesPerPage;
    if(pageStart > std::numeric_limits<usize>::max() / numComps)
    {
      return MakeErrorResult<const T*>(-62064, "Tuple transfer source-page offset overflows usize.");
    }
    usize pageIndex = 0;
    while(pageIndex < kTransferPageCount && pageStarts[pageIndex] != pageStart)
    {
      pageIndex++;
    }
    if(pageIndex == kTransferPageCount)
    {
      pageIndex = static_cast<usize>(std::min_element(pageUse.begin(), pageUse.end()) - pageUse.begin());
      const usize pageTuples = std::min(tuplesPerPage, sourceTuples - pageStart);
      auto readResult = source.copyIntoBuffer(pageStart * numComps, nonstd::span<T>(pages[pageIndex].get(), pageTuples * numComps));
      if(readResult.invalid())
      {
        return ConvertInvalidResult<const T*>(std::move(readResult));
      }
      pageStarts[pageIndex] = pageStart;
    }
    pageUse[pageIndex] = ++useCounter;
    return {pages[pageIndex].get() + (tupleIndex - pageStart) * numComps + component};
  };

  for(usize recordStart = 0; recordStart < records.size(); recordStart += facesPerRun)
  {
    const usize recordCount = std::min(facesPerRun, records.size() - recordStart);
    const usize firstFace = records[recordStart].faceIndex;
    if(recordCount > std::numeric_limits<usize>::max() / valuesPerFace || firstFace > std::numeric_limits<usize>::max() / valuesPerFace ||
       firstFace > std::numeric_limits<usize>::max() - recordCount || firstFace + recordCount > destinationTuples)
    {
      return MakeErrorResult(-62065, "Tuple transfer destination face range overflows or exceeds its destination array.");
    }
    try
    {
      auto destinationBuffer = std::make_unique<T[]>(recordCount * valuesPerFace);
      std::fill_n(destinationBuffer.get(), recordCount * valuesPerFace, T{});
      for(usize localRecord = 0; localRecord < recordCount; localRecord++)
      {
        const auto& record = records[recordStart + localRecord];
        if(record.faceIndex != firstFace + localRecord)
        {
          return MakeErrorResult(-62063, "Tuple transfer records must be contiguous in destination face order.");
        }
        const auto indices = sourceIndices(record);
        for(usize side = 0; side < 2; side++)
        {
          if(indices[side] == std::numeric_limits<usize>::max())
          {
            continue;
          }
          for(usize component = 0; component < numComps; component++)
          {
            auto valueResult = sourceValue(indices[side], component);
            if(valueResult.invalid())
            {
              return ConvertResult(std::move(valueResult));
            }
            destinationBuffer[(localRecord * 2 + side) * numComps + component] = *valueResult.value();
          }
        }
      }
      auto writeResult = destination.copyFromBuffer(firstFace * valuesPerFace, nonstd::span<const T>(destinationBuffer.get(), recordCount * valuesPerFace));
      if(writeResult.invalid())
      {
        return writeResult;
      }
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult(-62061, "Tuple transfer failed to allocate its bounded destination run.");
    } catch(const std::length_error&)
    {
      return MakeErrorResult(-62061, "Tuple transfer failed to allocate its bounded destination run.");
    }
  }
  return {};
}

/**
 * @brief Copies feature tuples to two-sided faces through Feature ID indirection.
 * @tparam T Specifies feature and destination value type.
 * @tparam K Specifies the Feature ID value type.
 * @tparam Record Specifies one face-transfer record.
 * @tparam SourceIndices Maps a record to two cell indexes.
 * @param featureIds Maps source cells to feature tuples.
 * @param featureData Supplies feature tuples.
 * @param destination Receives two feature tuples per face.
 * @param numComps Number of components in one feature tuple.
 * @param records Supplies contiguous destination face records.
 * @param sourceIndices Returns two cell indexes or usize max for an exterior side.
 * @return Allocation, shape, ID, range, ordering, or bulk-I/O result.
 *
 * Independent four-page caches serve Feature IDs and feature tuples. Destination
 * runs retain complete two-sided faces and can exceed their value target. A
 * feature tuple wider than one page streams side and component ranges. This
 * function does not inspect cancellation. An all-exterior batch is a no-op.
 * Earlier writes remain after error.
 */
template <typename T, typename K, typename Record, typename SourceIndices>
Result<> TransferBoundedFeatureTuples(AbstractDataStore<K>& featureIds, AbstractDataStore<T>& featureData, AbstractDataStore<T>& destination, usize numComps, nonstd::span<const Record> records,
                                      SourceIndices sourceIndices)
{
  if(records.empty() || numComps == 0 || numComps > std::numeric_limits<usize>::max() / 2)
  {
    return records.empty() ? Result<>{} : MakeErrorResult(-62070, "Feature tuple transfer component count is zero or overflows its two-sided destination shape.");
  }
  const usize valuesPerFace = numComps * 2;
  if(featureIds.getNumberOfComponents() != 1 || featureData.getNumberOfComponents() != numComps || destination.getNumberOfComponents() != valuesPerFace)
  {
    return MakeErrorResult(-62078, "Feature tuple transfer array component shapes are incompatible.");
  }
  constexpr usize kInvalid = std::numeric_limits<usize>::max();
  bool hasSource = false;
  for(const auto& record : records)
  {
    const auto cells = sourceIndices(record);
    hasSource = cells[0] != kInvalid || cells[1] != kInvalid;
    if(hasSource)
    {
      break;
    }
  }
  if(!hasSource)
  {
    return {};
  }

  if(numComps > kTransferPageValues)
  {
    std::unique_ptr<K[]> idPage;
    std::unique_ptr<T[]> values;
    try
    {
      idPage = std::make_unique<K[]>(kTransferPageValues);
      values = std::make_unique<T[]>(kTransferPageValues);
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult(-62071, "Feature tuple transfer failed to allocate its bounded component buffers.");
    } catch(const std::length_error&)
    {
      return MakeErrorResult(-62071, "Feature tuple transfer failed to allocate its bounded component buffers.");
    }

    usize idPageStart = kInvalid;
    const auto loadId = [&](usize cell) -> Result<K> {
      if(cell >= featureIds.getNumberOfTuples())
      {
        return MakeErrorResult<K>(-62072, "Feature tuple transfer cell index is outside FeatureIds.");
      }
      const usize pageStart = (cell / kTransferPageValues) * kTransferPageValues;
      if(pageStart != idPageStart)
      {
        auto readResult = featureIds.copyIntoBuffer(pageStart, nonstd::span<K>(idPage.get(), std::min(kTransferPageValues, featureIds.getNumberOfTuples() - pageStart)));
        if(readResult.invalid())
        {
          return ConvertInvalidResult<K>(std::move(readResult));
        }
        idPageStart = pageStart;
      }
      return {idPage[cell - pageStart]};
    };

    const usize firstFace = records.front().faceIndex;
    if(firstFace > destination.getNumberOfTuples() || records.size() > destination.getNumberOfTuples() - firstFace)
    {
      return MakeErrorResult(-62074, "Feature tuple transfer destination range is invalid.");
    }
    for(usize localRecord = 0; localRecord < records.size(); localRecord++)
    {
      const auto& record = records[localRecord];
      if(record.faceIndex != firstFace + localRecord || record.faceIndex > std::numeric_limits<usize>::max() / valuesPerFace)
      {
        return MakeErrorResult(-62075, "Feature tuple transfer records must be contiguous.");
      }
      const auto cells = sourceIndices(record);
      const usize destinationFaceOffset = record.faceIndex * valuesPerFace;
      for(usize side = 0; side < 2; side++)
      {
        usize feature = kInvalid;
        if(cells[side] != kInvalid)
        {
          auto featureIdResult = loadId(cells[side]);
          if(featureIdResult.invalid())
          {
            return ConvertResult(std::move(featureIdResult));
          }
          if constexpr(std::is_signed_v<K>)
          {
            if(featureIdResult.value() < 0)
            {
              return MakeErrorResult(-62076, "Feature tuple transfer encountered a negative feature id.");
            }
          }
          feature = static_cast<usize>(featureIdResult.value());
          if(feature >= featureData.getNumberOfTuples() || feature > std::numeric_limits<usize>::max() / numComps)
          {
            return MakeErrorResult(-62073, "Feature tuple transfer feature id is outside its feature array.");
          }
        }

        const usize sourceTupleOffset = feature == kInvalid ? 0 : feature * numComps;
        if(destinationFaceOffset > std::numeric_limits<usize>::max() - side * numComps)
        {
          return MakeErrorResult(-62074, "Feature tuple transfer destination offset overflows usize.");
        }
        const usize destinationSideOffset = destinationFaceOffset + side * numComps;
        for(usize componentStart = 0; componentStart < numComps;)
        {
          const usize componentCount = std::min(kTransferPageValues, numComps - componentStart);
          if(feature == kInvalid)
          {
            std::fill_n(values.get(), componentCount, T{});
          }
          else
          {
            if(sourceTupleOffset > std::numeric_limits<usize>::max() - componentStart)
            {
              return MakeErrorResult(-62077, "Feature tuple transfer source component offset overflows usize.");
            }
            auto readResult = featureData.copyIntoBuffer(sourceTupleOffset + componentStart, nonstd::span<T>(values.get(), componentCount));
            if(readResult.invalid())
            {
              return readResult;
            }
          }
          if(destinationSideOffset > std::numeric_limits<usize>::max() - componentStart)
          {
            return MakeErrorResult(-62074, "Feature tuple transfer destination component offset overflows usize.");
          }
          auto writeResult = destination.copyFromBuffer(destinationSideOffset + componentStart, nonstd::span<const T>(values.get(), componentCount));
          if(writeResult.invalid())
          {
            return writeResult;
          }
          componentStart += componentCount;
        }
      }
    }
    return {};
  }

  const usize tuplesPerFeaturePage = kTransferPageValues / numComps;
  const usize facesPerRun = std::min(kTransferFaceRun, std::max(usize{1}, kTransferPageValues / valuesPerFace));
  std::array<std::unique_ptr<K[]>, kTransferPageCount> idPages;
  std::array<std::unique_ptr<T[]>, kTransferPageCount> valuePages;
  std::array<usize, kTransferPageCount> idStarts, valueStarts, idUse{}, valueUse{};
  usize idUseCounter = 0;
  usize valueUseCounter = 0;
  idStarts.fill(kInvalid);
  valueStarts.fill(kInvalid);
  try
  {
    for(usize i = 0; i < kTransferPageCount; i++)
    {
      idPages[i] = std::make_unique<K[]>(kTransferPageValues);
      valuePages[i] = std::make_unique<T[]>(kTransferPageValues);
    }
  } catch(const std::bad_alloc&)
  {
    return MakeErrorResult(-62071, "Feature tuple transfer failed to allocate bounded pages.");
  } catch(const std::length_error&)
  {
    return MakeErrorResult(-62071, "Feature tuple transfer failed to allocate bounded pages.");
  }
  const auto loadId = [&](usize cell) -> Result<K> {
    if(cell >= featureIds.getNumberOfTuples())
      return MakeErrorResult<K>(-62072, "Feature tuple transfer cell index is outside FeatureIds.");
    const usize start = (cell / kTransferPageValues) * kTransferPageValues;
    usize p = 0;
    while(p < kTransferPageCount && idStarts[p] != start)
      p++;
    if(p == kTransferPageCount)
    {
      p = static_cast<usize>(std::min_element(idUse.begin(), idUse.end()) - idUse.begin());
      auto r = featureIds.copyIntoBuffer(start, nonstd::span<K>(idPages[p].get(), std::min(kTransferPageValues, featureIds.getNumberOfTuples() - start)));
      if(r.invalid())
        return ConvertInvalidResult<K>(std::move(r));
      idStarts[p] = start;
    }
    idUse[p] = ++idUseCounter;
    return {idPages[p][cell - start]};
  };
  const auto loadValue = [&](usize feature, usize component) -> Result<const T*> {
    if(feature >= featureData.getNumberOfTuples())
      return MakeErrorResult<const T*>(-62073, "Feature tuple transfer feature id is outside its feature array.");
    const usize start = (feature / tuplesPerFeaturePage) * tuplesPerFeaturePage;
    usize p = 0;
    while(p < kTransferPageCount && valueStarts[p] != start)
      p++;
    if(p == kTransferPageCount)
    {
      p = static_cast<usize>(std::min_element(valueUse.begin(), valueUse.end()) - valueUse.begin());
      const usize count = std::min(tuplesPerFeaturePage, featureData.getNumberOfTuples() - start);
      if(start > std::numeric_limits<usize>::max() / numComps || count > std::numeric_limits<usize>::max() / numComps)
      {
        return MakeErrorResult<const T*>(-62077, "Feature tuple transfer page offset overflows usize.");
      }
      auto r = featureData.copyIntoBuffer(start * numComps, nonstd::span<T>(valuePages[p].get(), count * numComps));
      if(r.invalid())
        return ConvertInvalidResult<const T*>(std::move(r));
      valueStarts[p] = start;
    }
    valueUse[p] = ++valueUseCounter;
    return {valuePages[p].get() + (feature - start) * numComps + component};
  };
  for(usize begin = 0; begin < records.size(); begin += facesPerRun)
  {
    const usize count = std::min(facesPerRun, records.size() - begin);
    const usize first = records[begin].faceIndex;
    if(count > std::numeric_limits<usize>::max() / valuesPerFace || first > std::numeric_limits<usize>::max() / valuesPerFace || first > destination.getNumberOfTuples() ||
       count > destination.getNumberOfTuples() - first)
      return MakeErrorResult(-62074, "Feature tuple transfer destination range is invalid.");
    std::unique_ptr<T[]> out;
    try
    {
      out = std::make_unique<T[]>(count * valuesPerFace);
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult(-62071, "Feature tuple transfer failed to allocate its bounded destination run.");
    } catch(const std::length_error&)
    {
      return MakeErrorResult(-62071, "Feature tuple transfer failed to allocate its bounded destination run.");
    }
    std::fill_n(out.get(), count * valuesPerFace, T{});
    for(usize n = 0; n < count; n++)
    {
      const auto& rec = records[begin + n];
      if(rec.faceIndex != first + n)
        return MakeErrorResult(-62075, "Feature tuple transfer records must be contiguous.");
      const auto cells = sourceIndices(rec);
      for(usize side = 0; side < 2; side++)
        if(cells[side] != kInvalid)
        {
          auto fid = loadId(cells[side]);
          if(fid.invalid())
            return ConvertResult(std::move(fid));
          if constexpr(std::is_signed_v<K>)
            if(fid.value() < 0)
              return MakeErrorResult(-62076, "Feature tuple transfer encountered a negative feature id.");
          const usize feature = static_cast<usize>(fid.value());
          for(usize c = 0; c < numComps; c++)
          {
            auto value = loadValue(feature, c);
            if(value.invalid())
              return ConvertResult(std::move(value));
            out[(n * 2 + side) * numComps + c] = *value.value();
          }
        }
    }
    auto r = destination.copyFromBuffer(first * valuesPerFace, nonstd::span<const T>(out.get(), count * valuesPerFace));
    if(r.invalid())
      return r;
  }
  return {};
}
} // namespace detail

/**
 * @struct QuickSurfaceTransferData
 * @brief Stores one QuickSurfaceMesh face transfer.
 *
 * A -1 label marks an exterior side. Batch calls require ascending contiguous
 * faceIndex values so one destination run can publish all records.
 */
struct QuickSurfaceTransferData
{
  usize faceIndex = 0;
  usize firstcIndex = 0;
  usize secondcIndex = 0;
  int32 faceLabel0 = 0;
  int32 faceLabel1 = 0;
};

/**
 * @struct SurfaceNetsTransferData
 * @brief Stores one Surface Nets face transfer.
 *
 * usize max marks an exterior side. Batch calls require ascending contiguous
 * faceIndex values so one destination run can publish all records.
 */
struct SurfaceNetsTransferData
{
  usize faceIndex = 0;
  std::array<usize, 2> quadNxArrayIndices = {std::numeric_limits<usize>::max(), std::numeric_limits<usize>::max()};
};

/**
 * @class AbstractTupleTransfer
 * @brief Defines cell-to-face and feature-to-face tuple transfer operations.
 *
 * Each destination face stores side zero components followed by side one
 * components. Direct methods use unchecked per-value store access and cannot
 * report storage failures. Exterior sides remain unchanged in direct output.
 *
 * Batch methods use checked source pages and contiguous destination runs. A
 * mixed batch writes zeros for exterior sides. An all-exterior batch is a no-op.
 * Base batch implementations are no-ops; typed subclasses perform transfers.
 * Transfers do not synchronize shared stores. Concurrent calls must not write
 * overlapping destination ranges.
 */
class SIMPLNXCORE_EXPORT AbstractTupleTransfer
{
public:
  /**
   * @brief Destroys a typed transfer through the base interface.
   */
  virtual ~AbstractTupleTransfer() = default;

  AbstractTupleTransfer(const AbstractTupleTransfer&) = delete;
  AbstractTupleTransfer(AbstractTupleTransfer&&) noexcept = delete;
  AbstractTupleTransfer& operator=(const AbstractTupleTransfer&) = delete;
  AbstractTupleTransfer& operator=(AbstractTupleTransfer&&) noexcept = delete;

  /**
   * @brief Copies one tuple for point-sampled triangle output.
   * @param faceIndex First destination value index.
   * @param firstcIndex First source value index or source cell index, by subclass.
   */
  virtual void pointSampleTransfer(size_t faceIndex, size_t firstcIndex) = 0;

  /**
   * @brief Copies adjacent tuples to one QuickSurfaceMesh face by value access.
   * @param faceIndex Destination face index.
   * @param firstcIndex Side-zero cell index.
   * @param secondcIndex Side-one cell index.
   * @param faceLabels Supplies -1 for exterior sides.
   */
  virtual void quickSurfaceTransfer(size_t faceIndex, size_t firstcIndex, size_t secondcIndex, AbstractDataStore<int32>& faceLabels) = 0;

  /**
   * @brief Copies adjacent tuples to one Surface Nets face by value access.
   * @param faceIndex Destination face index.
   * @param quadNxArrayIndices Supplies cell indexes or usize max for exterior sides.
   */
  virtual void surfaceNetsTransfer(size_t faceIndex, const std::array<usize, 2>& quadNxArrayIndices) = 0;

  /**
   * @brief Handles one QuickSurfaceMesh record batch.
   * @param records Ordered contiguous face records.
   * @return Success from the base no-op implementation.
   */
  virtual Result<> quickSurfaceTransferBatch(nonstd::span<const QuickSurfaceTransferData> /*records*/)
  {
    return {};
  }

  /**
   * @brief Handles one Surface Nets record batch.
   * @param records Ordered contiguous face records.
   * @return Success from the base no-op implementation.
   */
  virtual Result<> surfaceNetsTransferBatch(nonstd::span<const SurfaceNetsTransferData> /*records*/)
  {
    return {};
  }

protected:
  /**
   * @brief Initializes empty transfer metadata for a typed subclass.
   */
  AbstractTupleTransfer() = default;

  DataPath m_SourceDataPath;
  DataPath m_DestinationDataPath;
  size_t m_NumComps = 0;
};

/**
 * @class TransferTuple
 * @brief Transfers typed cell tuples to two-sided face tuples.
 * @tparam T Specifies source and destination value type.
 *
 * Direct methods use unchecked per-value access. Batch methods validate source
 * indexes, destination runs, and component shapes before checked bulk I/O.
 */
template <typename T>
class TransferTuple : public AbstractTupleTransfer
{
public:
  /**
   * @brief Defines the typed DataArray.
   */
  using DataArrayType = DataArray<T>;
  /**
   * @brief Defines the typed abstract store.
   */
  using DataStoreType = AbstractDataStore<T>;

  /**
   * @brief Initializes a cell-to-face transfer.
   * @param dataStructure Contains source and destination arrays.
   * @param selectedDataPath Identifies the cell source.
   * @param createdArrayPath Identifies the two-sided face destination.
   * @pre Source and destination have type T and outlive this transfer.
   * @pre Destination components equal two times source components for face transfers.
   */
  TransferTuple(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& createdArrayPath)
  : m_CellRef(dataStructure.template getDataRefAs<DataArrayType>(selectedDataPath).getDataStoreRef())
  , m_FaceRef(dataStructure.template getDataRefAs<DataArrayType>(createdArrayPath).getDataStoreRef())
  {
    m_SourceDataPath = selectedDataPath;
    m_DestinationDataPath = createdArrayPath;

    IDataArray* cellArrayPtr = dataStructure.template getDataAs<IDataArray>(m_SourceDataPath);
    m_NumComps = cellArrayPtr->getNumberOfComponents();
  }

  /**
   * @brief Destroys the typed cell transfer.
   */
  ~TransferTuple() override = default;
  TransferTuple(const TransferTuple&) = delete;
  TransferTuple(TransferTuple&&) noexcept = delete;
  TransferTuple& operator=(const TransferTuple&) = delete;
  TransferTuple& operator=(TransferTuple&&) noexcept = delete;

  /**
   * @brief Copies one source value range to a destination value range.
   * @param faceIndex First destination value index.
   * @param firstcIndex First source value index.
   * @pre Both value ranges contain m_NumComps values.
   */
  void pointSampleTransfer(size_t faceIndex, size_t firstcIndex) override
  {
    for(size_t i = 0; i < m_NumComps; i++)
    {
      m_FaceRef[faceIndex + i] = m_CellRef[firstcIndex + i];
    }
  }

  /**
   * @brief Copies non-exterior cell tuples to one QuickSurfaceMesh face.
   * @param faceIndex Destination face index.
   * @param firstcIndex Side-zero source cell.
   * @param secondcIndex Side-one source cell.
   * @param faceLabels Supplies -1 for an exterior side.
   * @pre Non-exterior source and destination ranges are valid.
   */
  void quickSurfaceTransfer(size_t faceIndex, size_t firstcIndex, size_t secondcIndex, AbstractDataStore<int32>& faceLabels) override
  {
    // Leave exterior destination sides unchanged.
    if(faceLabels[faceIndex * 2] != -1)
    {
      for(size_t i = 0; i < m_NumComps; i++)
      {
        m_FaceRef[faceIndex * m_NumComps * 2 + i] = m_CellRef[firstcIndex * m_NumComps + i];
      }
    }

    if(faceLabels[faceIndex * 2 + 1] != -1)
    {
      for(size_t i = 0; i < m_NumComps; i++)
      {
        size_t index = (faceIndex * m_NumComps * 2) + m_NumComps + i;
        m_FaceRef[index] = m_CellRef[secondcIndex * m_NumComps + i];
      }
    }
  }

  /**
   * @brief Copies non-exterior cell tuples to one Surface Nets face.
   * @param faceIndex Destination face index.
   * @param quadNxArrayIndices Supplies source cells or usize max for exterior sides.
   * @pre Non-exterior source and destination ranges are valid.
   */
  void surfaceNetsTransfer(size_t faceIndex, const std::array<usize, 2>& quadNxArrayIndices) override
  {
    // Leave exterior destination sides unchanged.
    if(quadNxArrayIndices[0] != std::numeric_limits<usize>::max())
    {
      for(size_t i = 0; i < m_NumComps; i++)
      {
        m_FaceRef[faceIndex * m_NumComps * 2 + i] = m_CellRef[quadNxArrayIndices[0] * m_NumComps + i];
      }
    }

    if(quadNxArrayIndices[1] != std::numeric_limits<usize>::max())
    {
      for(size_t i = 0; i < m_NumComps; i++)
      {
        size_t index = (faceIndex * m_NumComps * 2) + m_NumComps + i;
        m_FaceRef[index] = m_CellRef[quadNxArrayIndices[1] * m_NumComps + i];
      }
    }
  }

  /**
   * @brief Transfers one QuickSurfaceMesh batch with bounded pages.
   * @param records Ordered contiguous face records.
   * @return Allocation, shape, range, ordering, or bulk-I/O result.
   */
  Result<> quickSurfaceTransferBatch(nonstd::span<const QuickSurfaceTransferData> records) override
  {
    return detail::TransferBoundedCellTuples(m_CellRef, m_FaceRef, m_NumComps, records, [](const QuickSurfaceTransferData& record) {
      return std::array<usize, 2>{record.faceLabel0 == -1 ? std::numeric_limits<usize>::max() : record.firstcIndex, record.faceLabel1 == -1 ? std::numeric_limits<usize>::max() : record.secondcIndex};
    });
  }

  /**
   * @brief Transfers one Surface Nets batch with bounded pages.
   * @param records Ordered contiguous face records.
   * @return Allocation, shape, range, ordering, or bulk-I/O result.
   */
  Result<> surfaceNetsTransferBatch(nonstd::span<const SurfaceNetsTransferData> records) override
  {
    return detail::TransferBoundedCellTuples(m_CellRef, m_FaceRef, m_NumComps, records, [](const SurfaceNetsTransferData& record) { return record.quadNxArrayIndices; });
  }

private:
  DataStoreType& m_CellRef;
  DataStoreType& m_FaceRef;
};

/**
 * @class TransferFeatureTuple
 * @brief Transfers typed feature tuples to two-sided face tuples.
 * @tparam T Specifies feature and destination value type.
 * @tparam K Specifies the Feature ID value type.
 *
 * Face transfers resolve cell index to Feature ID to feature data. Direct
 * methods use unchecked per-value access. Batch methods validate cell indexes,
 * Feature IDs, destination runs, and component shapes before checked bulk I/O.
 */
template <typename T, typename K>
class TransferFeatureTuple : public AbstractTupleTransfer
{
public:
  /**
   * @brief Defines the feature and destination DataArray type.
   */
  using DataArrayType = DataArray<T>;
  /**
   * @brief Defines the Feature ID DataArray type.
   */
  using FeatureIdsArrayType = DataArray<K>;
  /**
   * @brief Defines the feature and destination abstract store type.
   */
  using DataStoreType = AbstractDataStore<T>;
  /**
   * @brief Defines the Feature ID abstract store type.
   */
  using FeatureIdsStoreType = AbstractDataStore<K>;

  /**
   * @brief Initializes a feature-to-face transfer.
   * @param dataStructure Contains feature, Feature ID, and destination arrays.
   * @param selectedDataPath Identifies feature data.
   * @param createdArrayPath Identifies the two-sided face destination.
   * @param featureIdsArrayPath Identifies the cell-to-feature map.
   * @pre Arrays have types T, T, and K and outlive this transfer.
   * @pre Feature IDs are scalar. Face destination components are twice the feature components.
   */
  TransferFeatureTuple(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& createdArrayPath, const DataPath& featureIdsArrayPath)
  : m_FeatureDataRef(dataStructure.template getDataRefAs<DataArrayType>(selectedDataPath).getDataStoreRef())
  , m_FaceRef(dataStructure.template getDataRefAs<DataArrayType>(createdArrayPath).getDataStoreRef())
  , m_FeatureIdsRef(dataStructure.template getDataRefAs<FeatureIdsArrayType>(featureIdsArrayPath).getDataStoreRef())
  {
    m_SourceDataPath = selectedDataPath;
    m_DestinationDataPath = createdArrayPath;

    IDataArray* cellArrayPtr = dataStructure.template getDataAs<IDataArray>(m_SourceDataPath);
    m_NumComps = cellArrayPtr->getNumberOfComponents();
  }

  /**
   * @brief Destroys the typed feature transfer.
   */
  ~TransferFeatureTuple() override = default;
  TransferFeatureTuple(const TransferFeatureTuple&) = delete;
  TransferFeatureTuple(TransferFeatureTuple&&) noexcept = delete;
  TransferFeatureTuple& operator=(const TransferFeatureTuple&) = delete;
  TransferFeatureTuple& operator=(TransferFeatureTuple&&) noexcept = delete;

  /**
   * @brief Copies feature values through the current flat-offset point-sample rule.
   * @param faceIndex First destination value index.
   * @param firstcIndex Source cell index.
   * @pre The resolved Feature ID and destination range are valid.
   *
   * This method uses Feature ID as the first flat feature-data value. It does not
   * multiply by m_NumComps. Multi-component input therefore does not select the
   * complete tuple at that Feature ID.
   */
  void pointSampleTransfer(size_t faceIndex, size_t firstcIndex) override
  {
    K firstFeatureId = m_FeatureIdsRef[firstcIndex];
    for(size_t i = 0; i < m_NumComps; i++)
    {
      m_FaceRef[faceIndex + i] = m_FeatureDataRef[firstFeatureId + i];
    }
  }

  /**
   * @brief Copies non-exterior feature tuples to one QuickSurfaceMesh face.
   * @param faceIndex Destination face index.
   * @param firstcIndex Side-zero source cell.
   * @param secondcIndex Side-one source cell.
   * @param faceLabels Supplies -1 for an exterior side.
   * @pre Resolved Feature IDs and destination ranges are valid.
   */
  void quickSurfaceTransfer(size_t faceIndex, size_t firstcIndex, size_t secondcIndex, AbstractDataStore<int32>& faceLabels) override
  {
    // Leave exterior destination sides unchanged.
    if(faceLabels[faceIndex * 2] != -1)
    {
      K firstFeatureId = m_FeatureIdsRef[firstcIndex];
      for(size_t i = 0; i < m_NumComps; i++)
      {
        m_FaceRef[faceIndex * m_NumComps * 2 + i] = m_FeatureDataRef[firstFeatureId * m_NumComps + i];
      }
    }

    if(faceLabels[faceIndex * 2 + 1] != -1)
    {
      K secondFeatureId = m_FeatureIdsRef[secondcIndex];
      for(size_t i = 0; i < m_NumComps; i++)
      {
        size_t index = (faceIndex * m_NumComps * 2) + m_NumComps + i;
        m_FaceRef[index] = m_FeatureDataRef[secondFeatureId * m_NumComps + i];
      }
    }
  }

  /**
   * @brief Copies non-exterior feature tuples to one Surface Nets face.
   * @param faceIndex Destination face index.
   * @param quadNxArrayIndices Supplies source cells or usize max for exterior sides.
   * @pre Resolved Feature IDs and destination ranges are valid.
   */
  void surfaceNetsTransfer(size_t faceIndex, const std::array<usize, 2>& quadNxArrayIndices) override
  {
    // Leave exterior destination sides unchanged.
    if(quadNxArrayIndices[0] != std::numeric_limits<usize>::max())
    {
      usize firstcIndex = quadNxArrayIndices[0];
      K firstFeatureId = m_FeatureIdsRef[firstcIndex];
      for(size_t i = 0; i < m_NumComps; i++)
      {
        m_FaceRef[faceIndex * m_NumComps * 2 + i] = m_FeatureDataRef[firstFeatureId * m_NumComps + i];
      }
    }

    if(quadNxArrayIndices[1] != std::numeric_limits<usize>::max())
    {
      usize secondcIndex = quadNxArrayIndices[1];
      K secondFeatureId = m_FeatureIdsRef[secondcIndex];
      for(size_t i = 0; i < m_NumComps; i++)
      {
        size_t index = (faceIndex * m_NumComps * 2) + m_NumComps + i;
        m_FaceRef[index] = m_FeatureDataRef[secondFeatureId * m_NumComps + i];
      }
    }
  }

  /**
   * @brief Transfers one QuickSurfaceMesh batch through bounded ID and value pages.
   * @param records Ordered contiguous face records.
   * @return Allocation, shape, ID, range, ordering, or bulk-I/O result.
   */
  Result<> quickSurfaceTransferBatch(nonstd::span<const QuickSurfaceTransferData> records) override
  {
    return detail::TransferBoundedFeatureTuples<T, K>(m_FeatureIdsRef, m_FeatureDataRef, m_FaceRef, m_NumComps, records, [](const QuickSurfaceTransferData& record) {
      constexpr usize kInvalid = std::numeric_limits<usize>::max();
      return std::array<usize, 2>{record.faceLabel0 == -1 ? kInvalid : record.firstcIndex, record.faceLabel1 == -1 ? kInvalid : record.secondcIndex};
    });
  }

  /**
   * @brief Transfers one Surface Nets batch through bounded ID and value pages.
   * @param records Ordered contiguous face records.
   * @return Allocation, shape, ID, range, ordering, or bulk-I/O result.
   */
  Result<> surfaceNetsTransferBatch(nonstd::span<const SurfaceNetsTransferData> records) override
  {
    return detail::TransferBoundedFeatureTuples<T, K>(m_FeatureIdsRef, m_FeatureDataRef, m_FaceRef, m_NumComps, records,
                                                      [](const SurfaceNetsTransferData& record) { return record.quadNxArrayIndices; });
  }

private:
  DataStoreType& m_FeatureDataRef;
  DataStoreType& m_FaceRef;
  FeatureIdsStoreType& m_FeatureIdsRef;
};

/**
 * @brief Appends a typed cell-to-face transfer for a runtime DataType.
 * @param dataStructure Contains source and destination arrays.
 * @param selectedDataPath Identifies the cell source.
 * @param createdDataPath Identifies the face destination.
 * @param tupleTransferFunctions Receives the new transfer.
 * @pre Paths identify compatible DataArrays.
 */
SIMPLNXCORE_EXPORT void AddTupleTransferInstance(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& createdDataPath,
                                                 std::vector<std::shared_ptr<AbstractTupleTransfer>>& tupleTransferFunctions);

/**
 * @brief Appends a typed feature-to-face transfer for runtime DataTypes.
 * @param dataStructure Contains feature, Feature ID, and destination arrays.
 * @param selectedDataPath Identifies feature data.
 * @param createdDataPath Identifies the face destination.
 * @param featureIdsArrayPath Identifies the cell-to-feature map.
 * @param tupleTransferFunctions Receives the new transfer.
 * @pre Paths identify compatible DataArrays and Feature IDs are Int32.
 */
SIMPLNXCORE_EXPORT void AddFeatureTupleTransferInstance(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& createdDataPath, const DataPath& featureIdsArrayPath,
                                                        std::vector<std::shared_ptr<AbstractTupleTransfer>>& tupleTransferFunctions);

} // namespace nx::core
