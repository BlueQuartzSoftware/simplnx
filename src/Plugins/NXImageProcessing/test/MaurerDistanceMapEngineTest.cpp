#include "simplnx/Utilities/ImageProcessing/MaurerDistanceMapEngine.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"

#include <algorithm>
#include <catch2/catch.hpp>

#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;

namespace
{
usize FlatIndex(usize x, usize y, usize z, usize dimX, usize dimY)
{
  return (z * dimY + y) * dimX + x;
}

void RequireBitIdentical(const std::vector<float32>& actual, const std::vector<float32>& expected)
{
  REQUIRE(actual.size() == expected.size());
  for(usize index = 0; index < actual.size(); ++index)
  {
    CAPTURE(index);
    REQUIRE(std::bit_cast<uint32>(actual[index]) == std::bit_cast<uint32>(expected[index]));
  }
}

template <class T>
class ReadCountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++m_ReadCount;
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  [[nodiscard]] usize readCount() const noexcept
  {
    return m_ReadCount;
  }

private:
  mutable usize m_ReadCount = 0;
};

template <class T>
class CountingForwardingDataStore : public AbstractDataStore<T>
{
public:
  using value_type = typename AbstractDataStore<T>::value_type;

  CountingForwardingDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue)
  : m_Store(tupleShape, componentShape, initValue)
  {
  }

  usize getNumberOfTuples() const override
  {
    return m_Store.getNumberOfTuples();
  }

  const ShapeType& getTupleShape() const override
  {
    return m_Store.getTupleShape();
  }

  usize getNumberOfComponents() const override
  {
    return m_Store.getNumberOfComponents();
  }

  const ShapeType& getComponentShape() const override
  {
    return m_Store.getComponentShape();
  }

  Result<> resizeTuples(const ShapeType& tupleShape) override
  {
    return m_Store.resizeTuples(tupleShape);
  }

  DataType getDataType() const override
  {
    return m_Store.getDataType();
  }

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::InMemory;
  }

  IDataStore::StoreType getPlannedStoreType() const override
  {
    return m_Store.getPlannedStoreType();
  }

  std::string getDataFormat() const override
  {
    return m_Store.getDataFormat();
  }

  std::optional<ShapeType> getChunkShape() const override
  {
    return m_Store.getChunkShape();
  }

  std::map<std::string, std::string> getRecoveryMetadata() const override
  {
    return m_Store.getRecoveryMetadata();
  }

  usize getTypeSize() const override
  {
    return m_Store.getTypeSize();
  }

  std::unique_ptr<IDataStore> deepCopy(const std::string& destinationFormat) const override
  {
    return m_Store.deepCopy(destinationFormat);
  }

  std::unique_ptr<IDataStore> createNewInstance() const override
  {
    return m_Store.createNewInstance();
  }

  std::pair<int32, std::string> writeBinaryFile(const std::string& absoluteFilePath) const override
  {
    return m_Store.writeBinaryFile(absoluteFilePath);
  }

  std::pair<int32, std::string> writeBinaryFile(std::ostream& outputStream) const override
  {
    return m_Store.writeBinaryFile(outputStream);
  }

  value_type getValue(usize index) const override
  {
    return m_Store.getValue(index);
  }

  void setValue(usize index, value_type value) override
  {
    m_Store.setValue(index, value);
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++m_ReadCount;
    return m_Store.copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    return m_Store.copyFromBuffer(startIndex, buffer);
  }

  Result<std::vector<T>> readExtent(const Extent& extent) const override
  {
    return m_Store.readExtent(extent);
  }

  Result<> readExtentIntoBuffer(const Extent& extent, nonstd::span<T> destination) const override
  {
    return m_Store.readExtentIntoBuffer(extent, destination);
  }

  Result<> readExtentsIntoBuffers(nonstd::span<const Extent> extents, nonstd::span<nonstd::span<T>> destinations) const override
  {
    return m_Store.readExtentsIntoBuffers(extents, destinations);
  }

  Result<std::vector<std::vector<T>>> readExtents(nonstd::span<const Extent> extents) const override
  {
    return m_Store.readExtents(extents);
  }

  Result<> writeExtent(const Extent& extent, nonstd::span<const T> data) override
  {
    return m_Store.writeExtent(extent, data);
  }

  value_type at(usize index) const override
  {
    return m_Store.at(index);
  }

  void add(usize index, value_type value) override
  {
    m_Store.add(index, value);
  }

  void sub(usize index, value_type value) override
  {
    m_Store.sub(index, value);
  }

  void mul(usize index, value_type value) override
  {
    m_Store.mul(index, value);
  }

  void div(usize index, value_type value) override
  {
    m_Store.div(index, value);
  }

  void rem(usize index, value_type value) override
  {
    m_Store.rem(index, value);
  }

  void bitwiseAND(usize index, value_type value) override
  {
    m_Store.bitwiseAND(index, value);
  }

  void bitwiseOR(usize index, value_type value) override
  {
    m_Store.bitwiseOR(index, value);
  }

  void bitwiseXOR(usize index, value_type value) override
  {
    m_Store.bitwiseXOR(index, value);
  }

  void bitwiseLShift(usize index, value_type value) override
  {
    m_Store.bitwiseLShift(index, value);
  }

  void bitwiseRShift(usize index, value_type value) override
  {
    m_Store.bitwiseRShift(index, value);
  }

  void byteSwap(usize index) override
  {
    m_Store.byteSwap(index);
  }

  void swap(usize index1, usize index2) override
  {
    m_Store.swap(index1, index2);
  }

  void fill(value_type value) override
  {
    m_Store.fill(value);
  }

  bool copy(const AbstractDataStore<T>& other) override
  {
    return m_Store.copy(other);
  }

  void flush() const override
  {
    m_Store.flush();
  }

  uint64 memoryUsage() const override
  {
    return m_Store.memoryUsage();
  }

  Result<> readHdf5(const hdf5::DatasetIO& dataset) override
  {
    return m_Store.readHdf5(dataset);
  }

  Result<> writeHdf5(hdf5::DatasetIO& dataset) const override
  {
    return m_Store.writeHdf5(dataset);
  }

  [[nodiscard]] usize readCount() const noexcept
  {
    return m_ReadCount;
  }

private:
  DataStore<T> m_Store;
  mutable usize m_ReadCount = 0;
};

template <class T>
class CancelOnThirdReadDataStore : public DataStore<T>
{
public:
  CancelOnThirdReadDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, T initValue, std::atomic_bool& shouldCancel)
  : DataStore<T>(tupleShape, componentShape, initValue)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    Result<> result = DataStore<T>::copyIntoBuffer(startIndex, buffer);
    ++m_ReadCount;
    if(m_ReadCount == 3)
    {
      m_ShouldCancel.store(true);
    }
    return result;
  }

  [[nodiscard]] usize readCount() const noexcept
  {
    return m_ReadCount;
  }

private:
  std::atomic_bool& m_ShouldCancel;
  mutable usize m_ReadCount = 0;
};

template <class T>
class OocReportingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++m_ReadCount;
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  [[nodiscard]] usize readCount() const noexcept
  {
    return m_ReadCount;
  }

private:
  mutable usize m_ReadCount = 0;
};

template <class T>
class ExtentCountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    ++m_FlatWriteCount;
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  Result<> writeExtent(const Extent& extent, nonstd::span<const T> data) override
  {
    ++m_ExtentWriteCount;
    return DataStore<T>::writeExtent(extent, data);
  }

  [[nodiscard]] usize flatWriteCount() const noexcept
  {
    return m_FlatWriteCount;
  }

  [[nodiscard]] usize extentWriteCount() const noexcept
  {
    return m_ExtentWriteCount;
  }

private:
  usize m_FlatWriteCount = 0;
  usize m_ExtentWriteCount = 0;
};

template <class T>
using WriteCountingDataStoreType = ExtentCountingDataStore<T>;

struct TransferEvent
{
  enum class Kind
  {
    InputRead,
    WorkRead,
    WorkWrite,
    OutputExtent
  };

  Kind kind;
  usize start = 0;
  usize count = 0;
  Extent extent;
};

struct TransferLog
{
  std::atomic<int32> active{0};
  std::atomic<int32> max_active{0};
  std::mutex mutex;
  std::vector<TransferEvent> events;
};

class ActiveCallScope
{
public:
  explicit ActiveCallScope(TransferLog& log)
  : m_Log(log)
  {
    const int32 activeCount = m_Log.active.fetch_add(1, std::memory_order_relaxed) + 1;
    int32 observedMaximum = m_Log.max_active.load(std::memory_order_relaxed);
    while(observedMaximum < activeCount && !m_Log.max_active.compare_exchange_weak(observedMaximum, activeCount, std::memory_order_relaxed))
    {
    }
  }

  ~ActiveCallScope()
  {
    m_Log.active.fetch_sub(1, std::memory_order_relaxed);
  }

  ActiveCallScope(const ActiveCallScope&) = delete;
  ActiveCallScope(ActiveCallScope&&) noexcept = delete;
  ActiveCallScope& operator=(const ActiveCallScope&) = delete;
  ActiveCallScope& operator=(ActiveCallScope&&) noexcept = delete;

private:
  TransferLog& m_Log;
};

void RecordTransfer(TransferLog& log, TransferEvent event)
{
  std::lock_guard<std::mutex> lock(log.mutex);
  log.events.push_back(std::move(event));
}

std::vector<TransferEvent> GetTransferEvents(TransferLog& log, TransferEvent::Kind kind)
{
  std::lock_guard<std::mutex> lock(log.mutex);
  std::vector<TransferEvent> events;
  std::copy_if(log.events.cbegin(), log.events.cend(), std::back_inserter(events), [kind](const TransferEvent& event) { return event.kind == kind; });
  return events;
}

std::vector<TransferEvent> GetAllTransferEvents(TransferLog& log)
{
  std::lock_guard<std::mutex> lock(log.mutex);
  return log.events;
}

std::vector<std::pair<usize, usize>> GetTransferRanges(const std::vector<TransferEvent>& events)
{
  std::vector<std::pair<usize, usize>> ranges;
  ranges.reserve(events.size());
  std::transform(events.cbegin(), events.cend(), std::back_inserter(ranges), [](const TransferEvent& event) { return std::pair<usize, usize>{event.start, event.count}; });
  return ranges;
}

template <class T>
class RecordingInputStore : public DataStore<T>
{
public:
  RecordingInputStore(const ShapeType& tupleShape, const ShapeType& componentShape, T initValue, TransferLog& log, std::atomic_bool& shouldCancel, std::optional<usize> failAtRead = std::nullopt,
                      std::optional<usize> cancelAtRead = std::nullopt, std::optional<usize> throwAtRead = std::nullopt)
  : DataStore<T>(tupleShape, componentShape, initValue)
  , m_Log(log)
  , m_ShouldCancel(shouldCancel)
  , m_FailAtRead(failAtRead)
  , m_CancelAtRead(cancelAtRead)
  , m_ThrowAtRead(throwAtRead)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ActiveCallScope scope(m_Log);
    const usize readOrdinal = m_ReadCount.fetch_add(1, std::memory_order_relaxed) + 1;
    RecordTransfer(m_Log, TransferEvent{TransferEvent::Kind::InputRead, startIndex, buffer.size(), {}});
    if(m_ThrowAtRead == readOrdinal)
    {
      throw std::runtime_error("Injected Maurer input read exception.");
    }
    if(m_FailAtRead == readOrdinal)
    {
      return MakeErrorResult(-8399, "Injected Maurer input read failure.");
    }
    Result<> result = DataStore<T>::copyIntoBuffer(startIndex, buffer);
    if(m_CancelAtRead == readOrdinal)
    {
      m_ShouldCancel.store(true);
    }
    return result;
  }

private:
  TransferLog& m_Log;
  std::atomic_bool& m_ShouldCancel;
  std::optional<usize> m_FailAtRead;
  std::optional<usize> m_CancelAtRead;
  std::optional<usize> m_ThrowAtRead;
  mutable std::atomic<usize> m_ReadCount{0};
};

