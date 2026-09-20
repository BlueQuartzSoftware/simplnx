#include "ComputeArrayHistogram.hpp"

#include "simplnx/Common/Uuid.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/HistogramUtilities.hpp"
#include "simplnx/Utilities/Math/StatisticsCalculations.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <system_error>
#include <type_traits>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize k_ChunkSize = 65536;

/**
 * @class TempDirectory
 * @brief Removes one unique modal-sort directory at scope exit.
 *
 * Cleanup uses the error-code overload and does not report removal failures.
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
 * @brief Reads one sorted binary run through a bounded value buffer.
 * @tparam T Stored value type.
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

template <typename T>
bool Equivalent(const T& lhs, const T& rhs)
{
  return !(lhs < rhs) && !(rhs < lhs);
}

/**
 * @brief Maps one modal value to a legacy uniform range calculation.
 * @tparam T Histogram value type.
 * @param binRanges Stored histogram range values.
 * @param mode Modal value.
 * @return Calculated lower and upper values, or two default values when outside the range.
 *
 * This calculation treats binRanges as one edge sequence. Histogram output
 * stores lower/upper pairs, so the result can differ from an actual stored bin.
 */
template <typename T>
std::pair<T, T> FindModalBinRange(nonstd::span<const T> binRanges, const T& mode)
{
  const usize numBins = binRanges.size() - 1;
  const T min = binRanges[0];
  const T max = binRanges[numBins];
  const T increment = (max - min) / static_cast<T>(numBins);
  if constexpr(std::is_floating_point_v<T>)
  {
    if(std::abs(increment) < 1E-10)
    {
      return {min, max};
    }
  }
  else if(increment == static_cast<T>(0))
  {
    return {min, max};
  }

  const auto bin = static_cast<int64>((mode - min) / increment);
  if((bin >= 0) && (bin < numBins))
  {
    return {static_cast<T>(min + (bin * increment)), static_cast<T>(min + ((bin + 1) * increment))};
  }
  if(mode == max)
  {
    return {static_cast<T>(min + ((bin - 1) * increment)), static_cast<T>(min + (bin * increment))};
  }
  return {};
}

/**
 * @class BufferedValueReader
 * @brief Visits selected input values through bounded source and mask pages.
 * @tparam T Input value type.
 *
 * The optional mask must be Bool or UInt8 and must match the input tuple count.
 */
template <typename T>
class BufferedValueReader
{
public:
  BufferedValueReader(const AbstractDataStore<T>& inputStore, const IDataArray* maskArray, const std::atomic_bool& shouldCancel)
  : m_InputStore(inputStore)
  , m_MaskArray(maskArray)
  , m_ShouldCancel(shouldCancel)
  , m_InputBuffer(std::make_unique<T[]>(k_ChunkSize))
  {
    if(maskArray != nullptr && maskArray->getDataType() == DataType::boolean)
    {
      m_BoolMaskBuffer = std::make_unique<bool[]>(k_ChunkSize);
    }
    else if(maskArray != nullptr)
    {
      m_UInt8MaskBuffer = std::make_unique<uint8[]>(k_ChunkSize);
    }
  }

  template <class FunctionT>
  Result<> forEach(usize numValues, FunctionT&& function)
  {
    for(usize offset = 0; offset < numValues; offset += k_ChunkSize)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const usize count = std::min(k_ChunkSize, numValues - offset);
      Result<> inputResult = m_InputStore.copyIntoBuffer(offset, nonstd::span<T>(m_InputBuffer.get(), count));
      if(inputResult.invalid())
      {
        return inputResult;
      }

      if(m_BoolMaskBuffer != nullptr)
      {
        const auto& maskStore = m_MaskArray->template getIDataStoreRefAs<AbstractDataStore<bool>>();
        Result<> maskResult = maskStore.copyIntoBuffer(offset, nonstd::span<bool>(m_BoolMaskBuffer.get(), count));
        if(maskResult.invalid())
        {
          return maskResult;
        }
      }
      else if(m_UInt8MaskBuffer != nullptr)
      {
        const auto& maskStore = m_MaskArray->template getIDataStoreRefAs<AbstractDataStore<uint8>>();
        Result<> maskResult = maskStore.copyIntoBuffer(offset, nonstd::span<uint8>(m_UInt8MaskBuffer.get(), count));
        if(maskResult.invalid())
        {
          return maskResult;
        }
      }

      for(usize index = 0; index < count; index++)
      {
        const bool selected = m_MaskArray == nullptr || (m_BoolMaskBuffer != nullptr ? m_BoolMaskBuffer[index] : m_UInt8MaskBuffer[index] != 0);
        if(selected)
        {
          function(m_InputBuffer[index], offset + index);
        }
      }
    }

