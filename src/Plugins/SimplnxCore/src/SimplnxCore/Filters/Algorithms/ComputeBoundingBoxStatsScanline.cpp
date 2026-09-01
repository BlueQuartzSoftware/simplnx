#include "ComputeBoundingBoxStatsScanline.hpp"

#include "ComputeBoundingBoxStats.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <system_error>

using namespace nx::core;

namespace
{
constexpr usize k_MinXIndex = 0;
constexpr usize k_MinYIndex = 1;
constexpr usize k_MinZIndex = 2;
constexpr usize k_MaxXIndex = 3;
constexpr usize k_MaxYIndex = 4;
constexpr usize k_MaxZIndex = 5;
constexpr usize k_ChunkSize = 65536;

/* clang-format off */
template <class T>
concept ArithmeticNotBool = std::is_arithmetic_v<T> && !std::is_same_v<T, bool>;
/* clang-format on */

/**
 * @brief Converts one stored physical bounding box to half-open voxel indices.
 * @param unifiedBounds Contains six values for each bounding box.
 * @param targetBoundsIndex Selects the bounding box.
 * @param image Supplies the origin, spacing, and dimensions.
 * @return Clipped minimum and maximum voxel indices in X-Y-Z order.
 * @pre Preflight has verified nonzero image spacing.
 */
std::array<usize, 6> GetVoxelIndices(const Float32AbstractDataStore& unifiedBounds, usize targetBoundsIndex, const ImageGeom& image)
{
  std::array<usize, 6> voxelIndices = {};

  // Preflight verifies nonzero spacing, so this function does not repeat that check.
  FloatVec3 spacing = image.getSpacing();
  FloatVec3 origin = image.getOrigin();
  SizeVec3 dims = image.getDimensions();

  // The lower bound from input is expected to be the lower corner of the voxel
  for(usize i = 0; i < 3; i++)
  {
    float32 minVoxel = unifiedBounds.getValue((targetBoundsIndex * 6) + i);
    float32 maxDim = (spacing[i] * static_cast<float32>(dims[i])) + origin[i];
    if(minVoxel < origin[i])
    {
      voxelIndices[i] = 0;
    }
    else if(minVoxel > maxDim)
    {
      voxelIndices[i] = std::floor((maxDim - origin[i]) / spacing[i]);
    }
    else
    {
      voxelIndices[i] = std::floor((minVoxel - origin[i]) / spacing[i]);
    }
  }

  // The upper bound from input is expected to be the upper corner of the voxel
  for(usize i = 0; i < 3; i++)
  {
    usize offset = i + 3;
    float32 maxVoxel = unifiedBounds.getValue((targetBoundsIndex * 6) + offset);
    float32 maxDim = (spacing[i] * static_cast<float32>(dims[i])) + origin[i];
    if(maxVoxel < origin[i])
    {
      voxelIndices[offset] = 0;
    }
    else if(maxVoxel > maxDim)
    {
      voxelIndices[offset] = std::floor((maxDim - origin[i]) / spacing[i]);
    }
    else
    {
      voxelIndices[offset] = std::floor((maxVoxel - origin[i]) / spacing[i]);
    }
  }

  return voxelIndices;
}

/**
 * @brief Converts one buffered physical bounding box to half-open voxel indices.
 * @param unifiedBounds Contains six values for each bounding box.
 * @param targetBoundsIndex Selects the bounding box.
 * @param image Supplies the origin, spacing, and dimensions.
 * @return Clipped minimum and maximum voxel indices in X-Y-Z order.
 * @pre Preflight has verified nonzero image spacing.
 */
std::array<usize, 6> GetVoxelIndices(nonstd::span<const float32> unifiedBounds, usize targetBoundsIndex, const ImageGeom& image)
{
  std::array<usize, 6> voxelIndices = {};
  const FloatVec3 spacing = image.getSpacing();
  const FloatVec3 origin = image.getOrigin();
  const SizeVec3 dims = image.getDimensions();

  for(usize i = 0; i < 3; i++)
  {
    const float32 minVoxel = unifiedBounds[(targetBoundsIndex * 6) + i];
    const float32 maxDim = (spacing[i] * static_cast<float32>(dims[i])) + origin[i];
    if(minVoxel < origin[i])
    {
      voxelIndices[i] = 0;
    }
    else if(minVoxel > maxDim)
    {
      voxelIndices[i] = std::floor((maxDim - origin[i]) / spacing[i]);
    }
    else
    {
      voxelIndices[i] = std::floor((minVoxel - origin[i]) / spacing[i]);
    }
  }

  for(usize i = 0; i < 3; i++)
  {
    const usize offset = i + 3;
    const float32 maxVoxel = unifiedBounds[(targetBoundsIndex * 6) + offset];
    const float32 maxDim = (spacing[i] * static_cast<float32>(dims[i])) + origin[i];
    if(maxVoxel < origin[i])
    {
      voxelIndices[offset] = 0;
    }
    else if(maxVoxel > maxDim)
    {
      voxelIndices[offset] = std::floor((maxDim - origin[i]) / spacing[i]);
    }
    else
    {
      voxelIndices[offset] = std::floor((maxVoxel - origin[i]) / spacing[i]);
    }
  }

  return voxelIndices;
}

/**
 * @struct StatsCache
 * @brief Stores statistics that one input pass can collect.
 * @tparam T Input value type.
 *
 * Mode storage exists only when mode is requested. Standard deviation uses a
 * second pass, so this cache does not reserve storage for either result.
 */
template <typename T>
struct StatsCache
{
  using value_type = T;
  T minValue = std::numeric_limits<T>::quiet_NaN();
  T maxValue = std::numeric_limits<T>::quiet_NaN();
  usize count = 0;
  T summationValue = static_cast<T>(0);
};

/**
 * @struct CompleteStatsCache
 * @brief Extends the base cache with frequency-derived statistics.
 * @tparam T Input value type.
 */
template <typename T>
struct CompleteStatsCache : StatsCache<T>
{
  float32 medianValue = std::numeric_limits<float32>::quiet_NaN();
  usize uniqueValCount = 0;
};

/**
 * @class TempDirectory
 * @brief Owns the UUID-named temporary directory used by one execution.
 *
 * Destruction makes a best-effort recursive removal. Cleanup errors are not
 * reported because the main result has already been determined.
 * @warning Destruction removes the exact supplied path even when directory
 *          creation fails. The caller must supply a disposable path.
 */
class TempDirectory
{
public:
  explicit TempDirectory(std::filesystem::path path)
  : m_Path(std::move(path))
  {
  }