class RecordingWorkStore : public DataStore<float32>
{
public:
  RecordingWorkStore(const ShapeType& tupleShape, const ShapeType& componentShape, float32 initValue, TransferLog& log, std::atomic_bool& shouldCancel, std::optional<usize> failAtRead = std::nullopt,
                     std::optional<usize> failAtWrite = std::nullopt, std::optional<usize> cancelAtRead = std::nullopt, std::optional<usize> throwAtRead = std::nullopt,
                     std::optional<usize> throwAtWrite = std::nullopt)
  : DataStore<float32>(tupleShape, componentShape, initValue)
  , m_Log(log)
  , m_ShouldCancel(shouldCancel)
  , m_FailAtRead(failAtRead)
  , m_FailAtWrite(failAtWrite)
  , m_CancelAtRead(cancelAtRead)
  , m_ThrowAtRead(throwAtRead)
  , m_ThrowAtWrite(throwAtWrite)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<float32> buffer) const override
  {
    ActiveCallScope scope(m_Log);
    const usize readOrdinal = m_ReadCount.fetch_add(1, std::memory_order_relaxed) + 1;
    RecordTransfer(m_Log, TransferEvent{.kind = TransferEvent::Kind::WorkRead, .start = startIndex, .count = buffer.size(), .extent = {}});
    if(m_ThrowAtRead == readOrdinal)
    {
      throw std::runtime_error("Injected Maurer work read exception.");
    }
    if(m_FailAtRead == readOrdinal)
    {
      return MakeErrorResult(-8399, "Injected Maurer work read failure.");
    }
    Result<> result = DataStore<float32>::copyIntoBuffer(startIndex, buffer);
    if(m_CancelAtRead == readOrdinal)
    {
      m_ShouldCancel.store(true);
    }
    return result;
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const float32> buffer) override
  {
    ActiveCallScope scope(m_Log);
    const usize writeOrdinal = m_WriteCount.fetch_add(1, std::memory_order_relaxed) + 1;
    RecordTransfer(m_Log, TransferEvent{.kind = TransferEvent::Kind::WorkWrite, .start = startIndex, .count = buffer.size(), .extent = {}});
    if(m_ThrowAtWrite == writeOrdinal)
    {
      throw std::runtime_error("Injected Maurer work write exception.");
    }
    if(m_FailAtWrite == writeOrdinal)
    {
      return MakeErrorResult(-8398, "Injected Maurer work write failure.");
    }
    return DataStore<float32>::copyFromBuffer(startIndex, buffer);
  }

private:
  TransferLog& m_Log;
  std::atomic_bool& m_ShouldCancel;
  std::optional<usize> m_FailAtRead;
  std::optional<usize> m_FailAtWrite;
  std::optional<usize> m_CancelAtRead;
  std::optional<usize> m_ThrowAtRead;
  std::optional<usize> m_ThrowAtWrite;
  mutable std::atomic<usize> m_ReadCount{0};
  std::atomic<usize> m_WriteCount{0};
};

class RecordingOutputStore : public DataStore<float32>
{
public:
  RecordingOutputStore(const ShapeType& tupleShape, const ShapeType& componentShape, float32 initValue, TransferLog& log, std::atomic_bool& shouldCancel,
                       std::optional<usize> cancelAtExtent = std::nullopt, std::optional<usize> throwAtExtent = std::nullopt)
  : DataStore<float32>(tupleShape, componentShape, initValue)
  , m_Log(log)
  , m_ShouldCancel(shouldCancel)
  , m_CancelAtExtent(cancelAtExtent)
  , m_ThrowAtExtent(throwAtExtent)
  {
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const float32> buffer) override
  {
    ActiveCallScope scope(m_Log);
    m_FlatWriteCount.fetch_add(1, std::memory_order_relaxed);
    return DataStore<float32>::copyFromBuffer(startIndex, buffer);
  }

  Result<> writeExtent(const Extent& extent, nonstd::span<const float32> data) override
  {
    ActiveCallScope scope(m_Log);
    const usize extentOrdinal = m_ExtentCount.fetch_add(1, std::memory_order_relaxed) + 1;
    RecordTransfer(m_Log, TransferEvent{.kind = TransferEvent::Kind::OutputExtent, .start = 0, .count = data.size(), .extent = extent});
    if(m_ThrowAtExtent == extentOrdinal)
    {
      throw std::runtime_error("Injected Maurer output extent exception.");
    }
    Result<> extentWriteResult = DataStore<float32>::writeExtent(extent, data);
    if(extentWriteResult.invalid())
    {
      return extentWriteResult;
    }
    m_CompletedExtentCount.fetch_add(1, std::memory_order_relaxed);
    if(m_CancelAtExtent == extentOrdinal)
    {
      m_ShouldCancel.store(true);
    }
    return extentWriteResult;
  }

  [[nodiscard]] usize flatWriteCount() const noexcept
  {
    return m_FlatWriteCount.load(std::memory_order_relaxed);
  }

  [[nodiscard]] usize completedExtentCount() const noexcept
  {
    return m_CompletedExtentCount.load(std::memory_order_relaxed);
  }

private:
  TransferLog& m_Log;
  std::atomic_bool& m_ShouldCancel;
  std::optional<usize> m_CancelAtExtent;
  std::optional<usize> m_ThrowAtExtent;
  std::atomic<usize> m_FlatWriteCount{0};
  std::atomic<usize> m_ExtentCount{0};
  std::atomic<usize> m_CompletedExtentCount{0};
};

struct SeamResults
{
  Result<> xy_result;
  Result<> z_result;
  bool z_ran = false;
};

template <class T>
SeamResults RunSeam(const std::vector<int32>& pattern, const SizeVec3Type& dims, const ImageProcessing::detail::Maurer3DSlabParameters<T>& parameters, TransferLog& /*unused*/,
                    RecordingInputStore<T>& inputStore, RecordingWorkStore& workStore, RecordingOutputStore& outputStore, std::atomic_bool& shouldCancel)
{
  REQUIRE(pattern.size() == dims[0] * dims[1] * dims[2]);
  for(usize index = 0; index < pattern.size(); ++index)
  {
    inputStore.setValue(index, static_cast<T>(pattern[index]));
  }

  bool hasBoundary = false;
  Result<> xyResult = ImageProcessing::detail::StreamMaurer3DInitAndTransformXY<T>(inputStore, workStore, parameters, shouldCancel, hasBoundary);
  Result<> zResult;
  bool zRan = false;
  if(xyResult.valid())
  {
    zRan = true;
    zResult = ImageProcessing::detail::RunMaurer3DZPass<T>(workStore, outputStore, parameters, shouldCancel, hasBoundary);
  }
  return {.xy_result = std::move(xyResult), .z_result = std::move(zResult), .z_ran = zRan};
}

usize PlanGrant(const SizeVec3Type& dims, usize rows)
{
  const usize workerCount = std::max<usize>(1, static_cast<usize>(std::thread::hardware_concurrency()));
  const usize maximumLineValues = std::max({dims[0], dims[1], dims[2]});
  const usize workerScratchBytes = workerCount * maximumLineValues * 3 * sizeof(float32);
  const usize stagingRowBytes = dims[0] * dims[2] * sizeof(float32);
  return workerScratchBytes + 2 * rows * stagingRowBytes;
}

std::vector<float32> ReadOutput(const RecordingOutputStore& outputStore, usize valueCount)
{
  std::vector<float32> output(valueCount);
  for(usize index = 0; index < valueCount; ++index)
  {
    output[index] = outputStore.getValue(index);
  }
  return output;
}

// This brute-force signed EDT oracle does not use image spacing.
// An object voxel is a feature when it has an in-bounds background neighbor in full connectivity.
// The 2D case uses eight-connectivity. Out-of-bounds neighbors do not create boundaries.
// Distance is the exact Euclidean distance to the nearest feature. The sign follows the ITK rule.
std::vector<float32> MaurerOracle(const std::vector<int32>& in, usize dx, usize dy, usize dz, int32 bg, bool insideIsPositive, bool squared)
{
  const int64 nX = static_cast<int64>(dx);
  const int64 ny = static_cast<int64>(dy);
  const int64 nz = static_cast<int64>(dz);
  const int64 wzLo = (nz == 1) ? 0 : -1;
  const int64 wzHi = (nz == 1) ? 0 : 1;
  auto at = [&](int64 x, int64 y, int64 z) { return in[FlatIndex(static_cast<usize>(x), static_cast<usize>(y), static_cast<usize>(z), dx, dy)]; };

  std::vector<std::array<int64, 3>> feats;
  for(int64 z = 0; z < nz; ++z)
  {
    for(int64 y = 0; y < ny; ++y)
    {
      for(int64 x = 0; x < nX; ++x)
      {
        if(at(x, y, z) == bg)
        {
          continue;
        }
        bool boundary = false;
        for(int64 wz = wzLo; wz <= wzHi && !boundary; ++wz)
        {
          for(int64 wy = -1; wy <= 1 && !boundary; ++wy)
          {
            for(int64 wx = -1; wx <= 1 && !boundary; ++wx)
            {
              if(wx == 0 && wy == 0 && wz == 0)
              {
                continue;
              }
              const int64 ax = x + wx;
              const int64 ay = y + wy;
              const int64 az = z + wz;
              if(ax < 0 || ay < 0 || az < 0 || ax >= nX || ay >= ny || az >= nz)
              {
                continue; // ITK BinaryContour ignores out-of-bounds neighbors.
              }
              if(at(ax, ay, az) == bg)
              {
                boundary = true;
              }
            }
          }
        }
        if(boundary)
        {
          feats.push_back({x, y, z});
        }
      }
    }
  }

  std::vector<float32> out(dx * dy * dz);
  for(int64 z = 0; z < nz; ++z)
  {
    for(int64 y = 0; y < ny; ++y)
    {
      for(int64 x = 0; x < nX; ++x)
      {
        int64 bestSq = std::numeric_limits<int64>::max();
        for(const auto& f : feats)
        {
          const int64 ddx = x - f[0];
          const int64 ddy = y - f[1];
          const int64 ddz = z - f[2];
          const int64 sq = ddx * ddx + ddy * ddy + ddz * ddz;
          bestSq = std::min(sq, bestSq);
        }
        const float32 v = squared ? static_cast<float32>(bestSq) : std::sqrt(static_cast<float32>(bestSq));
        const bool inside = at(x, y, z) != bg;
        out[FlatIndex(static_cast<usize>(x), static_cast<usize>(y), static_cast<usize>(z), dx, dy)] = ((inside == insideIsPositive) ? v : -v);
      }
    }
  }
  return out;
}

template <class T>
std::vector<float32> BuildMaurerEncodedInitReference(const std::vector<T>& input, T backgroundValue, const SizeVec3Type& dims, bool insideIsPositive)
{
  constexpr float32 k_Max = std::numeric_limits<float32>::max();
  const int64 dimX = static_cast<int64>(dims[0]);
  const int64 dimY = static_cast<int64>(dims[1]);
  const int64 dimZ = static_cast<int64>(dims[2]);
  const int64 minOffsetZ = dimZ == 1 ? 0 : -1;
  const int64 maxOffsetZ = dimZ == 1 ? 0 : 1;
  const auto flatIndex = [dimX, dimY](int64 x, int64 y, int64 z) { return static_cast<usize>((z * dimY + y) * dimX + x); };

  std::vector<float32> reference(input.size());
  for(int64 z = 0; z < dimZ; ++z)
  {
    for(int64 y = 0; y < dimY; ++y)
    {
      for(int64 x = 0; x < dimX; ++x)
      {
        const usize index = flatIndex(x, y, z);
        const bool isObject = input[index] != backgroundValue;
        bool isBoundary = false;
        if(isObject)
        {
          for(int64 offsetZ = minOffsetZ; offsetZ <= maxOffsetZ && !isBoundary; ++offsetZ)
          {
            for(int64 offsetY = -1; offsetY <= 1 && !isBoundary; ++offsetY)
            {
              for(int64 offsetX = -1; offsetX <= 1 && !isBoundary; ++offsetX)
              {
                if(offsetX == 0 && offsetY == 0 && offsetZ == 0)
                {
                  continue;
                }
                const int64 neighborX = x + offsetX;
                const int64 neighborY = y + offsetY;
                const int64 neighborZ = z + offsetZ;
                if(neighborX < 0 || neighborY < 0 || neighborZ < 0 || neighborX >= dimX || neighborY >= dimY || neighborZ >= dimZ)
                {
                  continue;
                }
                isBoundary = input[flatIndex(neighborX, neighborY, neighborZ)] == backgroundValue;
              }
            }
          }
        }
        reference[index] = ImageProcessing::detail::MaurerSignedValue(isBoundary ? 0.0f : k_Max, isObject, insideIsPositive);
      }
    }
  }
  return reference;
}