    return {};
  }

private:
  const AbstractDataStore<T>& m_InputStore;
  const IDataArray* m_MaskArray = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  std::unique_ptr<T[]> m_InputBuffer;
  std::unique_ptr<bool[]> m_BoolMaskBuffer;
  std::unique_ptr<uint8[]> m_UInt8MaskBuffer;
};

/**
 * @class CalculateModalRangesDirect
 * @brief Computes exact modes from resident storage.
 * @tparam T Input value type.
 *
 * A masked calculation retains every selected value before reduction. Memory
 * can scale with the full selected value count.
 */
template <typename T>
class CalculateModalRangesDirect
{
public:
  CalculateModalRangesDirect(const AbstractDataStore<T>& inputStore, const IDataArray* maskArray, usize numValues, const AbstractDataStore<T>& binRangesStore, NeighborList<T>& modalBinRanges,
                             const std::atomic_bool& shouldCancel)
  : m_InputStore(inputStore)
  , m_MaskArray(maskArray)
  , m_NumValues(numValues)
  , m_BinRangesStore(binRangesStore)
  , m_ModalBinRanges(modalBinRanges)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> operator()() const
  {
    std::vector<T> modes;
    if(m_MaskArray != nullptr)
    {
      std::vector<T> selectedValues;
      selectedValues.reserve(m_NumValues);

      auto collectSelectedValues = [&](const auto& maskStore) -> bool {
        for(usize offset = 0; offset < m_NumValues; offset += k_ChunkSize)
        {
          if(m_ShouldCancel)
          {
            return false;
          }

          const usize end = std::min(offset + k_ChunkSize, m_NumValues);
          for(usize index = offset; index < end; index++)
          {
            if(maskStore[index])
            {
              selectedValues.push_back(m_InputStore[index]);
            }
          }
        }
        return true;
      };

      bool completed = false;
      if(m_MaskArray->getDataType() == DataType::boolean)
      {
        const auto& maskStore = m_MaskArray->template getIDataStoreRefAs<AbstractDataStore<bool>>();
        completed = collectSelectedValues(maskStore);
      }
      else
      {
        const auto& maskStore = m_MaskArray->template getIDataStoreRefAs<AbstractDataStore<uint8>>();
        completed = collectSelectedValues(maskStore);
      }
      if(!completed)
      {
        return {};
      }

      selectedValues.shrink_to_fit();
      modes = StatisticsCalculations::findModes(selectedValues);
    }
    else
    {
      modes = StatisticsCalculations::findModes(m_InputStore);
    }

    for(const T& mode : modes)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const auto modalRange = StatisticsCalculations::findModalBinRange(m_InputStore, m_BinRangesStore, mode);
      m_ModalBinRanges.addEntry(0, modalRange.first);
      m_ModalBinRanges.addEntry(0, modalRange.second);
    }
    return {};
  }