  ~TempDirectory()
  {
    std::error_code errorCode;
    std::filesystem::remove_all(m_Path, errorCode);
  }

  TempDirectory(const TempDirectory&) = delete;
  TempDirectory(TempDirectory&&) noexcept = delete;
  TempDirectory& operator=(const TempDirectory&) = delete;
  TempDirectory& operator=(TempDirectory&&) noexcept = delete;

  const std::filesystem::path& path() const
  {
    return m_Path;
  }

private:
  std::filesystem::path m_Path;
};

/**
 * @class BinaryRunReader
 * @brief Reads a bounded slice of one temporary sorted-run file.
 * @tparam T Stored value type.
 *
 * The reader uses a fixed 65,536-value buffer. Callers must inspect failed()
 * after hasValue() becomes false to distinguish end of range from I/O failure.
 */
template <typename T>
class BinaryRunReader
{
public:
  BinaryRunReader(const std::filesystem::path& path, usize start, usize count)
  : m_Stream(path, std::ios::binary)
  , m_Remaining(count)
  , m_Buffer(std::make_unique<T[]>(k_ChunkSize))
  {
    if(!m_Stream.is_open())
    {
      m_Failed = true;
      return;
    }

    m_Stream.seekg(static_cast<std::streamoff>(start) * static_cast<std::streamoff>(sizeof(T)));
    m_Failed = m_Stream.fail();
  }

  bool hasValue()
  {
    if(m_BufferIndex == m_BufferCount && m_Remaining > 0)
    {
      refill();
    }
    return m_BufferIndex < m_BufferCount;
  }

  const T& value() const
  {
    return m_Buffer[m_BufferIndex];
  }

  void advance()
  {
    m_BufferIndex++;
  }

  bool failed() const
  {
    return m_Failed;
  }

private:
  void refill()
  {
    m_BufferIndex = 0;
    m_BufferCount = std::min(k_ChunkSize, m_Remaining);
    const auto byteCount = static_cast<std::streamsize>(m_BufferCount * sizeof(T));
    m_Stream.read(reinterpret_cast<char*>(m_Buffer.get()), byteCount);
    if(m_Stream.gcount() != byteCount)
    {
      m_BufferCount = 0;
      m_Failed = true;
      return;
    }
    m_Remaining -= m_BufferCount;
  }