template <class T>
std::vector<float32> RunInCore(const std::vector<int32>& pattern, usize dx, usize dy, usize dz, int32 bg, bool insideIsPositive, bool squared, bool useSpacing = false,
                               FloatVec3Type spacing = FloatVec3Type{1.0f, 1.0f, 1.0f})
{
  DataStore<T> inStore(ShapeType{dz, dy, dx}, ShapeType{1}, static_cast<T>(0));
  for(usize i = 0; i < pattern.size(); ++i)
  {
    inStore.setValue(i, static_cast<T>(pattern[i]));
  }
  DataStore<float32> outStore(ShapeType{dz, dy, dx}, ShapeType{1}, 0.0f);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  MaurerDistanceInCore<T> engine(inStore, outStore, SizeVec3Type{dx, dy, dz}, static_cast<T>(bg), insideIsPositive, squared, useSpacing, std::move(spacing), shouldCancel, messageHandler);
  const Result<> r = engine();
  REQUIRE(r.valid());
  std::vector<float32> out(pattern.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

template <class T>
std::vector<float32> RunSlab(const std::vector<int32>& pattern, usize dx, usize dy, usize dz, int32 bg, bool insideIsPositive, bool squared, bool useSpacing = false,
                             FloatVec3Type spacing = FloatVec3Type{1.0f, 1.0f, 1.0f})
{
  DataStore<T> inStore(ShapeType{dz, dy, dx}, ShapeType{1}, static_cast<T>(0));
  for(usize i = 0; i < pattern.size(); ++i)
  {
    inStore.setValue(i, static_cast<T>(pattern[i]));
  }
  DataStore<float32> outStore(ShapeType{dz, dy, dx}, ShapeType{1}, 0.0f);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  MaurerDistanceSlab<T> engine(inStore, outStore, SizeVec3Type{dx, dy, dz}, static_cast<T>(bg), insideIsPositive, squared, useSpacing, std::move(spacing), shouldCancel, messageHandler);
  const Result<> r = engine();
  REQUIRE(r.valid());
  std::vector<float32> out(pattern.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

// A deterministic mixed image: object blobs (value 1) on a background (0), with a couple of isolated object pixels.
std::vector<int32> MakePattern(usize dx, usize dy, usize dz)
{
  std::vector<int32> v(dx * dy * dz, 0);
  for(usize z = 0; z < dz; ++z)
  {
    for(usize y = 0; y < dy; ++y)
    {
      for(usize x = 0; x < dx; ++x)
      {
        // a filled quadrant is object; plus a checker of isolated object pixels elsewhere.
        const bool block = (x >= dx / 2) && (y >= dy / 2);
        const bool speck = ((x + 2 * y + 3 * z) % 5 == 0);
        v[FlatIndex(x, y, z, dx, dy)] = (block || speck) ? 1 : 0;
      }
    }
  }
  return v;
}

std::vector<int32> MakeInteriorPattern(usize dimX, usize dimY, usize dimZ)
{
  std::vector<int32> pattern(dimX * dimY * dimZ, 0);
  const auto setObject = [&](usize x, usize y, usize z) { pattern[FlatIndex(x, y, z, dimX, dimY)] = 1; };

  for(const usize z : {usize{0}, dimZ - 1})
  {
    for(const usize y : {usize{0}, dimY - 1})
    {
      for(const usize x : {usize{0}, dimX - 1})
      {
        setObject(x, y, z);
      }
    }
  }

  const usize centerX = dimX / 2;
  const usize centerY = dimY / 2;
  const usize centerZ = dimZ / 2;
  setObject(0, centerY, centerZ);
  setObject(dimX - 1, centerY, centerZ);
  setObject(centerX, 0, centerZ);
  setObject(centerX, dimY - 1, centerZ);
  setObject(centerX, centerY, 0);
  setObject(centerX, centerY, dimZ - 1);

  if(dimZ > 2)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        if(x == 0 || x == dimX - 1 || y == 0 || y == dimY - 1)
        {
          setObject(x, y, 1);
        }
      }
    }
  }

  if(dimX >= 5 && dimY > 2 && dimZ > 2)
  {
    for(usize x = centerX - 1; x <= centerX + 1; ++x)
    {
      setObject(x, centerY, centerZ);
    }
  }
  else if(dimY >= 5 && dimX > 2 && dimZ > 2)
  {
    for(usize y = centerY - 1; y <= centerY + 1; ++y)
    {
      setObject(centerX, y, centerZ);
    }
  }
  else if(dimZ >= 5 && dimX > 2 && dimY > 2)
  {
    for(usize z = centerZ - 1; z <= centerZ + 1; ++z)
    {
      setObject(centerX, centerY, z);
    }
  }

  return pattern;
}
} // namespace

TEMPLATE_TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: in-core == brute-force EDT oracle (no spacing)", "[NXImageProcessing][MaurerDistanceMapEngine]", uint8, int16, int32)
{
  using T = TestType;
  const bool insideIsPositive = GENERATE(false, true);
  const bool squared = GENERATE(false, true);
  CAPTURE(insideIsPositive, squared);

  auto check = [&](usize dx, usize dy, usize dz) {
    const std::vector<int32> pattern = MakePattern(dx, dy, dz);
    const std::vector<float32> oracle = MaurerOracle(pattern, dx, dy, dz, /*bg=*/0, insideIsPositive, squared);
    const std::vector<float32> got = RunInCore<T>(pattern, dx, dy, dz, /*bg=*/0, insideIsPositive, squared);
    const std::vector<float32> slab = RunSlab<T>(pattern, dx, dy, dz, /*bg=*/0, insideIsPositive, squared);
    REQUIRE(got.size() == oracle.size());
    REQUIRE(slab.size() == oracle.size());
    for(usize i = 0; i < got.size(); ++i)
    {
      INFO("index " << i << " dims " << dx << "x" << dy << "x" << dz);
      REQUIRE(got[i] == oracle[i]);  // in-core == oracle
      REQUIRE(slab[i] == oracle[i]); // OOC slab == oracle
      REQUIRE(slab[i] == got[i]);    // D3 gate: OOC == in-core, byte-identical
    }
  };

  SECTION("3D 5x6x7")
  {
    check(5, 6, 7);
  }
  SECTION("2D 9x8x1")
  {
    check(9, 8, 1);
  }
  SECTION("small 3x3x3")
  {
    check(3, 3, 3);
  }
}

TEMPLATE_TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: 3D interior scan matches the border scan and the oracle", "[NXImageProcessing][MaurerDistanceMapEngine]", uint8, int16)
{
  using T = TestType;
  const std::vector<SizeVec3Type> dimensions = {{5, 6, 7}, {3, 3, 3}, {4, 5, 3}, {1, 6, 7}, {2, 6, 7}, {7, 2, 7}};
  for(const SizeVec3Type& dims : dimensions)
  {
    DYNAMIC_SECTION(dims[0] << "x" << dims[1] << "x" << dims[2])
    {
      const std::vector<int32> pattern = MakeInteriorPattern(dims[0], dims[1], dims[2]);
      const std::vector<float32> oracle = MaurerOracle(pattern, dims[0], dims[1], dims[2], 0, false, true);
      const std::vector<float32> inCore = RunInCore<T>(pattern, dims[0], dims[1], dims[2], 0, false, true);
      const std::vector<float32> slab = RunSlab<T>(pattern, dims[0], dims[1], dims[2], 0, false, true);
      RequireBitIdentical(inCore, oracle);
      RequireBitIdentical(slab, oracle);
      RequireBitIdentical(slab, inCore);
    }
  }
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: every interior neighbour offset marks the centre as a boundary", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr int64 k_Dim5 = 5;
  constexpr int64 k_Center5 = 2;
  const uint32 expectedBoundaryBits = std::bit_cast<uint32>(ImageProcessing::detail::MaurerSignedValue(0.0f, true, false));

  auto checkCase = [expectedBoundaryBits](usize dimX, usize dimY, usize dimZ, usize holeX, usize holeY, usize holeZ) {
    std::vector<int32> pattern(dimX * dimY * dimZ, 1);
    pattern[FlatIndex(holeX, holeY, holeZ, dimX, dimY)] = 0;
    const std::vector<float32> oracle = MaurerOracle(pattern, dimX, dimY, dimZ, 0, false, true);
    const std::vector<float32> inCore = RunInCore<uint8>(pattern, dimX, dimY, dimZ, 0, false, true);
    const std::vector<float32> slab = RunSlab<uint8>(pattern, dimX, dimY, dimZ, 0, false, true);
    const usize centerIndex = FlatIndex(dimX / 2, dimY / 2, dimZ / 2, dimX, dimY);
    REQUIRE(std::bit_cast<uint32>(inCore[centerIndex]) == expectedBoundaryBits);
    REQUIRE(std::bit_cast<uint32>(slab[centerIndex]) == expectedBoundaryBits);
    RequireBitIdentical(inCore, oracle);
    RequireBitIdentical(slab, oracle);
  };

  for(int64 offsetZ = -1; offsetZ <= 1; ++offsetZ)
  {
    for(int64 offsetY = -1; offsetY <= 1; ++offsetY)
    {
      for(int64 offsetX = -1; offsetX <= 1; ++offsetX)
      {
        if(offsetX == 0 && offsetY == 0 && offsetZ == 0)
        {
          continue;
        }
        DYNAMIC_SECTION("5x5x5 offset (" << offsetX << ", " << offsetY << ", " << offsetZ << ")")
        {
          checkCase(k_Dim5, k_Dim5, k_Dim5, static_cast<usize>(k_Center5 + offsetX), static_cast<usize>(k_Center5 + offsetY), static_cast<usize>(k_Center5 + offsetZ));
        }
      }
    }
  }

  SECTION("3x3x3 smallest interior")
  {
    checkCase(3, 3, 3, 2, 1, 1);
  }
}