private:
  const AbstractDataStore<T>& m_InputStore;
  const IDataArray* m_MaskArray = nullptr;
  usize m_NumValues = 0;
  const AbstractDataStore<T>& m_BinRangesStore;
  NeighborList<T>& m_ModalBinRanges;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief Computes exact modes with a bounded external merge sort.
 * @tparam T Input value type.
 * @param inputStore Supplies scalar values.
 * @param maskArray Optional Bool or UInt8 tuple mask.
 * @param numValues Number of values to inspect.
 * @param binRangesStore Supplies stored histogram range pairs.
 * @param modalBinRanges Receives two values for each exact mode.
 * @param shouldCancel Signals cancellation between pages and merge passes.
 * @return Bulk-I/O, temporary-directory, temporary-file, or merge errors.
 *
 * Two alternating binary files can coexist during merge. Each file scales with
 * the selected value count. The temporary directory is removed at function exit.
 */
template <typename T>
Result<> CalculateModalRangesScanlineImpl(const AbstractDataStore<T>& inputStore, const IDataArray* maskArray, usize numValues, const AbstractDataStore<T>& binRangesStore,
                                          NeighborList<T>& modalBinRanges, const std::atomic_bool& shouldCancel)
{
  std::error_code errorCode;
  const std::filesystem::path tempRoot = std::filesystem::temp_directory_path(errorCode);
  if(errorCode)
  {
    return MakeErrorResult(-23767, fmt::format("ComputeArrayHistogram: Failed to locate the temporary directory: {}", errorCode.message()));
  }

  TempDirectory tempDirectory(tempRoot / fmt::format("simplnx-compute-array-histogram-{}", Uuid::GenerateV4().str()));
  if(!std::filesystem::create_directories(tempDirectory.path(), errorCode) || errorCode)
  {
    return MakeErrorResult(-23768, fmt::format("ComputeArrayHistogram: Failed to create temporary modal-sort directory '{}': {}", tempDirectory.path().string(), errorCode.message()));
  }

  std::filesystem::path sourcePath = tempDirectory.path() / "runs-a.bin";
  std::filesystem::path destinationPath = tempDirectory.path() / "runs-b.bin";
  usize selectedCount = 0;
  {
    std::ofstream runStream(sourcePath, std::ios::binary | std::ios::trunc);
    if(!runStream.is_open())
    {
      return MakeErrorResult(-23769, fmt::format("ComputeArrayHistogram: Failed to create temporary modal-sort file '{}'.", sourcePath.string()));
    }

    auto runBuffer = std::make_unique<T[]>(k_ChunkSize);
    usize runCount = 0;
    bool writeFailed = false;
    BufferedValueReader<T> inputReader(inputStore, maskArray, shouldCancel);
    Result<> readResult = inputReader.forEach(numValues, [&](T value, usize) {
      if(writeFailed)
      {
        return;
      }

      runBuffer[runCount++] = value;
      selectedCount++;
      if(runCount == k_ChunkSize)
      {
        std::stable_sort(runBuffer.get(), runBuffer.get() + runCount);
        writeFailed = !WriteBuffer(runStream, runBuffer.get(), runCount);
        runCount = 0;
      }
    });
    if(readResult.invalid() || shouldCancel)
    {
      return readResult;
    }
    if(!writeFailed && runCount > 0)
    {
      std::stable_sort(runBuffer.get(), runBuffer.get() + runCount);
      writeFailed = !WriteBuffer(runStream, runBuffer.get(), runCount);
    }
    runStream.flush();
    writeFailed = writeFailed || runStream.fail();
    if(writeFailed)
    {
      return MakeErrorResult(-23770, fmt::format("ComputeArrayHistogram: Failed while writing temporary modal-sort file '{}'.", sourcePath.string()));
    }
  }

  if(selectedCount == 0)
  {
    return {};
  }

  auto outputBuffer = std::make_unique<T[]>(k_ChunkSize);
  usize runWidth = k_ChunkSize;
  while(runWidth < selectedCount)
  {
    if(shouldCancel)
    {
      return {};
    }

    std::ofstream destinationStream(destinationPath, std::ios::binary | std::ios::trunc);
    if(!destinationStream.is_open())
    {
      return MakeErrorResult(-23771, fmt::format("ComputeArrayHistogram: Failed to create temporary modal-merge file '{}'.", destinationPath.string()));
    }

    usize outputCount = 0;
    const usize mergeWidth = runWidth > selectedCount - runWidth ? selectedCount : 2 * runWidth;
    usize leftStart = 0;
    while(leftStart < selectedCount)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize leftCount = std::min(runWidth, selectedCount - leftStart);
      const usize rightStart = leftStart + leftCount;
      const usize rightCount = std::min(runWidth, selectedCount - rightStart);
      BinaryRunReader<T> leftReader(sourcePath, leftStart, leftCount);
      BinaryRunReader<T> rightReader(sourcePath, rightStart, rightCount);

      bool leftAvailable = leftReader.hasValue();
      bool rightAvailable = rightReader.hasValue();
      while(leftAvailable || rightAvailable)
      {
        if(leftReader.failed() || rightReader.failed())
        {
          return MakeErrorResult(-23772, fmt::format("ComputeArrayHistogram: Failed while reading temporary modal-sort file '{}'.", sourcePath.string()));
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
            return MakeErrorResult(-23773, fmt::format("ComputeArrayHistogram: Failed while writing temporary modal-merge file '{}'.", destinationPath.string()));
          }
          outputCount = 0;
          if(shouldCancel)
          {
            return {};
          }
        }
      }
      if(leftReader.failed() || rightReader.failed())
      {
        return MakeErrorResult(-23772, fmt::format("ComputeArrayHistogram: Failed while reading temporary modal-sort file '{}'.", sourcePath.string()));
      }
      if(selectedCount - leftStart <= mergeWidth)
      {
        break;
      }
      leftStart += mergeWidth;
    }

    if(outputCount > 0 && !WriteBuffer(destinationStream, outputBuffer.get(), outputCount))
    {
      return MakeErrorResult(-23773, fmt::format("ComputeArrayHistogram: Failed while writing temporary modal-merge file '{}'.", destinationPath.string()));
    }
    destinationStream.close();
    if(destinationStream.fail())
    {
      return MakeErrorResult(-23773, fmt::format("ComputeArrayHistogram: Failed while closing temporary modal-merge file '{}'.", destinationPath.string()));
    }
    std::swap(sourcePath, destinationPath);
    runWidth = runWidth > selectedCount / 2 ? selectedCount : runWidth * 2;
  }

  auto scanGroups = [&](auto&& groupFunction) -> Result<> {
    BinaryRunReader<T> sortedReader(sourcePath, 0, selectedCount);
    std::optional<T> currentValue;
    uint64 currentCount = 0;
    while(sortedReader.hasValue())
    {
      if(shouldCancel)
      {
        return {};
      }

      const T value = sortedReader.value();
      sortedReader.advance();
      if(currentValue.has_value() && Equivalent(currentValue.value(), value))
      {
        currentCount++;
      }
      else
      {
        if(currentValue.has_value())
        {
          groupFunction(currentValue.value(), currentCount);
        }
        currentValue = value;
        currentCount = 1;
      }
    }
    if(sortedReader.failed())
    {
      return MakeErrorResult(-23774, fmt::format("ComputeArrayHistogram: Failed while scanning sorted modal values in '{}'.", sourcePath.string()));
    }
    if(currentValue.has_value())
    {
      groupFunction(currentValue.value(), currentCount);
    }
    return {};
  };

  uint64 maxCount = 0;
  Result<> countResult = scanGroups([&](T, uint64 count) { maxCount = std::max(maxCount, count); });
  if(countResult.invalid() || shouldCancel)
  {
    return countResult;
  }

  const usize numBinRangeValues = binRangesStore.getSize();
  auto binRanges = std::make_unique<T[]>(numBinRangeValues);
  Result<> binRangeResult = binRangesStore.copyIntoBuffer(0, nonstd::span<T>(binRanges.get(), numBinRangeValues));
  if(binRangeResult.invalid())
  {
    return binRangeResult;
  }
  const nonstd::span<const T> binRangeSpan(binRanges.get(), numBinRangeValues);
  Result<> outputResult = scanGroups([&](T mode, uint64 count) {
    if(count == maxCount)
    {
      const auto modalRange = FindModalBinRange(binRangeSpan, mode);
      modalBinRanges.addEntry(0, modalRange.first);
      modalBinRanges.addEntry(0, modalRange.second);
    }
  });
  return outputResult;
}