  std::ifstream m_Stream;
  usize m_Remaining = 0;
  std::unique_ptr<T[]> m_Buffer;
  usize m_BufferIndex = 0;
  usize m_BufferCount = 0;
  bool m_Failed = false;
};

template <typename T>
bool WriteBuffer(std::ofstream& stream, const T* buffer, usize count)
{
  stream.write(reinterpret_cast<const char*>(buffer), static_cast<std::streamsize>(count * sizeof(T)));
  return stream.good();
}

/**
 * @brief Compares values with the ordering used by the frequency algorithms.
 * @tparam T Input value type.
 * @param lhs First value.
 * @param rhs Second value.
 * @return True when neither value orders before the other.
 * @note Unordered floating-point pairs compare as equivalent. This preserves
 *       the direct implementation's behavior.
 */
template <typename T>
bool Equivalent(const T& lhs, const T& rhs)
{
  return !(lhs < rhs) && !(rhs < lhs);
}

/**
 * @brief Streams one clipped box in the original Z-Y-X order using bounded contiguous reads.
 * @tparam T Input value type.
 * @tparam FunctionT Callback type.
 * @param imageGeom Supplies the image dimensions.
 * @param inputStore Supplies the input values.
 * @param voxelIndices Contains half-open voxel bounds.
 * @param buffer Supplies fixed-size read scratch.
 * @param shouldCancel Signals cancellation between rows.
 * @param function Receives each value in Z-Y-X order.
 * @return Success, or a bulk-read error.
 *
 * Fixed-size row reads keep RAM independent of box volume. Cancellation can
 * leave callback side effects from earlier rows, and it returns success.
 */
template <typename T, class FunctionT>
Result<> ForEachBoxValue(const ImageGeom& imageGeom, const AbstractDataStore<T>& inputStore, const std::array<usize, 6>& voxelIndices, T* buffer, const std::atomic_bool& shouldCancel,
                         FunctionT&& function)
{
  if(voxelIndices[k_MinXIndex] >= voxelIndices[k_MaxXIndex] || voxelIndices[k_MinYIndex] >= voxelIndices[k_MaxYIndex] || voxelIndices[k_MinZIndex] >= voxelIndices[k_MaxZIndex])
  {
    return {};
  }

  const usize xPoints = imageGeom.getNumXCells();
  const usize yPoints = imageGeom.getNumYCells();
  for(usize zIndex = voxelIndices[k_MinZIndex]; zIndex < voxelIndices[k_MaxZIndex]; zIndex++)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize zStride = zIndex * xPoints * yPoints;
    for(usize yIndex = voxelIndices[k_MinYIndex]; yIndex < voxelIndices[k_MaxYIndex]; yIndex++)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize rowStart = zStride + (yIndex * xPoints) + voxelIndices[k_MinXIndex];
      const usize rowLength = voxelIndices[k_MaxXIndex] - voxelIndices[k_MinXIndex];
      for(usize rowOffset = 0; rowOffset < rowLength; rowOffset += k_ChunkSize)
      {
        const usize count = std::min(k_ChunkSize, rowLength - rowOffset);
        Result<> copyResult = inputStore.copyIntoBuffer(rowStart + rowOffset, nonstd::span<T>(buffer, count));
        if(copyResult.invalid())
        {
          return copyResult;
        }
        for(usize index = 0; index < count; index++)
        {
          function(buffer[index]);
        }
      }
    }
  }
  return {};
}

/**
 * @brief Applies a callback to each frequency group in a sorted file.
 * @tparam T Stored value type.
 * @tparam FunctionT Callback type.
 * @param path Identifies the sorted temporary file.
 * @param count Gives the number of stored values to scan.
 * @param shouldCancel Signals cancellation between fixed-size batches.
 * @param function Receives each value and its frequency.
 * @return Success, or a temporary-file read error.
 *
 * Cancellation returns success and can leave callback side effects from
 * earlier groups.
 */
template <typename T, class FunctionT>
Result<> ScanSortedGroups(const std::filesystem::path& path, usize count, const std::atomic_bool& shouldCancel, FunctionT&& function)
{
  BinaryRunReader<T> reader(path, 0, count);
  std::optional<T> currentValue;
  uint64 currentCount = 0;
  usize valuesUntilCancelCheck = 0;
  while(reader.hasValue())
  {
    if(valuesUntilCancelCheck == 0 && shouldCancel)
    {
      return {};
    }
    valuesUntilCancelCheck = (valuesUntilCancelCheck + 1) % k_ChunkSize;

    const T value = reader.value();
    reader.advance();
    if(currentValue.has_value() && Equivalent(currentValue.value(), value))
    {
      currentCount++;
    }
    else
    {
      if(currentValue.has_value())
      {
        function(currentValue.value(), currentCount);
      }
      currentValue = value;
      currentCount = 1;
    }
  }
  if(reader.failed())
  {
    return MakeErrorResult(-69320, fmt::format("ComputeBoundingBoxStats: Failed while reading temporary sorted values from '{}'.", path.string()));
  }
  if(currentValue.has_value())
  {
    function(currentValue.value(), currentCount);
  }
  return {};
}

/**
 * @brief Merges fixed-size sorted runs until one sorted run remains.
 * @tparam T Stored value type.
 * @param sourcePath Identifies the initial runs file.
 * @param destinationPath Identifies alternating merge output.
 * @param valueCount Gives the number of values.
 * @param shouldCancel Signals cancellation between bounded writes.
 * @return The path that contains the current complete run, or an I/O error.
 *
 * Cancellation can leave an incomplete destination file. The caller observes
 * the cancellation flag and does not consume that file.
 */