TEMPLATE_TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: encoded initialisation writes every work value", "[NXImageProcessing][MaurerDistanceMapEngine]", uint8, int16)
{
  using T = TestType;
  constexpr uint32 k_PoisonBits = 0x7FC00001u;
  const float32 poison = std::bit_cast<float32>(k_PoisonBits);
  const std::array<SizeVec3Type, 12> dimensions = {SizeVec3Type{1, 1, 1}, SizeVec3Type{1, 1, 7}, SizeVec3Type{7, 1, 1}, SizeVec3Type{1, 5, 1}, SizeVec3Type{5, 1, 7}, SizeVec3Type{5, 6, 1},
                                               SizeVec3Type{5, 6, 2}, SizeVec3Type{2, 6, 7}, SizeVec3Type{5, 2, 7}, SizeVec3Type{2, 2, 2}, SizeVec3Type{3, 3, 3}, SizeVec3Type{5, 6, 7}};

  for(const SizeVec3Type& dims : dimensions)
  {
    const std::array<std::vector<int32>, 2> patterns = {MakePattern(dims[0], dims[1], dims[2]), MakeInteriorPattern(dims[0], dims[1], dims[2])};
    for(usize patternIndex = 0; patternIndex < patterns.size(); ++patternIndex)
    {
      std::vector<T> input(patterns[patternIndex].size());
      std::transform(patterns[patternIndex].cbegin(), patterns[patternIndex].cend(), input.begin(), [](int32 value) { return static_cast<T>(value); });
      for(const bool insideIsPositive : {false, true})
      {
        CAPTURE(dims[0], dims[1], dims[2], patternIndex, insideIsPositive);
        const std::vector<float32> reference = BuildMaurerEncodedInitReference(input, T{0}, dims, insideIsPositive);
        const bool expectedHasBoundary = std::any_of(reference.cbegin(), reference.cend(), [](float32 value) { return value == 0.0f; });
        std::vector<float32> work(input.size(), poison);
        std::atomic_bool shouldCancel{false};
        const bool hasBoundary = ImageProcessing::detail::BuildMaurerEncodedInit<T>(nonstd::span<const T>(input.data(), input.size()), T{0}, dims, insideIsPositive,
                                                                                    nonstd::span<float32>(work.data(), work.size()), shouldCancel);
        REQUIRE(hasBoundary == expectedHasBoundary);
        for(usize index = 0; index < work.size(); ++index)
        {
          CAPTURE(index);
          REQUIRE(std::bit_cast<uint32>(work[index]) != k_PoisonBits);
          REQUIRE(std::bit_cast<uint32>(work[index]) == std::bit_cast<uint32>(reference[index]));
        }
      }
    }
  }
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: single-seed hand values", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  // A single object pixel at the center of a 5x5x1 background field. That pixel is a boundary feature (touches bg on
  // all sides), so distances are the squared distance to (2,2). Inside is only the center; everything else outside.
  constexpr usize k_Dx = 5;
  constexpr usize k_Dy = 5;
  constexpr usize k_Dz = 1;
  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, 0);
  pattern[FlatIndex(2, 2, 0, k_Dx, k_Dy)] = 1;
  const std::vector<float32> got = RunInCore<int32>(pattern, k_Dx, k_Dy, k_Dz, /*bg=*/0, /*insideIsPositive=*/false, /*squared=*/true);
  // Outside pixels are positive squared distance to (2,2); the center (inside) is 0 (it is itself a feature).
  REQUIRE(got[FlatIndex(2, 2, 0, k_Dx, k_Dy)] == 0.0f); // the seed
  REQUIRE(got[FlatIndex(0, 2, 0, k_Dx, k_Dy)] == 4.0f); // dx=2 -> 4
  REQUIRE(got[FlatIndex(0, 0, 0, k_Dx, k_Dy)] == 8.0f); // dx=2,dy=2 -> 8
  REQUIRE(got[FlatIndex(4, 4, 0, k_Dx, k_Dy)] == 8.0f); // symmetric corner
}