/**
 * @class CalculateModalRangesScanline
 * @brief Adapts external modal sorting to algorithm dispatch.
 * @tparam T Input value type.
 */
template <typename T>
class CalculateModalRangesScanline
{
public:
  CalculateModalRangesScanline(const AbstractDataStore<T>& inputStore, const IDataArray* maskArray, usize numValues, const AbstractDataStore<T>& binRangesStore, NeighborList<T>& modalBinRanges,
                               const std::atomic_bool& shouldCancel)
  : m_InputStore(inputStore)
  , m_MaskArray(maskArray)
  , m_NumValues(numValues)
  , m_BinRangesStore(binRangesStore)
  , m_ModalBinRanges(modalBinRanges)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> operator()() const
  {
    return CalculateModalRangesScanlineImpl(m_InputStore, m_MaskArray, m_NumValues, m_BinRangesStore, m_ModalBinRanges, m_ShouldCancel);
  }

private:
  const AbstractDataStore<T>& m_InputStore;
  const IDataArray* m_MaskArray = nullptr;
  usize m_NumValues = 0;
  const AbstractDataStore<T>& m_BinRangesStore;
  NeighborList<T>& m_ModalBinRanges;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @struct ComputeHistogramFunctor
 * @brief Computes one typed histogram through bounded passes.
 */
struct ComputeHistogramFunctor
{
  template <typename T>
  Result<> operator()(const IDataArray& inputArray, IDataArray& binRangesArray, AbstractDataStore<uint64>& countsStore, AbstractDataStore<uint64>& mostPopulatedStore, INeighborList* modalBinRanges,
                      const IDataArray* maskArray, const ComputeArrayHistogramInputValues& inputValues, const std::atomic_bool& shouldCancel, usize& overflow) const
  {
    const auto& inputStore = inputArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& binRangesStore = binRangesArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    const usize numValues = maskArray == nullptr ? inputStore.getSize() : inputStore.getNumberOfTuples();
    BufferedValueReader<T> reader(inputStore, maskArray, shouldCancel);

    std::pair<T, T> range = {static_cast<T>(inputValues.MinRange), static_cast<T>(inputValues.MaxRange)};
    if(inputValues.UserDefinedRange && inputValues.MinRange > inputValues.MaxRange)
    {
      return MakeErrorResult(-23760,
                             fmt::format("GenerateHistogramFunctor: The range min value is larger than the max value. Min value: {} | Max Value: {}", inputValues.MinRange, inputValues.MaxRange));
    }
    if(!inputValues.UserDefinedRange)
    {
      bool hasValue = false;
      Result<> rangeResult = reader.forEach(numValues, [&](T value, usize) {
        if(!hasValue)
        {
          range = {value, value};
          hasValue = true;
        }
        else
        {
          range.first = std::min(range.first, value);
          range.second = std::max(range.second, value);
        }
      });
      if(rangeResult.invalid() || shouldCancel)
      {
        return rangeResult;
      }
      if(!hasValue)
      {
        return MakeErrorResult(-23766, fmt::format("ComputeArrayHistogram: Input array '{}' has no values selected by the mask.", inputArray.getName()));
      }
      range.second += static_cast<T>(1.0);
    }

    const int32 numBins = inputValues.NumberOfBins;
    const float32 increment = HistogramUtilities::serial::CalculateIncrement(range.first, range.second, numBins);
    auto binRanges = std::make_unique<T[]>(static_cast<usize>(numBins) * 2);
    HistogramUtilities::serial::FillBinRanges(binRanges, range, numBins, increment);
    Result<> rangesResult = binRangesStore.copyFromBuffer(0, nonstd::span<const T>(binRanges.get(), static_cast<usize>(numBins) * 2));
    if(rangesResult.invalid())
    {
      return rangesResult;
    }

    auto counts = std::make_unique<uint64[]>(static_cast<usize>(numBins));
    std::fill_n(counts.get(), numBins, uint64{0});
    Result<> histogramResult = reader.forEach(numValues, [&](T value, usize) {
      const auto bin = HistogramUtilities::serial::CalculateBin(value, range.first, increment);
      if((bin >= 0) && (bin < numBins))
      {
        counts[static_cast<usize>(bin)]++;
      }
      else
      {
        overflow++;
      }
    });
    if(histogramResult.invalid() || shouldCancel)
    {
      return histogramResult;
    }

    Result<> countsResult = countsStore.copyFromBuffer(0, nonstd::span<const uint64>(counts.get(), static_cast<usize>(numBins)));
    if(countsResult.invalid())
    {
      return countsResult;
    }
    const auto maxElement = std::max_element(counts.get(), counts.get() + numBins);
    mostPopulatedStore.setComponent(0, 0, static_cast<uint64>(std::distance(counts.get(), maxElement)));
    mostPopulatedStore.setComponent(0, 1, *maxElement);

    if(modalBinRanges != nullptr)
    {
      // NeighborList<bool> is not an exported supported type, so Boolean modal
      // ranges remain empty.
      if constexpr(!std::is_same_v<T, bool>)
      {
        auto& typedModalRanges = dynamic_cast<NeighborList<T>&>(*modalBinRanges);
        Result<> modalResult = DispatchAlgorithm<CalculateModalRangesDirect<T>, CalculateModalRangesScanline<T>>({&inputArray, maskArray}, inputStore, maskArray, numValues, binRangesStore,
                                                                                                                 typedModalRanges, shouldCancel);
        if(modalResult.invalid())
        {
          return modalResult;
        }
      }
    }

    return {};
  }
};
} // namespace