template <typename T>
Result<std::filesystem::path> MergeSortedRuns(const std::filesystem::path& sourcePath, const std::filesystem::path& destinationPath, usize valueCount, const std::atomic_bool& shouldCancel)
{
  std::filesystem::path currentSource = sourcePath;
  std::filesystem::path currentDestination = destinationPath;
  auto outputBuffer = std::make_unique<T[]>(k_ChunkSize);
  usize runWidth = k_ChunkSize;
  while(runWidth < valueCount)
  {
    if(shouldCancel)
    {
      return {currentSource};
    }

    std::ofstream destinationStream(currentDestination, std::ios::binary | std::ios::trunc);
    if(!destinationStream.is_open())
    {
      return MakeErrorResult<std::filesystem::path>(-69321, fmt::format("ComputeBoundingBoxStats: Failed to create temporary merge file '{}'.", currentDestination.string()));
    }

    usize outputCount = 0;
    for(usize leftStart = 0; leftStart < valueCount;)
    {
      const usize leftCount = std::min(runWidth, valueCount - leftStart);
      const usize rightStart = leftStart + leftCount;
      const usize rightCount = std::min(runWidth, valueCount - rightStart);
      BinaryRunReader<T> leftReader(currentSource, leftStart, leftCount);
      BinaryRunReader<T> rightReader(currentSource, rightStart, rightCount);
      bool leftAvailable = leftReader.hasValue();
      bool rightAvailable = rightReader.hasValue();
      while(leftAvailable || rightAvailable)
      {
        if(leftReader.failed() || rightReader.failed())
        {
          return MakeErrorResult<std::filesystem::path>(-69322, fmt::format("ComputeBoundingBoxStats: Failed while reading temporary runs from '{}'.", currentSource.string()));
        }
        if(!rightAvailable || (leftAvailable && !(rightReader.value() < leftReader.value())))
        {
          outputBuffer[outputCount++] = leftReader.value();
          leftReader.advance();
          leftAvailable = leftReader.hasValue();
        }
        else
        {
          outputBuffer[outputCount++] = rightReader.value();
          rightReader.advance();
          rightAvailable = rightReader.hasValue();
        }
        if(outputCount == k_ChunkSize)
        {
          if(!WriteBuffer(destinationStream, outputBuffer.get(), outputCount))
          {
            return MakeErrorResult<std::filesystem::path>(-69323, fmt::format("ComputeBoundingBoxStats: Failed while writing temporary merge file '{}'.", currentDestination.string()));
          }
          outputCount = 0;
          if(shouldCancel)
          {
            return {currentSource};
          }
        }
      }
      leftStart = rightStart + rightCount;
      if(shouldCancel)
      {
        return {currentSource};
      }
    }

    if(outputCount > 0 && !WriteBuffer(destinationStream, outputBuffer.get(), outputCount))
    {
      return MakeErrorResult<std::filesystem::path>(-69323, fmt::format("ComputeBoundingBoxStats: Failed while writing temporary merge file '{}'.", currentDestination.string()));
    }
    destinationStream.close();
    if(destinationStream.fail())
    {
      return MakeErrorResult<std::filesystem::path>(-69323, fmt::format("ComputeBoundingBoxStats: Failed while closing temporary merge file '{}'.", currentDestination.string()));
    }
    std::swap(currentSource, currentDestination);
    runWidth = runWidth > valueCount / 2 ? valueCount : runWidth * 2;
  }
  return {currentSource};
}

/**
 * @brief Streams base statistics for one bounding box.
 * @tparam T Input value type.
 * @param imageGeom Supplies the image dimensions.
 * @param inputStore Supplies the input values.
 * @param voxelIndices Contains half-open voxel bounds.
 * @param inputBuffer Supplies fixed-size read scratch.
 * @param shouldCancel Signals cancellation.
 * @param stats Receives the complete result after the scan.
 * @return Success, or a bulk-read error.
 *
 * Cancellation returns success and does not publish a partial cache entry.
 */
template <typename T>
Result<> StreamBaseStats(const ImageGeom& imageGeom, const AbstractDataStore<T>& inputStore, const std::array<usize, 6>& voxelIndices, T* inputBuffer, const std::atomic_bool& shouldCancel,
                         StatsCache<T>& stats)
{
  T minValue = std::numeric_limits<T>::max();
  T maxValue = std::numeric_limits<T>::lowest();
  T summationValue = static_cast<T>(0);
  usize count = 0;
  Result<> result = ForEachBoxValue(imageGeom, inputStore, voxelIndices, inputBuffer, shouldCancel, [&](T value) {
    count++;
    minValue = std::min(minValue, value);
    maxValue = std::max(maxValue, value);
    summationValue += value;
  });
  if(result.invalid() || shouldCancel)
  {
    return result;
  }

  if(count == 0)
  {
    minValue = std::numeric_limits<T>::quiet_NaN();
    maxValue = std::numeric_limits<T>::quiet_NaN();
  }
  stats.count = count;
  stats.minValue = minValue;
  stats.maxValue = maxValue;
  stats.summationValue = summationValue;
  return {};
}

/**
 * @brief Streams base statistics and writes sorted runs for one bounding box.
 * @tparam T Input value type.
 * @param imageGeom Supplies the image dimensions.
 * @param inputStore Supplies the input values.
 * @param voxelIndices Contains half-open voxel bounds.
 * @param inputBuffer Supplies fixed-size read scratch.
 * @param runBuffer Supplies fixed-size sorting scratch.
 * @param sourcePath Identifies the initial runs file.
 * @param destinationPath Identifies alternating merge output.
 * @param shouldCancel Signals cancellation.
 * @param stats Receives base statistics after the complete input scan.
 * @return The sorted-file path, or a storage or temporary-file error.
 *
 * Cancellation returns the source path as a successful value. The caller
 * observes the cancellation flag before it consumes that value.
 */