// The spacing case requires bit-identical slab and in-core results. It covers the Z-pass float conversion order.
// The unit-spacing oracle does not apply. This test covers types, distance forms, and image dimensions.
TEMPLATE_TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: slab == in-core with image spacing (D3)", "[NXImageProcessing][MaurerDistanceMapEngine]", uint8, int16, int32)
{
  using T = TestType;
  const bool insideIsPositive = GENERATE(false, true);
  const bool squared = GENERATE(false, true);
  const FloatVec3Type spacing{2.0f, 1.5f, 3.0f};
  CAPTURE(insideIsPositive, squared);

  auto check = [&](usize dx, usize dy, usize dz) {
    const std::vector<int32> pattern = MakePattern(dx, dy, dz);
    const std::vector<float32> inCore = RunInCore<T>(pattern, dx, dy, dz, /*bg=*/0, insideIsPositive, squared, /*useSpacing=*/true, spacing);
    const std::vector<float32> slab = RunSlab<T>(pattern, dx, dy, dz, /*bg=*/0, insideIsPositive, squared, /*useSpacing=*/true, spacing);
    REQUIRE(inCore.size() == slab.size());
    for(usize i = 0; i < inCore.size(); ++i)
    {
      INFO("index " << i << " dims " << dx << "x" << dy << "x" << dz);
      REQUIRE(slab[i] == inCore[i]);
    }
  };

  SECTION("3D 5x6x7")
  {
    check(5, 6, 7);
  }
  SECTION("2D 9x8x1")
  {
    check(9, 8, 1);
  }
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: featureless volume normalises to the in-core result on both routes", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  constexpr usize k_ValueCount = k_DimX * k_DimY * k_DimZ;
  constexpr float32 k_Max = std::numeric_limits<float32>::max();
  const FloatVec3Type spacing{0.5f, 2.0f, 1.25f};

  for(const int32 inputValue : {int32{0}, int32{1}})
  {
    for(const bool insideIsPositive : {false, true})
    {
      for(const bool squared : {true, false})
      {
        for(const bool useSpacing : {false, true})
        {
          CAPTURE(inputValue, insideIsPositive, squared, useSpacing);
          const std::vector<int32> pattern(k_ValueCount, inputValue);
          // ITK leaves +FLT_MAX for squared boundary-free images and signs the square root by input classification.
          const float32 root = static_cast<float32>(std::sqrt(static_cast<float64>(k_Max)));
          const std::array<std::array<float32, 2>, 2> signTable = {{{root, -root}, {-root, root}}};
          const float32 expectedValue = squared ? k_Max : signTable[inputValue == 0 ? 0 : 1][insideIsPositive ? 1 : 0];
          const std::vector<float32> expected(k_ValueCount, expectedValue);
          const std::vector<float32> inCore = RunInCore<uint8>(pattern, k_DimX, k_DimY, k_DimZ, 0, insideIsPositive, squared, useSpacing, spacing);
          const std::vector<float32> slab = RunSlab<uint8>(pattern, k_DimX, k_DimY, k_DimZ, 0, insideIsPositive, squared, useSpacing, spacing);
          RequireBitIdentical(inCore, expected);
          RequireBitIdentical(slab, expected);
          RequireBitIdentical(slab, inCore);
        }
      }
    }
  }
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: 2D planner bounds direct transpose batches", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  const auto benchmark = ImageProcessing::detail::BuildMaurer2DBufferPlan(5888, 5888, sizeof(uint8));
  REQUIRE(benchmark.valid);
  REQUIRE_FALSE(benchmark.spill_x);
  REQUIRE_FALSE(benchmark.spill_y);
  REQUIRE(benchmark.row_batch_rows > 1000);
  REQUIRE(benchmark.column_batch_cols > 1000);
  REQUIRE(benchmark.resident_bytes <= ImageProcessing::detail::k_Maurer2DResidentLimit);

  for(const usize inputBytes : {sizeof(uint8), sizeof(uint64)})
  {
    const auto stress = ImageProcessing::detail::BuildMaurer2DBufferPlan(16385, 1025, inputBytes);
    CAPTURE(inputBytes);
    REQUIRE(stress.valid);
    REQUIRE(stress.row_batch_rows > 0);
    REQUIRE(stress.column_batch_cols > 0);
    REQUIRE(stress.resident_bytes <= ImageProcessing::detail::k_Maurer2DResidentLimit);
  }

  const auto oneCell = ImageProcessing::detail::BuildMaurer2DBufferPlan(1, 1, sizeof(uint64));
  REQUIRE(oneCell.valid);
  REQUIRE(oneCell.resident_bytes <= ImageProcessing::detail::k_Maurer2DResidentLimit);
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: 2D planner selects spill lines", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  const auto xPlan = ImageProcessing::detail::BuildMaurer2DBufferPlan(100000000, 1, sizeof(uint8));
  const auto yPlan = ImageProcessing::detail::BuildMaurer2DBufferPlan(1, 100000000, sizeof(uint8));
  REQUIRE(xPlan.valid);
  REQUIRE(yPlan.valid);
  REQUIRE(xPlan.spill_x);
  REQUIRE(yPlan.spill_y);
  REQUIRE(xPlan.line_block_values >= 2);
  REQUIRE(yPlan.line_block_values >= 2);
  REQUIRE(xPlan.resident_bytes <= ImageProcessing::detail::k_Maurer2DResidentLimit);
  REQUIRE(yPlan.resident_bytes <= ImageProcessing::detail::k_Maurer2DResidentLimit);

  constexpr usize k_SmallTestLimit = 8192;
  const auto forcedX = ImageProcessing::detail::BuildMaurer2DBufferPlan(100, 8, sizeof(uint8), k_SmallTestLimit);
  const auto forcedY = ImageProcessing::detail::BuildMaurer2DBufferPlan(8, 100, sizeof(uint8), k_SmallTestLimit);
  REQUIRE(forcedX.valid);
  REQUIRE(forcedX.spill_x);
  REQUIRE(forcedX.spill_y);
  REQUIRE(forcedX.resident_bytes <= k_SmallTestLimit);
  REQUIRE(forcedY.valid);
  REQUIRE_FALSE(forcedY.spill_x);
  REQUIRE(forcedY.spill_y);
  REQUIRE(forcedY.resident_bytes <= k_SmallTestLimit);
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: 2D planner rejects overflow", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  const auto overflow = ImageProcessing::detail::BuildMaurer2DBufferPlan(std::numeric_limits<usize>::max(), std::numeric_limits<usize>::max(), sizeof(uint8));
  const auto zeroX = ImageProcessing::detail::BuildMaurer2DBufferPlan(0, 1, sizeof(uint8));
  const auto zeroY = ImageProcessing::detail::BuildMaurer2DBufferPlan(1, 0, sizeof(uint8));
  REQUIRE_FALSE(overflow.valid);
  REQUIRE(overflow.overflow);
  REQUIRE_FALSE(zeroX.valid);
  REQUIRE(zeroX.overflow);
  REQUIRE_FALSE(zeroY.valid);
  REQUIRE(zeroY.overflow);
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: working format follows OOC endpoint precedence", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  using StoreType = IDataStore::StoreType;
  REQUIRE(ImageProcessing::detail::SelectDistanceWorkingDataFormat(StoreType::InMemory, "input", StoreType::InMemory, "output") == "input");
  REQUIRE(ImageProcessing::detail::SelectDistanceWorkingDataFormat(StoreType::OutOfCore, "input-ooc", StoreType::OutOfCore, "output-ooc") == "input-ooc");
  REQUIRE(ImageProcessing::detail::SelectDistanceWorkingDataFormat(StoreType::InMemory, "input", StoreType::OutOfCore, "output-ooc") == "output-ooc");
  REQUIRE(ImageProcessing::detail::SelectDistanceWorkingDataFormat(StoreType::OutOfCore, "input-ooc", StoreType::InMemory, "output") == "input-ooc");
  DataStore<uint8> residentStore(ShapeType{1}, ShapeType{1}, 0);
  const IDataStore& abstractResidentStore = residentStore;
  REQUIRE_FALSE(abstractResidentStore.getChunkShape().has_value());
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: 3D slab plan charges two staging buffers", "[NXImageProcessing][MaurerDistanceMapEngine][WorkingMemory]")
{
  const SizeVec3Type dims{5, 6, 7};
  for(const usize rows : {usize{1}, usize{3}, usize{6}})
  {
    const usize grant = PlanGrant(dims, rows);
    auto planResult = ImageProcessing::detail::CreateMaurer3DSlabMemoryPlan<uint8>(dims, grant);
    SIMPLNX_RESULT_REQUIRE_VALID(planResult);
    REQUIRE(planResult.value().max_y_rows == rows);
    REQUIRE(planResult.value().staging_values == rows * 5 * 7);
    REQUIRE(planResult.value().resident_bytes == grant);
    REQUIRE(planResult.value().resident_bytes <= grant);
  }

  auto minimumGrantPlanResult = ImageProcessing::detail::CreateMaurer3DSlabMemoryPlan<uint8>(dims, PlanGrant(dims, 1) - 1);
  SIMPLNX_RESULT_REQUIRE_INVALID(minimumGrantPlanResult);

  const SizeVec3Type largeDims{512, 512, 128};
  const usize largeGrant = PlanGrant(largeDims, 512);
  auto largePlanResult = ImageProcessing::detail::CreateMaurer3DSlabMemoryPlan<uint8>(largeDims, largeGrant);
  SIMPLNX_RESULT_REQUIRE_VALID(largePlanResult);
  REQUIRE(largePlanResult.value().max_y_rows == 512);
  REQUIRE(largePlanResult.value().staging_values == 512 * 512 * 128);
  REQUIRE(largePlanResult.value().resident_bytes == largeGrant);
  REQUIRE(largePlanResult.value().resident_bytes <= largeGrant);
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: resident state requires a complete dataset-scaled reservation", "[NXImageProcessing][MaurerDistanceMapEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 512;
  constexpr usize k_DimY = 512;
  constexpr usize k_DimZ = 128;
  constexpr usize k_ValueCount = k_DimX * k_DimY * k_DimZ;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  constexpr uint64 k_GiB = 1024ULL * k_MiB;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};

  auto requiredResult = ImageProcessing::detail::CalculateMaurerResidentWorkingMemoryBytes<uint8>(dims);
  SIMPLNX_RESULT_REQUIRE_VALID(requiredResult);
  REQUIRE(requiredResult.value() == k_ValueCount * (sizeof(uint8) + sizeof(float32)));

  auto halfResult = ImageProcessing::detail::CalculateMaurerResidentWorkingMemoryBytes<uint8>(SizeVec3Type{k_DimX, k_DimY, k_DimZ / 2});
  SIMPLNX_RESULT_REQUIRE_VALID(halfResult);
  REQUIRE(halfResult.value() * 2 == requiredResult.value());
  Result<usize> overflowDimsResult = ImageProcessing::detail::CalculateMaurerResidentWorkingMemoryBytes<uint8>(SizeVec3Type{std::numeric_limits<usize>::max(), 2, 2});
  SIMPLNX_RESULT_REQUIRE_INVALID(overflowDimsResult);

  REQUIRE(ImageProcessing::detail::ShouldUseMaurerResidentState(dims));
  REQUIRE_FALSE(ImageProcessing::detail::ShouldUseMaurerResidentState(SizeVec3Type{k_DimX, k_DimY, 1}));

  auto smallSlabPlan = ImageProcessing::detail::CreateMaurer3DSlabMemoryPlan<uint8>(dims, 16 * k_MiB);
  auto mediumSlabPlan = ImageProcessing::detail::CreateMaurer3DSlabMemoryPlan<uint8>(dims, 128 * k_MiB);
  const usize fullSlabGrant = PlanGrant(dims, k_DimY);
  auto fullSlabPlan = ImageProcessing::detail::CreateMaurer3DSlabMemoryPlan<uint8>(dims, fullSlabGrant);
  SIMPLNX_RESULT_REQUIRE_VALID(smallSlabPlan);
  SIMPLNX_RESULT_REQUIRE_VALID(mediumSlabPlan);
  SIMPLNX_RESULT_REQUIRE_VALID(fullSlabPlan);
  const usize expectedWorkerCount = std::max<usize>(1, static_cast<usize>(std::thread::hardware_concurrency()));
  REQUIRE(smallSlabPlan.value().worker_count == expectedWorkerCount);
  REQUIRE(mediumSlabPlan.value().worker_count == expectedWorkerCount);
  REQUIRE(fullSlabPlan.value().worker_count == expectedWorkerCount);
  REQUIRE(smallSlabPlan.value().max_y_rows > 0);
  REQUIRE(mediumSlabPlan.value().max_y_rows > smallSlabPlan.value().max_y_rows);
  REQUIRE(fullSlabPlan.value().max_y_rows == k_DimY);
  auto requireMinimumRowsForBatchCount = [k_DimY](const ImageProcessing::detail::Maurer3DSlabMemoryPlan& plan) {
    const usize batchCount = (k_DimY + plan.max_y_rows - 1) / plan.max_y_rows;
    REQUIRE(plan.max_y_rows == (k_DimY + batchCount - 1) / batchCount);
  };
  requireMinimumRowsForBatchCount(smallSlabPlan.value());
  requireMinimumRowsForBatchCount(mediumSlabPlan.value());
  requireMinimumRowsForBatchCount(fullSlabPlan.value());
  REQUIRE(smallSlabPlan.value().resident_bytes <= 16 * k_MiB);
  REQUIRE(mediumSlabPlan.value().resident_bytes <= 128 * k_MiB);
  REQUIRE(fullSlabPlan.value().resident_bytes <= fullSlabGrant);
  auto undersizedGrantPlanResult = ImageProcessing::detail::CreateMaurer3DSlabMemoryPlan<uint8>(dims, 1);
  SIMPLNX_RESULT_REQUIRE_INVALID(undersizedGrantPlanResult);

  auto& manager = CacheMemoryBudgetManager::Instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  manager.setBudgetBytes(512 * k_MiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveMaurerResidentWorkingMemory<uint8>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE_FALSE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == 128 * k_MiB);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

  manager.setBudgetBytes(k_GiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveMaurerResidentWorkingMemory<uint8>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == requiredResult.value());
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: 3D chunk hint follows the bounded Y plan", "[NXImageProcessing][MaurerDistanceMapEngine][WorkingMemory]")
{
  const SizeVec3Type dims{512, 512, 128};
  constexpr usize k_GrantBytes = 16ULL * 1024ULL * 1024ULL;
  auto planResult = ImageProcessing::detail::CreateMaurer3DSlabMemoryPlan<uint8>(dims, k_GrantBytes);
  SIMPLNX_RESULT_REQUIRE_VALID(planResult);

  auto hintResult = ImageProcessing::detail::CreateMaurer3DChunkHint<uint8>(dims, k_GrantBytes);
  SIMPLNX_RESULT_REQUIRE_VALID(hintResult);
  REQUIRE(hintResult.value() == ShapeType{1, planResult.value().max_y_rows, dims[0]});
  REQUIRE(hintResult.value()[0] * hintResult.value()[1] * hintResult.value()[2] * sizeof(float32) > 0);
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: 3D batch alignment preserves a short fallback", "[NXImageProcessing][MaurerDistanceMapEngine][WorkingMemory]")
{
  REQUIRE(ImageProcessing::detail::AlignMaurer3DYBatchRows(120, 7) == 119);
  REQUIRE(ImageProcessing::detail::AlignMaurer3DYBatchRows(6, 7) == 6);
  REQUIRE(ImageProcessing::detail::AlignMaurer3DYBatchRows(57, 57) == 57);
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: 3D scratch inherits a valid output chunk hint", "[NXImageProcessing][MaurerDistanceMapEngine][WorkingMemory]")
{
  const ShapeType outputChunkShape{1, 7, 32};
  const ShapeType scratchTupleShape{8, 17, 32};
  const auto scratchHint = ImageProcessing::detail::SelectMaurer3DScratchChunkHint(outputChunkShape, scratchTupleShape);
  REQUIRE(scratchHint.has_value());
  REQUIRE(*scratchHint == outputChunkShape);
  REQUIRE_FALSE(ImageProcessing::detail::SelectMaurer3DScratchChunkHint(ShapeType{1, 18, 32}, scratchTupleShape).has_value());
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: bounded 3D reads each input plane once", "[NXImageProcessing][MaurerDistanceMapEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  constexpr uint64 k_PartialBudgetBytes = 1024;
  constexpr uint64 k_CompleteBudgetBytes = 8192;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);
  const std::vector<float32> expected = RunInCore<uint8>(pattern, k_DimX, k_DimY, k_DimZ, 0, false, true);

  auto& manager = CacheMemoryBudgetManager::Instance();
  const uint64 previousBudget = manager.budgetBytes();

  auto run = [&](uint64 budgetBytes) {
    manager.clear();
    manager.setBudgetBytes(budgetBytes);
    ReadCountingDataStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0});
    DataStore<float32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f);
    for(usize index = 0; index < pattern.size(); ++index)
    {
      inputStore.setValue(index, static_cast<uint8>(pattern[index]));
    }

    std::atomic_bool shouldCancel{false};
    IFilter::MessageHandler messageHandler{};
    MaurerDistanceWorkingMemory<uint8> engine(inputStore, outputStore, dims, uint8{0}, false, true, false, FloatVec3Type{1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler);
    Result<> engineResult = engine();
    SIMPLNX_RESULT_REQUIRE_VALID(engineResult);

    std::vector<float32> actual(pattern.size());
    Result<> copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<float32>(actual.data(), actual.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
    RequireBitIdentical(actual, expected);
    REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
    return inputStore.readCount();
  };

  auto runOutOfCore = [&]() {
    manager.clear();
    manager.setBudgetBytes(k_CompleteBudgetBytes);
    OocReportingDataStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0});
    DataStore<float32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f);
    for(usize index = 0; index < pattern.size(); ++index)
    {
      inputStore.setValue(index, static_cast<uint8>(pattern[index]));
    }

    std::atomic_bool shouldCancel{false};
    IFilter::MessageHandler messageHandler{};
    MaurerDistanceWorkingMemory<uint8> engine(inputStore, outputStore, dims, uint8{0}, false, true, false, FloatVec3Type{1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler);
    Result<> engineResult = engine();
    SIMPLNX_RESULT_REQUIRE_VALID(engineResult);

    std::vector<float32> actual(pattern.size());
    Result<> copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<float32>(actual.data(), actual.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
    RequireBitIdentical(actual, expected);
    REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
    return inputStore.readCount();
  };

  REQUIRE(run(k_PartialBudgetBytes) == k_DimZ);
  REQUIRE(run(k_CompleteBudgetBytes) == 0);
  REQUIRE(runOutOfCore() == 1);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: slab cancellation after the third plane read leaves the output untouched", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  constexpr float32 k_Poison = -999.0f;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);
  std::atomic_bool shouldCancel{false};
  CancelOnThirdReadDataStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0}, shouldCancel);
  WriteCountingDataStoreType<float32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, k_Poison);
  for(usize index = 0; index < pattern.size(); ++index)
  {
    inputStore.setValue(index, static_cast<uint8>(pattern[index]));
  }

  IFilter::MessageHandler messageHandler{};
  MaurerDistanceSlab<uint8> engine(inputStore, outputStore, dims, uint8{0}, false, true, false, FloatVec3Type{1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler);
  Result<> engineResult = engine();
  SIMPLNX_RESULT_REQUIRE_VALID(engineResult);
  REQUIRE(inputStore.readCount() == 3);
  REQUIRE(outputStore.flatWriteCount() == 0);
  REQUIRE(outputStore.extentWriteCount() == 0);
  for(usize index = 0; index < pattern.size(); ++index)
  {
    CAPTURE(index);
    REQUIRE(outputStore.getValue(index) == k_Poison);
  }
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: non-DataStore in-memory input copies once and matches the borrowed-span route", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);
  const std::vector<float32> expected = RunInCore<uint8>(pattern, k_DimX, k_DimY, k_DimZ, 0, false, true);
  CountingForwardingDataStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0});
  DataStore<float32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f);
  for(usize index = 0; index < pattern.size(); ++index)
  {
    inputStore.setValue(index, static_cast<uint8>(pattern[index]));
  }

  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  MaurerDistanceInCore<uint8> engine(inputStore, outputStore, dims, uint8{0}, false, true, false, FloatVec3Type{1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler);
  Result<> engineResult = engine();
  SIMPLNX_RESULT_REQUIRE_VALID(engineResult);
  REQUIRE(inputStore.readCount() == 1);
  std::vector<float32> actual(pattern.size());
  Result<> copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<float32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  RequireBitIdentical(actual, expected);
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: pre-cancelled copied-input route performs no transfers", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  constexpr float32 k_Poison = -999.0f;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);
  CountingForwardingDataStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0});
  WriteCountingDataStoreType<float32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, k_Poison);
  for(usize index = 0; index < pattern.size(); ++index)
  {
    inputStore.setValue(index, static_cast<uint8>(pattern[index]));
  }

  std::atomic_bool shouldCancel{true};
  IFilter::MessageHandler messageHandler{};
  MaurerDistanceInCore<uint8> engine(inputStore, outputStore, dims, uint8{0}, false, true, false, FloatVec3Type{1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler);
  Result<> engineResult = engine();
  SIMPLNX_RESULT_REQUIRE_VALID(engineResult);
  REQUIRE(inputStore.readCount() == 0);
  REQUIRE(outputStore.flatWriteCount() == 0);
  REQUIRE(outputStore.extentWriteCount() == 0);
  for(usize index = 0; index < pattern.size(); ++index)
  {
    CAPTURE(index);
    REQUIRE(outputStore.getValue(index) == k_Poison);
  }
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: pre-cancelled borrowed-span route performs no transfers", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  constexpr float32 k_Poison = -999.0f;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);
  ReadCountingDataStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0});
  WriteCountingDataStoreType<float32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, k_Poison);
  for(usize index = 0; index < pattern.size(); ++index)
  {
    inputStore.setValue(index, static_cast<uint8>(pattern[index]));
  }

  std::atomic_bool shouldCancel{true};
  IFilter::MessageHandler messageHandler{};
  MaurerDistanceInCore<uint8> engine(inputStore, outputStore, dims, uint8{0}, false, true, false, FloatVec3Type{1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler);
  Result<> engineResult = engine();
  SIMPLNX_RESULT_REQUIRE_VALID(engineResult);
  REQUIRE(inputStore.readCount() == 0);
  REQUIRE(outputStore.flatWriteCount() == 0);
  REQUIRE(outputStore.extentWriteCount() == 0);
  for(usize index = 0; index < pattern.size(); ++index)
  {
    CAPTURE(index);
    REQUIRE(outputStore.getValue(index) == k_Poison);
  }
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: bounded 3D Z pass transfers each Y batch as one extent", "[NXImageProcessing][MaurerDistanceMapEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);
  const std::vector<float32> expected = RunInCore<uint8>(pattern, k_DimX, k_DimY, k_DimZ, 0, false, true);

  auto& manager = CacheMemoryBudgetManager::Instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  manager.setBudgetBytes(1024);

  ReadCountingDataStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0});
  ExtentCountingDataStore<float32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, -1.0f);
  for(usize index = 0; index < pattern.size(); ++index)
  {
    inputStore.setValue(index, static_cast<uint8>(pattern[index]));
  }

  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  MaurerDistanceWorkingMemory<uint8> engine(inputStore, outputStore, dims, uint8{0}, false, true, false, FloatVec3Type{1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler);
  Result<> engineResult = engine();
  SIMPLNX_RESULT_REQUIRE_VALID(engineResult);

  std::vector<float32> actual(pattern.size());
  Result<> copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<float32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(actual == expected);
  REQUIRE(outputStore.extentWriteCount() == 1);
  REQUIRE(outputStore.flatWriteCount() == 0);
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: XY stream and Z pass transfer exact ranges", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  struct ExactTransferCase
  {
    usize dim_z;
    std::vector<std::pair<usize, usize>> plane_ranges;
    std::vector<std::pair<usize, usize>> work_read_ranges;
  };
  const std::vector<ExactTransferCase> testCases = {
      {.dim_z = 2, .plane_ranges = {{0, 30}, {30, 30}}, .work_read_ranges = {{0, 15}, {30, 15}, {15, 15}, {45, 15}}},
      {.dim_z = 3, .plane_ranges = {{0, 30}, {30, 30}, {60, 30}}, .work_read_ranges = {{0, 15}, {30, 15}, {60, 15}, {15, 15}, {45, 15}, {75, 15}}},
      {.dim_z = 7,
       .plane_ranges = {{0, 30}, {30, 30}, {60, 30}, {90, 30}, {120, 30}, {150, 30}, {180, 30}},
       .work_read_ranges = {{0, 15}, {30, 15}, {60, 15}, {90, 15}, {120, 15}, {150, 15}, {180, 15}, {15, 15}, {45, 15}, {75, 15}, {105, 15}, {135, 15}, {165, 15}, {195, 15}}}};

  for(const ExactTransferCase& testCase : testCases)
  {
    DYNAMIC_SECTION(k_DimX << "x" << k_DimY << "x" << testCase.dim_z)
    {
      const SizeVec3Type dims{k_DimX, k_DimY, testCase.dim_z};
      const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, testCase.dim_z);
      const std::vector<float32> expected = RunInCore<uint8>(pattern, k_DimX, k_DimY, testCase.dim_z, 0, false, true);
      TransferLog log;
      std::atomic_bool shouldCancel{false};
      RecordingInputStore<uint8> inputStore(ShapeType{testCase.dim_z, k_DimY, k_DimX}, ShapeType{1}, uint8{0}, log, shouldCancel);
      RecordingWorkStore workStore(ShapeType{testCase.dim_z, k_DimY, k_DimX}, ShapeType{1}, 0.0f, log, shouldCancel);
      RecordingOutputStore outputStore(ShapeType{testCase.dim_z, k_DimY, k_DimX}, ShapeType{1}, -999.0f, log, shouldCancel);
      const ImageProcessing::detail::Maurer3DSlabParameters<uint8> parameters{
          .dims = dims, .background_value = uint8{0}, .inside_is_positive = false, .squared_distance = true, .use_spacing = false, .spacing = FloatVec3Type{1.0f, 1.0f, 1.0f}, .max_y_rows = 3, .worker_count = 2};

      const SeamResults results = RunSeam(pattern, dims, parameters, log, inputStore, workStore, outputStore, shouldCancel);
      SIMPLNX_RESULT_REQUIRE_VALID(results.xy_result);
      REQUIRE(results.z_ran);
      SIMPLNX_RESULT_REQUIRE_VALID(results.z_result);
      REQUIRE(GetTransferRanges(GetTransferEvents(log, TransferEvent::Kind::InputRead)) == testCase.plane_ranges);
      REQUIRE(GetTransferRanges(GetTransferEvents(log, TransferEvent::Kind::WorkWrite)) == testCase.plane_ranges);
      REQUIRE(GetTransferRanges(GetTransferEvents(log, TransferEvent::Kind::WorkRead)) == testCase.work_read_ranges);

      const std::vector<TransferEvent> outputExtents = GetTransferEvents(log, TransferEvent::Kind::OutputExtent);
      REQUIRE(outputExtents.size() == 2);
      REQUIRE(outputExtents[0].extent.min == std::vector<uint64>{0, 0, 0});
      REQUIRE(outputExtents[0].extent.max == std::vector<uint64>{static_cast<uint64>(testCase.dim_z - 1), 2, 4});
      REQUIRE(outputExtents[0].count == 3 * k_DimX * testCase.dim_z);
      REQUIRE(outputExtents[1].extent.min == std::vector<uint64>{0, 3, 0});
      REQUIRE(outputExtents[1].extent.max == std::vector<uint64>{static_cast<uint64>(testCase.dim_z - 1), 5, 4});
      REQUIRE(outputExtents[1].count == 3 * k_DimX * testCase.dim_z);
      REQUIRE(outputStore.flatWriteCount() == 0);
      RequireBitIdentical(ReadOutput(outputStore, pattern.size()), expected);
    }
  }
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: single store call in flight", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);

  for(const usize maxYRows : {usize{3}, usize{1}, usize{6}})
  {
    CAPTURE(maxYRows);
    TransferLog log;
    std::atomic_bool shouldCancel{false};
    RecordingInputStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0}, log, shouldCancel);
    RecordingWorkStore workStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f, log, shouldCancel);
    RecordingOutputStore outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, -999.0f, log, shouldCancel);
    const ImageProcessing::detail::Maurer3DSlabParameters<uint8> parameters{.dims = dims,
                                                                            .background_value = uint8{0},
                                                                            .inside_is_positive = false,
                                                                            .squared_distance = true,
                                                                            .use_spacing = false,
                                                                            .spacing = FloatVec3Type{1.0f, 1.0f, 1.0f},
                                                                            .max_y_rows = maxYRows,
                                                                            .worker_count = 2};

    const SeamResults results = RunSeam(pattern, dims, parameters, log, inputStore, workStore, outputStore, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_VALID(results.xy_result);
    REQUIRE(results.z_ran);
    SIMPLNX_RESULT_REQUIRE_VALID(results.z_result);
    REQUIRE(log.active.load() == 0);
    REQUIRE(log.max_active.load() == 1);
  }
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: cancellation during a prefetched plane read", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  constexpr float32 k_Poison = -999.0f;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);
  TransferLog log;
  std::atomic_bool shouldCancel{false};
  RecordingInputStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0}, log, shouldCancel, std::nullopt, 3);
  RecordingWorkStore workStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f, log, shouldCancel);
  RecordingOutputStore outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, k_Poison, log, shouldCancel);
  const ImageProcessing::detail::Maurer3DSlabParameters<uint8> parameters{
      .dims = dims, .background_value = uint8{0}, .inside_is_positive = false, .squared_distance = true, .use_spacing = false, .spacing = FloatVec3Type{1.0f, 1.0f, 1.0f}, .max_y_rows = 3, .worker_count = 2};

  const SeamResults results = RunSeam(pattern, dims, parameters, log, inputStore, workStore, outputStore, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(results.xy_result);
  REQUIRE(results.z_ran);
  SIMPLNX_RESULT_REQUIRE_VALID(results.z_result);
  REQUIRE(GetTransferEvents(log, TransferEvent::Kind::InputRead).size() == 3);
  REQUIRE(GetTransferEvents(log, TransferEvent::Kind::WorkWrite).size() <= 2);
  REQUIRE(GetTransferEvents(log, TransferEvent::Kind::OutputExtent).empty());
  const std::vector<float32> output = ReadOutput(outputStore, pattern.size());
  REQUIRE(std::all_of(output.cbegin(), output.cend(), [](float32 value) { return value == k_Poison; }));
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: cancellation during the first output extent write", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);
  TransferLog log;
  std::atomic_bool shouldCancel{false};
  RecordingInputStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0}, log, shouldCancel);
  RecordingWorkStore workStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f, log, shouldCancel);
  RecordingOutputStore outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, -999.0f, log, shouldCancel, 1);
  const ImageProcessing::detail::Maurer3DSlabParameters<uint8> parameters{
      .dims = dims, .background_value = uint8{0}, .inside_is_positive = false, .squared_distance = true, .use_spacing = false, .spacing = FloatVec3Type{1.0f, 1.0f, 1.0f}, .max_y_rows = 2, .worker_count = 2};

  const SeamResults results = RunSeam(pattern, dims, parameters, log, inputStore, workStore, outputStore, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(results.xy_result);
  REQUIRE(results.z_ran);
  SIMPLNX_RESULT_REQUIRE_VALID(results.z_result);
  REQUIRE(GetTransferEvents(log, TransferEvent::Kind::OutputExtent).size() == 1);
  const std::vector<TransferEvent> workReads = GetTransferEvents(log, TransferEvent::Kind::WorkRead);
  REQUIRE(workReads.size() == 14);
  REQUIRE(std::none_of(workReads.cbegin(), workReads.cend(), [](const TransferEvent& event) { return event.start % 30 == 20; }));
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: cancellation during a scratch-band read stops after the previous extent", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 9;
  constexpr usize k_DimZ = 4;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);
  TransferLog log;
  std::atomic_bool shouldCancel{false};
  RecordingInputStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0}, log, shouldCancel);
  RecordingWorkStore workStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f, log, shouldCancel, std::nullopt, std::nullopt, 9);
  RecordingOutputStore outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, -999.0f, log, shouldCancel);
  const ImageProcessing::detail::Maurer3DSlabParameters<uint8> parameters{
      .dims = dims, .background_value = uint8{0}, .inside_is_positive = false, .squared_distance = true, .use_spacing = false, .spacing = FloatVec3Type{1.0f, 1.0f, 1.0f}, .max_y_rows = 3, .worker_count = 2};

  const SeamResults results = RunSeam(pattern, dims, parameters, log, inputStore, workStore, outputStore, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(results.xy_result);
  REQUIRE(results.z_ran);
  SIMPLNX_RESULT_REQUIRE_VALID(results.z_result);
  const std::vector<TransferEvent> workReads = GetTransferEvents(log, TransferEvent::Kind::WorkRead);
  REQUIRE(workReads.size() == 9);
  REQUIRE(workReads.back().start == 30);
  REQUIRE(GetTransferEvents(log, TransferEvent::Kind::OutputExtent).size() == 1);
  REQUIRE(outputStore.completedExtentCount() == 1);
  const std::vector<TransferEvent> events = GetAllTransferEvents(log);
  REQUIRE(events.back().kind == TransferEvent::Kind::WorkRead);
  REQUIRE(events.back().start == 30);
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: input read error stops the run before any output", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);
  TransferLog log;
  std::atomic_bool shouldCancel{false};
  RecordingInputStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0}, log, shouldCancel, 5);
  RecordingWorkStore workStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f, log, shouldCancel);
  RecordingOutputStore outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, -999.0f, log, shouldCancel);
  const ImageProcessing::detail::Maurer3DSlabParameters<uint8> parameters{
      .dims = dims, .background_value = uint8{0}, .inside_is_positive = false, .squared_distance = true, .use_spacing = false, .spacing = FloatVec3Type{1.0f, 1.0f, 1.0f}, .max_y_rows = 3, .worker_count = 2};

  const SeamResults results = RunSeam(pattern, dims, parameters, log, inputStore, workStore, outputStore, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(results.xy_result);
  REQUIRE(results.xy_result.errors().size() == 1);
  REQUIRE(results.xy_result.errors().front().code == -8399);
  REQUIRE_FALSE(results.z_ran);
  REQUIRE(GetTransferEvents(log, TransferEvent::Kind::OutputExtent).empty());
  const std::vector<TransferEvent> workWrites = GetTransferEvents(log, TransferEvent::Kind::WorkWrite);
  REQUIRE(workWrites.size() <= 3);
  REQUIRE(std::none_of(workWrites.cbegin(), workWrites.cend(), [](const TransferEvent& event) { return event.start >= 120; }));
  const std::vector<TransferEvent> events = GetAllTransferEvents(log);
  REQUIRE(events.back().kind == TransferEvent::Kind::InputRead);
  REQUIRE(events.back().start == 120);
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: scratch-plane write error stops the XY stream", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 4;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);
  TransferLog log;
  std::atomic_bool shouldCancel{false};
  RecordingInputStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0}, log, shouldCancel);
  RecordingWorkStore workStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f, log, shouldCancel, std::nullopt, 3);
  RecordingOutputStore outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, -999.0f, log, shouldCancel);
  const ImageProcessing::detail::Maurer3DSlabParameters<uint8> parameters{
      .dims = dims, .background_value = uint8{0}, .inside_is_positive = false, .squared_distance = true, .use_spacing = false, .spacing = FloatVec3Type{1.0f, 1.0f, 1.0f}, .max_y_rows = 3, .worker_count = 2};

  const SeamResults results = RunSeam(pattern, dims, parameters, log, inputStore, workStore, outputStore, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(results.xy_result);
  REQUIRE(results.xy_result.errors().size() == 1);
  REQUIRE(results.xy_result.errors().front().code == -8398);
  REQUIRE_FALSE(results.z_ran);
  const std::vector<TransferEvent> workWrites = GetTransferEvents(log, TransferEvent::Kind::WorkWrite);
  REQUIRE(workWrites.size() == 3);
  REQUIRE(workWrites.back().start == 60);
  REQUIRE(GetTransferEvents(log, TransferEvent::Kind::InputRead).size() <= 4);
  REQUIRE(GetTransferEvents(log, TransferEvent::Kind::OutputExtent).empty());
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: work read error stops the Z pass", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);
  TransferLog log;
  std::atomic_bool shouldCancel{false};
  RecordingInputStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0}, log, shouldCancel);
  RecordingWorkStore workStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f, log, shouldCancel, 9);
  RecordingOutputStore outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, -999.0f, log, shouldCancel);
  const ImageProcessing::detail::Maurer3DSlabParameters<uint8> parameters{
      .dims = dims, .background_value = uint8{0}, .inside_is_positive = false, .squared_distance = true, .use_spacing = false, .spacing = FloatVec3Type{1.0f, 1.0f, 1.0f}, .max_y_rows = 3, .worker_count = 2};

  const SeamResults results = RunSeam(pattern, dims, parameters, log, inputStore, workStore, outputStore, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(results.xy_result);
  REQUIRE(results.z_ran);
  SIMPLNX_RESULT_REQUIRE_INVALID(results.z_result);
  REQUIRE(results.z_result.errors().size() == 1);
  REQUIRE(results.z_result.errors().front().code == -8399);
  const std::vector<TransferEvent> workReads = GetTransferEvents(log, TransferEvent::Kind::WorkRead);
  REQUIRE(workReads.size() <= 9);
  REQUIRE(workReads.back().start == 45);
  REQUIRE(GetTransferEvents(log, TransferEvent::Kind::OutputExtent).size() <= 1);
  const std::vector<TransferEvent> events = GetAllTransferEvents(log);
  REQUIRE(events.back().kind == TransferEvent::Kind::WorkRead);
  REQUIRE(events.back().start == 45);
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: store exceptions become error results", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::vector<int32> pattern = MakePattern(k_DimX, k_DimY, k_DimZ);

  SECTION("input read")
  {
    TransferLog log;
    std::atomic_bool shouldCancel{false};
    RecordingInputStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0}, log, shouldCancel, std::nullopt, std::nullopt, 2);
    RecordingWorkStore workStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f, log, shouldCancel);
    RecordingOutputStore outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, -999.0f, log, shouldCancel);
    const ImageProcessing::detail::Maurer3DSlabParameters<uint8> parameters{
        .dims = dims, .background_value = uint8{0}, .inside_is_positive = false, .squared_distance = true, .use_spacing = false, .spacing = FloatVec3Type{1.0f, 1.0f, 1.0f}, .max_y_rows = 3, .worker_count = 2};

    const SeamResults results = RunSeam(pattern, dims, parameters, log, inputStore, workStore, outputStore, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_INVALID(results.xy_result);
    REQUIRE_FALSE(results.xy_result.errors().empty());
    REQUIRE(results.xy_result.errors().front().code < 0);
    REQUIRE_FALSE(results.z_ran);
    REQUIRE(GetTransferEvents(log, TransferEvent::Kind::InputRead).size() == 2);
    REQUIRE(GetTransferEvents(log, TransferEvent::Kind::WorkWrite).empty());
    REQUIRE(GetTransferEvents(log, TransferEvent::Kind::OutputExtent).empty());
    const std::vector<TransferEvent> events = GetAllTransferEvents(log);
    REQUIRE(events.back().kind == TransferEvent::Kind::InputRead);
    REQUIRE(log.active.load() == 0);
  }

  SECTION("work write")
  {
    constexpr usize k_WriteDimZ = 4;
    const SizeVec3Type writeDims{k_DimX, k_DimY, k_WriteDimZ};
    const std::vector<int32> writePattern = MakePattern(k_DimX, k_DimY, k_WriteDimZ);
    TransferLog log;
    std::atomic_bool shouldCancel{false};
    RecordingInputStore<uint8> inputStore(ShapeType{k_WriteDimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0}, log, shouldCancel);
    RecordingWorkStore workStore(ShapeType{k_WriteDimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f, log, shouldCancel, std::nullopt, std::nullopt, std::nullopt, std::nullopt, 3);
    RecordingOutputStore outputStore(ShapeType{k_WriteDimZ, k_DimY, k_DimX}, ShapeType{1}, -999.0f, log, shouldCancel);
    const ImageProcessing::detail::Maurer3DSlabParameters<uint8> parameters{.dims = writeDims,
                                                                            .background_value = uint8{0},
                                                                            .inside_is_positive = false,
                                                                            .squared_distance = true,
                                                                            .use_spacing = false,
                                                                            .spacing = FloatVec3Type{1.0f, 1.0f, 1.0f},
                                                                            .max_y_rows = 3,
                                                                            .worker_count = 2};

    const SeamResults results = RunSeam(writePattern, writeDims, parameters, log, inputStore, workStore, outputStore, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_INVALID(results.xy_result);
    REQUIRE_FALSE(results.xy_result.errors().empty());
    REQUIRE(results.xy_result.errors().front().code < 0);
    REQUIRE_FALSE(results.z_ran);
    const std::vector<TransferEvent> workWrites = GetTransferEvents(log, TransferEvent::Kind::WorkWrite);
    REQUIRE(workWrites.size() == 3);
    REQUIRE(workWrites.back().start == 60);
    REQUIRE(GetTransferEvents(log, TransferEvent::Kind::InputRead).size() <= 4);
    REQUIRE(GetTransferEvents(log, TransferEvent::Kind::OutputExtent).empty());
    REQUIRE(log.active.load() == 0);
  }

  SECTION("work read")
  {
    TransferLog log;
    std::atomic_bool shouldCancel{false};
    RecordingInputStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0}, log, shouldCancel);
    RecordingWorkStore workStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f, log, shouldCancel, std::nullopt, std::nullopt, std::nullopt, 9);
    RecordingOutputStore outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, -999.0f, log, shouldCancel);
    const ImageProcessing::detail::Maurer3DSlabParameters<uint8> parameters{
        .dims = dims, .background_value = uint8{0}, .inside_is_positive = false, .squared_distance = true, .use_spacing = false, .spacing = FloatVec3Type{1.0f, 1.0f, 1.0f}, .max_y_rows = 3, .worker_count = 2};

    const SeamResults results = RunSeam(pattern, dims, parameters, log, inputStore, workStore, outputStore, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_VALID(results.xy_result);
    REQUIRE(results.z_ran);
    SIMPLNX_RESULT_REQUIRE_INVALID(results.z_result);
    REQUIRE_FALSE(results.z_result.errors().empty());
    REQUIRE(results.z_result.errors().front().code < 0);
    REQUIRE(GetTransferEvents(log, TransferEvent::Kind::WorkRead).size() == 9);
    REQUIRE(GetTransferEvents(log, TransferEvent::Kind::OutputExtent).size() <= 1);
    REQUIRE(log.active.load() == 0);
  }

  SECTION("output extent")
  {
    TransferLog log;
    std::atomic_bool shouldCancel{false};
    RecordingInputStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0}, log, shouldCancel);
    RecordingWorkStore workStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0.0f, log, shouldCancel);
    RecordingOutputStore outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, -999.0f, log, shouldCancel, std::nullopt, 2);
    const ImageProcessing::detail::Maurer3DSlabParameters<uint8> parameters{
        .dims = dims, .background_value = uint8{0}, .inside_is_positive = false, .squared_distance = true, .use_spacing = false, .spacing = FloatVec3Type{1.0f, 1.0f, 1.0f}, .max_y_rows = 3, .worker_count = 2};

    const SeamResults results = RunSeam(pattern, dims, parameters, log, inputStore, workStore, outputStore, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_VALID(results.xy_result);
    REQUIRE(results.z_ran);
    SIMPLNX_RESULT_REQUIRE_INVALID(results.z_result);
    REQUIRE_FALSE(results.z_result.errors().empty());
    REQUIRE(results.z_result.errors().front().code < 0);
    REQUIRE(GetTransferEvents(log, TransferEvent::Kind::OutputExtent).size() == 2);
    REQUIRE(outputStore.completedExtentCount() == 1);
    const std::vector<TransferEvent> events = GetAllTransferEvents(log);
    REQUIRE(events.back().kind == TransferEvent::Kind::OutputExtent);
    REQUIRE(log.active.load() == 0);
  }
}