ComputeArrayHistogram::ComputeArrayHistogram(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                             ComputeArrayHistogramInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

ComputeArrayHistogram::~ComputeArrayHistogram() noexcept = default;

Result<> ComputeArrayHistogram::operator()()
{
  const IDataArray* maskArray = nullptr;
  if(m_InputValues->MaskPath.has_value())
  {
    maskArray = &m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskPath.value());
  }

  MessageHelper messageHelper(m_MessageHandler);

  for(usize index = 0; index < m_InputValues->SelectedArrayPaths.size(); index++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto& inputData = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SelectedArrayPaths[index]);
    auto& binRanges = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->CreatedBinRangeDataPaths.at(index));
    auto& counts = m_DataStructure.getDataRefAs<DataArray<uint64>>(m_InputValues->CreatedHistogramCountsDataPaths.at(index)).getDataStoreRef();
    auto& mostPopulated = m_DataStructure.getDataRefAs<DataArray<uint64>>(m_InputValues->CreatedBinMostPopulatedDataPaths.at(index)).getDataStoreRef();
    INeighborList* modalBinRanges = nullptr;
    if(m_InputValues->CreatedBinModalRangesDataPaths.has_value())
    {
      modalBinRanges = &m_DataStructure.getDataRefAs<INeighborList>(m_InputValues->CreatedBinModalRangesDataPaths->at(index));
    }

    usize overflow = 0;
    Result<> result =
        ExecuteDataFunction(ComputeHistogramFunctor{}, inputData.getDataType(), inputData, binRanges, counts, mostPopulated, modalBinRanges, maskArray, *m_InputValues, m_ShouldCancel, overflow);
    if(result.invalid())
    {
      return result;
    }
    if(overflow > 0)
    {
      messageHelper.sendMessage(fmt::format("{} values not categorized into bin for array {}", overflow, inputData.getName()));
    }
  }

  return {};
}