template <typename T>
Result<std::filesystem::path> StreamStatsAndSortedRuns(const ImageGeom& imageGeom, const AbstractDataStore<T>& inputStore, const std::array<usize, 6>& voxelIndices, T* inputBuffer, T* runBuffer,
                                                       const std::filesystem::path& sourcePath, const std::filesystem::path& destinationPath, const std::atomic_bool& shouldCancel,
                                                       CompleteStatsCache<T>& stats)
{
  std::ofstream runStream(sourcePath, std::ios::binary | std::ios::trunc);
  if(!runStream.is_open())
  {
    return MakeErrorResult<std::filesystem::path>(-69324, fmt::format("ComputeBoundingBoxStats: Failed to create temporary sort file '{}'.", sourcePath.string()));
  }

  T minValue = std::numeric_limits<T>::max();
  T maxValue = std::numeric_limits<T>::lowest();
  T summationValue = static_cast<T>(0);
  usize count = 0;
  usize runCount = 0;
  bool writeFailed = false;
  Result<> result = ForEachBoxValue(imageGeom, inputStore, voxelIndices, inputBuffer, shouldCancel, [&](T value) {
    count++;
    minValue = std::min(minValue, value);
    maxValue = std::max(maxValue, value);
    summationValue += value;
    runBuffer[runCount++] = value;
    if(runCount == k_ChunkSize)
    {
      std::stable_sort(runBuffer, runBuffer + runCount);
      writeFailed = writeFailed || !WriteBuffer(runStream, runBuffer, runCount);
      runCount = 0;
    }
  });
  if(result.invalid())
  {
    return ConvertResultTo<std::filesystem::path>(ConvertResult(std::move(result)), {});
  }
  if(shouldCancel)
  {
    return {sourcePath};
  }
  if(!writeFailed && runCount > 0)
  {
    std::stable_sort(runBuffer, runBuffer + runCount);
    writeFailed = !WriteBuffer(runStream, runBuffer, runCount);
  }
  runStream.close();
  writeFailed = writeFailed || runStream.fail();
  if(writeFailed)
  {
    return MakeErrorResult<std::filesystem::path>(-69325, fmt::format("ComputeBoundingBoxStats: Failed while writing temporary sort file '{}'.", sourcePath.string()));
  }

  if(count == 0)
  {
    minValue = std::numeric_limits<T>::quiet_NaN();
    maxValue = std::numeric_limits<T>::quiet_NaN();
  }
  stats.count = count;
  stats.minValue = minValue;
  stats.maxValue = maxValue;
  stats.summationValue = summationValue;
  if(count == 0)
  {
    return {sourcePath};
  }
  return MergeSortedRuns<T>(sourcePath, destinationPath, count, shouldCancel);
}

/**
 * @brief Derives median, unique count, and optional modes from sorted values.
 * @tparam T Stored value type.
 * @param sortedPath Identifies the sorted temporary file.
 * @param stats Supplies the value count and receives frequency statistics.
 * @param modesList Receives tied modes, or is null when mode is not requested.
 * @param targetBoundsIndex Selects the mode output list.
 * @param shouldCancel Signals cancellation.
 * @return Success, or a temporary-file read error.
 *
 * This path narrows the maximum mode frequency to int before it compares
 * frequencies. A count above INT_MAX can produce an incorrect or empty list.
 */
template <typename T>
Result<> CalculateFrequencyStats(const std::filesystem::path& sortedPath, CompleteStatsCache<T>& stats, NeighborList<T>* modesList, usize targetBoundsIndex, const std::atomic_bool& shouldCancel)
{
  const usize medianPosition = (stats.count / 2) + 1;
  usize cumulativeFrequency = 0;
  uint64 maxCount = 0;
  std::optional<T> previousValue;
  bool medianFound = false;
  Result<> result = ScanSortedGroups<T>(sortedPath, stats.count, shouldCancel, [&](T value, uint64 frequency) {
    stats.uniqueValCount++;
    maxCount = std::max(maxCount, frequency);
    cumulativeFrequency += frequency;
    if(!medianFound && cumulativeFrequency >= medianPosition)
    {
      if(stats.count % 2 == 0 && cumulativeFrequency == medianPosition && previousValue.has_value())
      {
        stats.medianValue = (static_cast<float32>(previousValue.value()) + static_cast<float32>(value)) / 2.0f;
      }
      else
      {
        stats.medianValue = static_cast<float32>(value);
      }
      medianFound = true;
    }
    previousValue = value;
  });
  if(result.invalid() || shouldCancel || modesList == nullptr)
  {
    return result;
  }

  // Preserve the direct implementation's narrowing conversion before mode comparisons.
  const int modalCount = maxCount;
  return ScanSortedGroups<T>(sortedPath, stats.count, shouldCancel, [&](T value, uint64 frequency) {
    if(frequency == modalCount)
    {
      modesList->addEntry(targetBoundsIndex, value);
    }
  });
}