TEMPLATE_TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: seam matches the oracle at every batch size", "[NXImageProcessing][MaurerDistanceMapEngine]", int8, uint8, int16, uint16, int32, uint32,
                   int64, uint64)
{
  using T = TestType;
  SECTION("all integer types")
  {
    const SizeVec3Type dims{5, 6, 7};
    const std::vector<int32> pattern = MakePattern(dims[0], dims[1], dims[2]);
    const std::vector<float32> oracle = MaurerOracle(pattern, dims[0], dims[1], dims[2], 0, false, true);
    const std::vector<float32> inCore = RunInCore<T>(pattern, dims[0], dims[1], dims[2], 0, false, true);
    TransferLog log;
    std::atomic_bool shouldCancel{false};
    RecordingInputStore<T> inputStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, T{0}, log, shouldCancel);
    RecordingWorkStore workStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, 0.0f, log, shouldCancel);
    RecordingOutputStore outputStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, -999.0f, log, shouldCancel);
    const ImageProcessing::detail::Maurer3DSlabParameters<T> parameters{dims, T{0}, false, true, false, FloatVec3Type{1.0f, 1.0f, 1.0f}, 3, 2};

    const SeamResults results = RunSeam(pattern, dims, parameters, log, inputStore, workStore, outputStore, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_VALID(results.xy_result);
    REQUIRE(results.z_ran);
    SIMPLNX_RESULT_REQUIRE_VALID(results.z_result);
    const std::vector<float32> actual = ReadOutput(outputStore, pattern.size());
    RequireBitIdentical(actual, oracle);
    RequireBitIdentical(actual, inCore);
  }

  if constexpr(std::is_same_v<T, uint8> || std::is_same_v<T, int16>)
  {
    SECTION("unit spacing full matrix")
    {
      const std::vector<SizeVec3Type> dimensions = {{5, 6, 7}, {7, 9, 5}, {5, 6, 2}, {5, 6, 3}};
      for(const SizeVec3Type& dims : dimensions)
      {
        const std::vector<int32> pattern = MakePattern(dims[0], dims[1], dims[2]);
        const std::vector<float32> oracle = MaurerOracle(pattern, dims[0], dims[1], dims[2], 0, false, true);
        const std::vector<float32> inCore = RunInCore<T>(pattern, dims[0], dims[1], dims[2], 0, false, true);
        for(const usize maxYRows : {usize{1}, usize{2}, usize{3}, usize{6}})
        {
          DYNAMIC_SECTION(dims[0] << "x" << dims[1] << "x" << dims[2] << " with " << maxYRows << " Y rows")
          {
            TransferLog log;
            std::atomic_bool shouldCancel{false};
            RecordingInputStore<T> inputStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, T{0}, log, shouldCancel);
            RecordingWorkStore workStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, 0.0f, log, shouldCancel);
            RecordingOutputStore outputStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, -999.0f, log, shouldCancel);
            const ImageProcessing::detail::Maurer3DSlabParameters<T> parameters{dims, T{0}, false, true, false, FloatVec3Type{1.0f, 1.0f, 1.0f}, maxYRows, 2};

            const SeamResults results = RunSeam(pattern, dims, parameters, log, inputStore, workStore, outputStore, shouldCancel);
            SIMPLNX_RESULT_REQUIRE_VALID(results.xy_result);
            REQUIRE(results.z_ran);
            SIMPLNX_RESULT_REQUIRE_VALID(results.z_result);
            const std::vector<float32> actual = ReadOutput(outputStore, pattern.size());
            RequireBitIdentical(actual, oracle);
            RequireBitIdentical(actual, inCore);
          }
        }
      }
    }

    SECTION("image spacing")
    {
      const SizeVec3Type dims{5, 6, 7};
      const std::vector<int32> pattern = MakePattern(dims[0], dims[1], dims[2]);
      const FloatVec3Type spacing{0.5f, 2.0f, 1.25f};
      const std::vector<float32> inCore = RunInCore<T>(pattern, dims[0], dims[1], dims[2], 0, false, false, true, spacing);
      TransferLog log;
      std::atomic_bool shouldCancel{false};
      RecordingInputStore<T> inputStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, T{0}, log, shouldCancel);
      RecordingWorkStore workStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, 0.0f, log, shouldCancel);
      RecordingOutputStore outputStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, -999.0f, log, shouldCancel);
      const ImageProcessing::detail::Maurer3DSlabParameters<T> parameters{dims, T{0}, false, false, true, spacing, 2, 2};

      const SeamResults results = RunSeam(pattern, dims, parameters, log, inputStore, workStore, outputStore, shouldCancel);
      SIMPLNX_RESULT_REQUIRE_VALID(results.xy_result);
      REQUIRE(results.z_ran);
      SIMPLNX_RESULT_REQUIRE_VALID(results.z_result);
      RequireBitIdentical(ReadOutput(outputStore, pattern.size()), inCore);
    }
  }
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: encoded-sign line matches explicit inside mask", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr float32 k_Max = std::numeric_limits<float32>::max();
  const std::vector<std::vector<float32>> sources = {{k_Max, 0.0f, k_Max, k_Max, 0.0f, k_Max, 0.0f, k_Max, k_Max, 0.0f, k_Max},
                                                     {0.0f, k_Max, k_Max, k_Max, k_Max, k_Max, k_Max, k_Max, k_Max, k_Max, k_Max},
                                                     {k_Max, k_Max, k_Max, k_Max, k_Max, k_Max, k_Max, k_Max, k_Max, k_Max, 0.0f}};
  const std::vector<uint8> inside = {0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1};

  for(const std::vector<float32>& source : sources)
  {
    for(const bool insideIsPositive : {false, true})
    {
      for(const bool useSpacing : {false, true})
      {
        const float32 spacing = useSpacing ? 1.5f : 1.0f;
        CAPTURE(source, insideIsPositive, useSpacing);
        std::vector<float32> expected = source;
        std::vector<float32> encoded = source;
        for(usize index = 0; index < encoded.size(); ++index)
        {
          encoded[index] = ImageProcessing::detail::MaurerSignedValue(encoded[index], inside[index] != 0, insideIsPositive);
        }
        std::vector<float32> expectedG(source.size());
        std::vector<float32> expectedH(source.size());
        std::vector<float32> encodedG(source.size());
        std::vector<float32> encodedH(source.size());
        ImageProcessing::detail::Voronoi1D(expected, inside, expected.size(), insideIsPositive, useSpacing, spacing, expectedG, expectedH);
        ImageProcessing::detail::Voronoi1DEncodedSign(encoded, encoded.size(), useSpacing, spacing, encodedG, encodedH);
        RequireBitIdentical(encoded, expected);
      }
    }
  }
}

TEST_CASE("NXImageProcessing::MaurerDistanceMapEngine: external envelope matches resident kernel across blocks", "[NXImageProcessing][MaurerDistanceMapEngine]")
{
  constexpr float32 k_Max = std::numeric_limits<float32>::max();
  const std::vector<std::vector<float32>> sources = {{k_Max, 0.0f, k_Max, k_Max, 0.0f, k_Max, 0.0f, k_Max, k_Max, 0.0f, k_Max},
                                                     // The high candidate at index 4 is popped by index 6; the remaining zero-valued candidates span multiple
                                                     // three-value G/H blocks during evaluation.
                                                     {0.0f, k_Max, 0.0f, k_Max, 100.0f, k_Max, 0.0f, k_Max, 0.0f, k_Max, 0.0f},
                                                     std::vector<float32>(11, k_Max)};
  const std::vector<uint8> inside = {0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1};

  for(const std::vector<float32>& source : sources)
  {
    for(const bool insideIsPositive : {false, true})
    {
      for(const bool useSpacing : {false, true})
      {
        const float32 spacing = useSpacing ? 1.5f : 1.0f;
        CAPTURE(source, insideIsPositive, useSpacing, spacing);
        std::vector<float32> expected = source;
        std::vector<float32> residentG(source.size());
        std::vector<float32> residentH(source.size());
        ImageProcessing::detail::Voronoi1D(expected, inside, expected.size(), insideIsPositive, useSpacing, spacing, residentG, residentH);

        DataStore<float32> sourceStore(ShapeType{source.size()}, ShapeType{1}, 0.0f);
        DataStore<uint8> insideStore(ShapeType{inside.size()}, ShapeType{1}, 0);
        DataStore<float32> gStore(ShapeType{source.size()}, ShapeType{1}, 0.0f);
        DataStore<float32> hStore(ShapeType{source.size()}, ShapeType{1}, 0.0f);
        DataStore<float32> outputStore(ShapeType{source.size()}, ShapeType{1}, 0.0f);
        REQUIRE(sourceStore.copyFromBuffer(0, source).valid());
        REQUIRE(insideStore.copyFromBuffer(0, inside).valid());
        std::atomic_bool shouldCancel{false};
        const Result<> result =
            ImageProcessing::detail::ExternalVoronoi1D(sourceStore, insideStore, 0, source.size(), insideIsPositive, useSpacing, spacing, 3, gStore, hStore, outputStore, shouldCancel);
        REQUIRE(result.valid());
        std::vector<float32> actual(source.size());
        REQUIRE(outputStore.copyIntoBuffer(0, actual).valid());
        REQUIRE(actual == expected);
      }
    }
  }

  DataStore<float32> cancelledSource(ShapeType{sources.front().size()}, ShapeType{1}, 0.0f);
  DataStore<uint8> cancelledInside(ShapeType{inside.size()}, ShapeType{1}, 0);
  DataStore<float32> cancelledG(ShapeType{sources.front().size()}, ShapeType{1}, 0.0f);
  DataStore<float32> cancelledH(ShapeType{sources.front().size()}, ShapeType{1}, 0.0f);
  DataStore<float32> cancelledOutput(ShapeType{sources.front().size()}, ShapeType{1}, -123.0f);
  REQUIRE(cancelledSource.copyFromBuffer(0, sources.front()).valid());
  REQUIRE(cancelledInside.copyFromBuffer(0, inside).valid());
  std::atomic_bool shouldCancel{true};
  REQUIRE(
      ImageProcessing::detail::ExternalVoronoi1D(cancelledSource, cancelledInside, 0, sources.front().size(), false, false, 1.0f, 3, cancelledG, cancelledH, cancelledOutput, shouldCancel).valid());
  std::vector<float32> cancelledValues(sources.front().size());
  REQUIRE(cancelledOutput.copyIntoBuffer(0, cancelledValues).valid());
  REQUIRE(std::all_of(cancelledValues.cbegin(), cancelledValues.cend(), [](float32 value) { return value == -123.0f; }));

  DataStore<float32> midTransferSource(ShapeType{sources[1].size()}, ShapeType{1}, 0.0f);
  DataStore<uint8> midTransferInside(ShapeType{inside.size()}, ShapeType{1}, 0);
  DataStore<float32> midTransferG(ShapeType{sources[1].size()}, ShapeType{1}, 0.0f);
  DataStore<float32> midTransferH(ShapeType{sources[1].size()}, ShapeType{1}, 0.0f);
  DataStore<float32> midTransferOutput(ShapeType{sources[1].size()}, ShapeType{1}, -456.0f);
  REQUIRE(midTransferSource.copyFromBuffer(0, sources[1]).valid());
  REQUIRE(midTransferInside.copyFromBuffer(0, inside).valid());
  std::atomic_bool cancelDuringOutput{false};
  usize outputBlockCount = 0;
  auto cancellingSink = [&](usize blockBegin, nonstd::span<float32> values, nonstd::span<const uint8>) {
    ++outputBlockCount;
    Result<> result = midTransferOutput.copyFromBuffer(blockBegin, nonstd::span<const float32>(values.data(), values.size()));
    cancelDuringOutput.store(true);
    return result;
  };
  REQUIRE(ImageProcessing::detail::ExternalVoronoi1DToSink(midTransferSource, midTransferInside, 0, sources[1].size(), false, false, 1.0f, 3, midTransferG, midTransferH, cancellingSink,
                                                           cancelDuringOutput)
              .valid());
  REQUIRE(outputBlockCount == 1);
  std::vector<float32> partiallyWritten(sources[1].size());
  REQUIRE(midTransferOutput.copyIntoBuffer(0, partiallyWritten).valid());
  REQUIRE(std::none_of(partiallyWritten.cbegin(), partiallyWritten.cbegin() + 3, [](float32 value) { return value == -456.0f; }));
  REQUIRE(std::all_of(partiallyWritten.cbegin() + 3, partiallyWritten.cend(), [](float32 value) { return value == -456.0f; }));
}