/**
 * @brief Streams population standard deviation for all nonempty boxes.
 * @tparam T Input value type.
 * @tparam CacheT Base-statistics cache type.
 * @param imageGeom Supplies the image dimensions.
 * @param inputStore Supplies the input values.
 * @param unifiedBounds Contains six values for each bounding box.
 * @param inputBuffer Supplies fixed-size read scratch.
 * @param statsVector Supplies counts and sums.
 * @param stdDevStore Receives completed standard deviations immediately.
 * @param shouldCancel Signals cancellation.
 * @return Success, or a bulk-read error.
 *
 * Cancellation returns success. Values written for earlier boxes remain in
 * the output store.
 */
template <typename T, class CacheT>
Result<> StreamStdDeviation(const ImageGeom& imageGeom, const AbstractDataStore<T>& inputStore, nonstd::span<const float32> unifiedBounds, T* inputBuffer, const std::vector<CacheT>& statsVector,
                            Float32AbstractDataStore& stdDevStore, const std::atomic_bool& shouldCancel)
{
  for(usize targetBoundsIndex = 0; targetBoundsIndex < statsVector.size(); targetBoundsIndex++)
  {
    if(shouldCancel)
    {
      return {};
    }
    if(statsVector[targetBoundsIndex].count == 0)
    {
      continue;
    }

    const std::array<usize, 6> voxelIndices = GetVoxelIndices(unifiedBounds, targetBoundsIndex, imageGeom);
    const float32 meanValue = statsVector[targetBoundsIndex].summationValue / static_cast<float32>(statsVector[targetBoundsIndex].count);
    float64 sumOfDiffs = 0.0f;
    Result<> result = ForEachBoxValue(imageGeom, inputStore, voxelIndices, inputBuffer, shouldCancel, [&](T value) { sumOfDiffs += static_cast<float64>((value - meanValue) * (value - meanValue)); });
    if(result.invalid() || shouldCancel)
    {
      return result;
    }
    stdDevStore.setValue(targetBoundsIndex, static_cast<float32>(std::sqrt(sumOfDiffs / static_cast<float64>(statsVector[targetBoundsIndex].count))));
  }
  return {};
}

template <class Cache>
concept CacheType = std::is_base_of_v<StatsCache<typename Cache::value_type>, Cache>;

/**
 * @brief Writes completed cache values to the selected framework outputs.
 * @tparam T Input value type.
 * @tparam StatsCacheT Statistics cache type.
 * @param statsVector Supplies completed values for all boxes.
 * @param dataStructure Contains output arrays.
 * @param inputValues Selects outputs and identifies their paths.
 * @return Success, or an output-store error.
 *
 * Serial writes avoid relying on generic DataStore thread safety.
 */
template <typename T, CacheType StatsCacheT>
Result<> FillStatsArrays(const std::vector<StatsCacheT>& statsVector, DataStructure& dataStructure, const ComputeBoundingBoxStatsInputValues* inputValues)
{
  AbstractDataStore<bool>* boundsHasDataArray = dataStructure.getDataRefAs<BoolArray>(inputValues->BoundsHasDataPath).getDataStore();
  if(boundsHasDataArray == nullptr)
  {
    return MakeErrorResult(-69309, fmt::format("Bounds Has Data array from path {} invalid", inputValues->BoundsHasDataPath.toString()));
  }

  AbstractDataStore<uint64>* lengthArray = nullptr;
  AbstractDataStore<T>* minArray = nullptr;
  AbstractDataStore<T>* maxArray = nullptr;
  AbstractDataStore<float32>* summationArray = nullptr;
  AbstractDataStore<float32>* meanArray = nullptr;
  AbstractDataStore<float32>* medianArray = nullptr;
  AbstractDataStore<int32>* numUniqueValuesArray = nullptr;

  if(inputValues->CalculateLength)
  {
    lengthArray = dataStructure.getDataRefAs<UInt64Array>(inputValues->LengthPath).getDataStore();
    if(lengthArray == nullptr)
    {
      return MakeErrorResult(-69310, fmt::format("Count array from path {} invalid", inputValues->LengthPath.toString()));
    }
  }
  if(inputValues->CalculateMin)
  {
    minArray = dataStructure.getDataRefAs<DataArray<T>>(inputValues->MinPath).getDataStore();
    if(minArray == nullptr)
    {
      return MakeErrorResult(-69311, fmt::format("Min array from path {} invalid", inputValues->MinPath.toString()));
    }
  }
  if(inputValues->CalculateMax)
  {
    maxArray = dataStructure.getDataRefAs<DataArray<T>>(inputValues->MaxPath).getDataStore();
    if(maxArray == nullptr)
    {
      return MakeErrorResult(-69312, fmt::format("Max array from path {} invalid", inputValues->MaxPath.toString()));
    }
  }
  if(inputValues->CalculateSummation)
  {
    summationArray = dataStructure.getDataRefAs<DataArray<float32>>(inputValues->SummationPath).getDataStore();
    if(summationArray == nullptr)
    {
      return MakeErrorResult(-69313, fmt::format("Summation array from path {} invalid", inputValues->SummationPath.toString()));
    }
  }
  if(inputValues->CalculateMean)
  {
    meanArray = dataStructure.getDataRefAs<Float32Array>(inputValues->MeanPath).getDataStore();
    if(meanArray == nullptr)
    {
      return MakeErrorResult(-69314, fmt::format("Mean array from path {} invalid", inputValues->MeanPath.toString()));
    }
  }
  if constexpr(std::is_same_v<StatsCacheT, CompleteStatsCache<typename StatsCacheT::value_type>>)
  {
    if(inputValues->CalculateMedian)
    {
      medianArray = dataStructure.getDataRefAs<Float32Array>(inputValues->MedianPath).getDataStore();
      if(meanArray == nullptr)
      {
        return MakeErrorResult(-69315, fmt::format("Median array from path {} invalid", inputValues->MedianPath.toString()));
      }
    }
    if(inputValues->CalculateNumUniqueValues)
    {
      numUniqueValuesArray = dataStructure.getDataRefAs<Int32Array>(inputValues->NumUniqueValuesPath).getDataStore();
      if(numUniqueValuesArray == nullptr)
      {
        return MakeErrorResult(-69316, fmt::format("Number of Unique Value array from path {} invalid", inputValues->MedianPath.toString()));
      }
    }
  }

  for(usize i = 0; i < statsVector.size(); i++)
  {
    if(statsVector[i].count > 0) // This guards against dividing by zero
    {
      boundsHasDataArray->setValue(i, true);
      if(lengthArray != nullptr)
      {
        lengthArray->setValue(i, statsVector[i].count);
      }
      if(minArray != nullptr)
      {
        minArray->setValue(i, statsVector[i].minValue);
      }
      if(maxArray != nullptr)
      {
        maxArray->setValue(i, statsVector[i].maxValue);
      }
      if(summationArray != nullptr)
      {
        summationArray->setValue(i, statsVector[i].summationValue);
      }
      if(meanArray != nullptr)
      {
        float32 meanValue = 0.0f;
        meanValue = statsVector[i].summationValue / static_cast<float32>(statsVector[i].count);
        meanArray->setValue(i, meanValue);
      }
      if constexpr(std::is_same_v<StatsCacheT, CompleteStatsCache<typename StatsCacheT::value_type>>)
      {
        if(medianArray != nullptr)
        {
          medianArray->setValue(i, statsVector[i].medianValue);
        }
        if(numUniqueValuesArray != nullptr)
        {
          numUniqueValuesArray->setValue(i, statsVector[i].uniqueValCount);
        }
      }
    }
  }

  return {};
}

/**
 * @struct ExecuteBoundsStatsCalculationsScanline
 * @brief OOC execution path for bounding-box statistics.
 * @tparam UseModeV Enables type-dispatched NeighborList mode output.
 *
 * Boxes are processed serially with contiguous row reads. Frequency statistics
 * use external merge sorting, so frequency scratch does not scale with the box
 * volume in RAM. A UUID-named directory isolates the temporary files. The
 * directory is removed on scope exit when possible.
 */
template <bool UseModeV = false>
struct ExecuteBoundsStatsCalculationsScanline
{
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const ComputeBoundingBoxStatsInputValues* inputValues, const ImageGeom& imageGeom, const Float32AbstractDataStore& unifiedBoundsStore,
                      const IDataArray& inputIDataArray, const std::atomic_bool& shouldCancel)
  {
    const usize numBounds = unifiedBoundsStore.getNumberOfTuples();
    const usize numBoundValues = unifiedBoundsStore.getSize();
    auto unifiedBounds = std::make_unique<float32[]>(numBoundValues);
    Result<> boundsResult = unifiedBoundsStore.copyIntoBuffer(0, nonstd::span<float32>(unifiedBounds.get(), numBoundValues));
    if(boundsResult.invalid())
    {
      return boundsResult;
    }

    const auto& inputStore = dynamic_cast<const DataArray<T>&>(inputIDataArray).getDataStoreRef();
    auto inputBuffer = std::make_unique<T[]>(k_ChunkSize);
    const bool calculateFrequencyStats = inputValues->CalculateMedian || inputValues->CalculateNumUniqueValues || inputValues->CalculateMode;
    if(calculateFrequencyStats)
    {
      std::error_code errorCode;
      const std::filesystem::path tempRoot = std::filesystem::temp_directory_path(errorCode);
      if(errorCode)
      {
        return MakeErrorResult(-69326, fmt::format("ComputeBoundingBoxStats: Failed to locate the temporary directory: {}", errorCode.message()));
      }
      TempDirectory tempDirectory(tempRoot / fmt::format("simplnx-compute-bounding-box-stats-{}", Uuid::GenerateV4().str()));
      if(!std::filesystem::create_directories(tempDirectory.path(), errorCode) || errorCode)
      {
        return MakeErrorResult(-69327, fmt::format("ComputeBoundingBoxStats: Failed to create temporary sort directory '{}': {}", tempDirectory.path().string(), errorCode.message()));
      }

      const std::filesystem::path sourcePath = tempDirectory.path() / "runs-a.bin";
      const std::filesystem::path destinationPath = tempDirectory.path() / "runs-b.bin";
      auto runBuffer = std::make_unique<T[]>(k_ChunkSize);
      std::vector<CompleteStatsCache<T>> statsVector(numBounds);
      NeighborList<T>* modesList = nullptr;
      if constexpr(UseModeV)
      {
        modesList = &dataStructure.getDataRefAs<NeighborList<T>>(inputValues->ModePath);
      }

      for(usize targetBoundsIndex = 0; targetBoundsIndex < numBounds; targetBoundsIndex++)
      {
        if(shouldCancel)
        {
          return {};
        }
        const std::array<usize, 6> voxelIndices = GetVoxelIndices(nonstd::span<const float32>(unifiedBounds.get(), numBoundValues), targetBoundsIndex, imageGeom);
        Result<std::filesystem::path> sortResult =
            StreamStatsAndSortedRuns(imageGeom, inputStore, voxelIndices, inputBuffer.get(), runBuffer.get(), sourcePath, destinationPath, shouldCancel, statsVector[targetBoundsIndex]);
        if(sortResult.invalid())
        {
          return ConvertResult(std::move(sortResult));
        }
        if(statsVector[targetBoundsIndex].count > 0)
        {
          Result<> frequencyResult = CalculateFrequencyStats(sortResult.value(), statsVector[targetBoundsIndex], modesList, targetBoundsIndex, shouldCancel);
          if(frequencyResult.invalid() || shouldCancel)
          {
            return frequencyResult;
          }
        }
      }

      if(inputValues->CalculateStdDev)
      {
        auto& stdDevStore = dataStructure.getDataRefAs<Float32Array>(inputValues->StdDevPath).getDataStoreRef();
        Result<> stdDevResult = StreamStdDeviation(imageGeom, inputStore, nonstd::span<const float32>(unifiedBounds.get(), numBoundValues), inputBuffer.get(), statsVector, stdDevStore, shouldCancel);
        if(stdDevResult.invalid() || shouldCancel)
        {
          return stdDevResult;
        }
      }
      return FillStatsArrays<T>(statsVector, dataStructure, inputValues);
    }

    std::vector<StatsCache<T>> statsVector(numBounds);
    for(usize targetBoundsIndex = 0; targetBoundsIndex < numBounds; targetBoundsIndex++)
    {
      if(shouldCancel)
      {
        return {};
      }
      const std::array<usize, 6> voxelIndices = GetVoxelIndices(nonstd::span<const float32>(unifiedBounds.get(), numBoundValues), targetBoundsIndex, imageGeom);
      Result<> statsResult = StreamBaseStats(imageGeom, inputStore, voxelIndices, inputBuffer.get(), shouldCancel, statsVector[targetBoundsIndex]);
      if(statsResult.invalid() || shouldCancel)
      {
        return statsResult;
      }
    }

    if(inputValues->CalculateStdDev)
    {
      auto& stdDevStore = dataStructure.getDataRefAs<Float32Array>(inputValues->StdDevPath).getDataStoreRef();
      Result<> stdDevResult = StreamStdDeviation(imageGeom, inputStore, nonstd::span<const float32>(unifiedBounds.get(), numBoundValues), inputBuffer.get(), statsVector, stdDevStore, shouldCancel);
      if(stdDevResult.invalid() || shouldCancel)
      {
        return stdDevResult;
      }
    }
    return FillStatsArrays<T>(statsVector, dataStructure, inputValues);
  }
};

} // namespace

// -----------------------------------------------------------------------------
ComputeBoundingBoxStatsScanline::ComputeBoundingBoxStatsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                 const ComputeBoundingBoxStatsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeBoundingBoxStatsScanline::~ComputeBoundingBoxStatsScanline() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeBoundingBoxStatsScanline::operator()()
{
  const auto& geom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GeometryPath);
  auto& unifiedArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->UnifiedPath).getDataStoreRef();
  auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->InputPath);
  if(inputArray.getDataType() == DataType::boolean)
  {
    return MakeErrorResult(-98500, "Boolean arrays cannot be used as inputs to this filter.");
  }
  if(m_InputValues->CalculateMode)
  {
    return ExecuteNeighborFunction(ExecuteBoundsStatsCalculationsScanline<true>{}, inputArray.getDataType(), m_DataStructure, m_InputValues, geom, unifiedArray, inputArray, m_ShouldCancel);
  }

  return ExecuteDataFunctionNoBool(ExecuteBoundsStatsCalculationsScanline<false>{}, inputArray.getDataType(), m_DataStructure, m_InputValues, geom, unifiedArray, inputArray, m_ShouldCancel);
}
