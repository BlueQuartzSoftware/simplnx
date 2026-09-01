#include "OStreamUtilities.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/Bit.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <chrono>
#include <iomanip>
#include <memory>
#include <ostream>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;

namespace // for nonmember functions
{
// Delimiter underlying values index this table. Keep the enum and table order equal.
const std::array<std::string, 5> k_DelimiterStrings = {" ", ";", ",", ":", "\t"};

/**
 * @struct PrintNeighborList
 * @brief Dispatches delimited text output for one neighbor-list value type.
 */
struct PrintNeighborList
{
  /**
   * @brief Writes all lists and optional index and header fields.
   * @tparam ScalarType Specifies the neighbor value type.
   * @param outputStrm Receives text output.
   * @param inputNeighborList Supplies the runtime-validated neighbor list.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Supplies the cancellation flag.
   * @param delimiter Specifies field separation.
   * @param hasIndex True to write each list index.
   * @param hasHeader True to write column names.
   * @return Valid result. This function does not report stream failures.
   * @pre inputNeighborList is non-null and has the dispatched ScalarType.
   *
   * Cancellation is checked with throttled progress and can leave partial output.
   */
  template <typename ScalarType>
  Result<> operator()(std::ostream& outputStrm, INeighborList* inputNeighborList, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const std::string& delimiter = ",",
                      bool hasIndex = false, bool hasHeader = false)
  {
    auto& neighborList = *dynamic_cast<NeighborList<ScalarType>*>(inputNeighborList);
    auto start = std::chrono::steady_clock::now();
    auto numLists = neighborList.getNumberOfLists();

    if(hasHeader)
    {
      if(hasIndex)
      {
        outputStrm << "Feature_IDs" << delimiter;
      }
      outputStrm << "NumNeighbors" << delimiter << inputNeighborList->getName() << "\n";
    }
    if(hasIndex)
    {
      for(size_t list = 0; list < numLists; list++)
      {
        auto now = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
        {
          auto string = fmt::format("Processing {}: {}% completed", neighborList.getName(), static_cast<int32>(100 * static_cast<float>(list) / static_cast<float>(numLists)));
          mesgHandler(IFilter::Message::Type::Info, string);
          start = now;
          if(shouldCancel)
          {
            return {};
          }
        }
        const auto grain = neighborList.at(list);
        outputStrm << list << delimiter << grain.size() << delimiter;
        for(size_t index = 0; index < grain.size(); index++)
        {
          if constexpr(std::is_same_v<ScalarType, int8> || std::is_same_v<ScalarType, uint8>)
          {
            outputStrm << static_cast<int32>(grain[index]);
          }
          else if constexpr(std::is_same_v<ScalarType, float32> || std::is_same_v<ScalarType, float64>)
          {
            outputStrm << fmt::format("{}", grain[index]);
          }
          else
          {
            outputStrm << grain[index];
          }
          if(index != grain.size() - 1)
          {
            outputStrm << delimiter;
          }
        }
        outputStrm << "\n";
      }
    }
    else
    {
      for(size_t list = 0; list < neighborList.getNumberOfLists(); list++)
      {
        auto now = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
        {
          auto string = fmt::format("Processing {}: {}% completed", neighborList.getName(), static_cast<int32>(static_cast<float>(list) / static_cast<float>(numLists)));
          mesgHandler(IFilter::Message::Type::Info, string);
          start = now;
          if(shouldCancel)
          {
            return {};
          }
        }
        const auto grain = neighborList.at(list);
        outputStrm << grain.size() << delimiter;
        for(size_t index = 0; index < grain.size(); index++)
        {
          if constexpr(std::is_same_v<ScalarType, int8> || std::is_same_v<ScalarType, uint8>)
          {
            outputStrm << static_cast<int32>(grain[index]);
          }
          else if constexpr(std::is_same_v<ScalarType, float32> || std::is_same_v<ScalarType, float64>)
          {
            outputStrm << fmt::format("{}", grain[index]);
          }
          else
          {
            outputStrm << grain[index];
          }
          if(index != grain.size() - 1)
          {
            outputStrm << delimiter;
          }
        }
        outputStrm << "\n";
      }
    }
    return {};
  }
};

/**
 * @struct PrintDataArray
 * @brief Writes a numeric DataArray as delimited text using bounded contiguous reads.
 *
 * A fixed byte target sets the page size. Resident and disk-backed stores use
 * the same path. Memory does not grow with the full array. Tuple-wise formatting
 * preserves the file layout.
 */
struct PrintDataArray
{
  /**
   * @brief Writes one runtime-typed numeric array as text.
   * @tparam ScalarType Specifies the array value type.
   * @param outputStrm Receives text output.
   * @param inputDataArray Supplies the numeric array.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Supplies the cancellation flag.
   * @param delimiter Specifies value separation.
   * @param tuplesPerLine Specifies tuples per line. Zero selects one.
   * @return Valid result or the first bounded store-read error.
   * @pre The array has at least one component. tuplesPerLine is nonnegative.
   * @pre Component and page-size products fit usize.
   *
   * Cancellation returns a valid result and can leave partial output. Text stream
   * failures are not reported by this function.
   */
  template <typename ScalarType>
  Result<> operator()(std::ostream& outputStrm, const IDataArray& inputDataArray, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const std::string& delimiter = ",",
                      int32 tuplesPerLine = 0)
  {
    const auto& dataStore = inputDataArray.template getIDataStoreRefAs<AbstractDataStore<ScalarType>>();
    auto start = std::chrono::steady_clock::now();
    auto numTuples = inputDataArray.getNumberOfTuples();
    if(tuplesPerLine == 0)
    {
      tuplesPerLine = 1;
    }

    MessageHelper messageHelper(mesgHandler);
    ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();

    usize numComps = inputDataArray.getNumberOfComponents();
    int32 tuplesWritten = 0;
    constexpr usize k_TargetBufferBytes = 1024 * 1024;
    const usize tuplesPerBuffer = std::max<usize>(1, k_TargetBufferBytes / (sizeof(ScalarType) * numComps));
    const usize bufferElements = std::min(numTuples, tuplesPerBuffer) * numComps;
    auto values = std::make_unique<ScalarType[]>(std::max<usize>(1, bufferElements));
    for(usize tupleOffset = 0; tupleOffset < numTuples; tupleOffset += tuplesPerBuffer)
    {
      const usize tupleCount = std::min(tuplesPerBuffer, numTuples - tupleOffset);
      Result<> readResult = dataStore.copyIntoBuffer(tupleOffset * numComps, nonstd::span<ScalarType>(values.get(), tupleCount * numComps));
      if(readResult.invalid())
      {
        return readResult;
      }

      for(usize localTuple = 0; localTuple < tupleCount; localTuple++)
      {
        const usize tuple = tupleOffset + localTuple;
        throttledMessenger.sendThrottledMessage(
            [&]() { return fmt::format("Processing {}: {}% completed", inputDataArray.getName(), static_cast<int32>(100 * static_cast<float>(tuple) / static_cast<float>(numTuples))); });
        if(shouldCancel)
        {
          return {};
        }

        for(size_t index = 0; index < numComps; index++)
        {
          const ScalarType value = values[localTuple * numComps + index];
          if constexpr(std::is_same_v<ScalarType, int8> || std::is_same_v<ScalarType, uint8>)
          {
            outputStrm << static_cast<int32>(value);
          }
          else if constexpr(std::is_same_v<ScalarType, float32> || std::is_same_v<ScalarType, float64>)
          {
            outputStrm << fmt::format("{}", value);
          }
          else
          {
            outputStrm << value;
          }
          if(index != numComps - 1)
          {
            outputStrm << delimiter;
          }
        }
        // A tuple group ends with a newline. Other tuples end with the delimiter.
        tuplesWritten++;
        if(tuplesWritten == tuplesPerLine)
        {
          outputStrm << '\n';
          tuplesWritten = 0;
        }
        else
        {
          outputStrm << delimiter;
        }
      }
    }
    return {};
  }
};

/**
 * @struct PrintBinaryDataArray
 * @brief Writes a numeric DataArray in bounded contiguous pages. Byte swapping,
 * when requested, is applied only to the caller-owned page and never mutates the
 * source array.
 */
struct PrintBinaryDataArray
{
  /**
   * @brief Streams one runtime-typed array in fixed-size pages.
   * @tparam ScalarType Specifies the array value type.
   * @param outputStrm Receives binary output.
   * @param inputDataArray Supplies the numeric array.
   * @param shouldCancel Supplies the cancellation flag.
   * @param swapEndian True to byte-swap each temporary value.
   * @return First store-read or stream-write error, or a valid result.
   *
   * Cancellation returns a valid result and can leave partial output.
   */
  template <typename ScalarType>
  Result<> operator()(std::ostream& outputStrm, const IDataArray& inputDataArray, const std::atomic_bool& shouldCancel, bool swapEndian)
  {
    const auto& dataStore = inputDataArray.template getIDataStoreRefAs<AbstractDataStore<ScalarType>>();
    constexpr usize k_TargetBufferBytes = 1024 * 1024;
    const usize totalElements = dataStore.getSize();
    const usize bufferElements = std::max<usize>(1, std::min(totalElements, k_TargetBufferBytes / sizeof(ScalarType)));
    auto buffer = std::make_unique<ScalarType[]>(bufferElements);

    for(usize offset = 0; offset < totalElements; offset += bufferElements)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize count = std::min(bufferElements, totalElements - offset);
      Result<> readResult = dataStore.copyIntoBuffer(offset, nonstd::span<ScalarType>(buffer.get(), count));
      if(readResult.invalid())
      {
        return readResult;
      }

      if(swapEndian)
      {
        std::transform(buffer.get(), buffer.get() + count, buffer.get(), [](ScalarType value) { return nx::core::byteswap(value); });
      }

      outputStrm.write(reinterpret_cast<const char*>(buffer.get()), static_cast<std::streamsize>(sizeof(ScalarType) * count));
      if(outputStrm.bad())
      {
        return MakeErrorResult(-10175, fmt::format("Error writing binary data for array '{}'.", inputDataArray.getName()));
      }
    }
    return {};
  }
};

/**
 * @brief Writes one string value per line.
 * @param outputStrm Receives text output.
 * @param inputStringArray Supplies string tuples.
 * @param mesgHandler Receives progress messages.
 * @param shouldCancel Supplies the cancellation flag.
 * @param delimiter Reserved for API consistency. This function does not use it.
 * @return Valid result. This function does not report stream failures.
 *
 * Cancellation is checked with throttled progress and can leave partial output.
 */
Result<> PrintStringArray(std::ostream& outputStrm, const StringArray& inputStringArray, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                          const std::string& delimiter = ",")
{
  auto start = std::chrono::steady_clock::now();
  auto numTuples = inputStringArray.getNumberOfTuples();

  for(size_t tuple = 0; tuple < numTuples; tuple++)
  {
    auto now = std::chrono::steady_clock::now();
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
    {
      auto string = fmt::format("Processing {}: {}% completed", inputStringArray.getName(), static_cast<int32>(100 * static_cast<float>(tuple) / static_cast<float>(numTuples)));
      mesgHandler(IFilter::Message::Type::Info, string);
      start = now;
      if(shouldCancel)
      {
        return {};
      }
    }
    outputStrm << inputStringArray[tuple] << "\n";
  }

  return {};
}

/**
 * @class ITupleWriter
 * @brief Provides type-erased tuple interleaving for a shared ASCII row.
 *
 * Implementations retain only references to source arrays and any bounded read
 * cache. They must not outlive the referenced DataStructure objects.
 */
class ITupleWriter
{
public:
  ITupleWriter() = default;
  virtual ~ITupleWriter() = default;
  /**
   * @brief Appends one tuple to a caller-owned stream.
   * @param outputStrm Receives tuple values.
   * @param tupleIndex Identifies the source tuple.
   */
  virtual void write(std::ostream& outputStrm, usize tupleIndex) const = 0;

  /**
   * @brief Appends this array's column names.
   * @param outputStrm Receives header text.
   */
  virtual void writeHeader(std::ostream& outputStrm) const = 0;
};

/**
 * @class StringTupleWriter
 * @brief Writes quoted StringArray tuples without numeric type dispatch.
 */
class StringTupleWriter : public ITupleWriter
{
  using DataArrayType = StringArray;

public:
  /**
   * @brief Creates a writer with one borrowed source array.
   * @param iDataArray Supplies the array and must outlive this writer.
   * @param delimiter Specifies the copied quote or delimiter text.
   */
  StringTupleWriter(const StringArray& iDataArray, const std::string& delimiter)
  : m_DataArray(dynamic_cast<const StringArray&>(iDataArray))
  , m_Delimiter(delimiter)
  {
  }
  ~StringTupleWriter() override = default;

  StringTupleWriter(const StringTupleWriter&) = delete;
  StringTupleWriter(StringTupleWriter&&) noexcept = delete;

  StringTupleWriter& operator=(const StringTupleWriter&) = delete;
  StringTupleWriter& operator=(StringTupleWriter&&) noexcept = delete;

  /**
   * @brief Appends one surrounded string tuple.
   * @param outputStrm Receives the value.
   * @param tupleIndex Identifies the source tuple.
   */
  void write(std::ostream& outputStrm, usize tupleIndex) const override
  {
    outputStrm << m_Delimiter << m_DataArray[tupleIndex] << m_Delimiter;
  }

  /**
   * @brief Appends the single StringArray column name.
   * @param outputStrm Receives the header.
   */
  void writeHeader(std::ostream& outputStrm) const override
  {
    outputStrm << m_DataArray.getName();
  }

private:
  const DataArrayType& m_DataArray;
  const std::string m_Delimiter = "'";
};

/**
 * @class TupleWriter
 * @brief Numeric tuple writer with a one-megabyte forward read cache.
 * @tparam ScalarType Specifies the numeric array value type.
 *
 * PrintDataSetsToSingleFile requests tuples in increasing order. Caching the
 * surrounding page converts those requests into bulk store reads while keeping memory
 * bounded independently of array size. A nonsequential request simply replaces
 * the cache with a page beginning at that tuple.
 */
template <typename ScalarType>
class TupleWriter : public ITupleWriter
{
  using DataArrayType = DataArray<ScalarType>;

public:
  /**
   * @brief Borrows one typed source store and allocates its fixed one-megabyte forward page.
   * @param iDataArray Runtime-validated source array that must outlive this writer.
   * @param delimiter Borrowed component delimiter that must outlive this writer.
   * @pre The source has at least one component. Page-size products fit usize.
   */
  TupleWriter(const IDataArray& iDataArray, const std::string& delimiter)
  : m_Name(iDataArray.getName())
  , m_DataStore(iDataArray.template getIDataStoreRefAs<AbstractDataStore<ScalarType>>())
  , m_Delimiter(delimiter)
  {
    m_NumComps = m_DataStore.getNumberOfComponents();
    constexpr usize k_TargetBufferBytes = 1024 * 1024;
    m_TuplesPerBuffer = std::max<usize>(1, k_TargetBufferBytes / (sizeof(ScalarType) * m_NumComps));
    m_Values = std::make_unique<ScalarType[]>(m_TuplesPerBuffer * m_NumComps);
  }
  ~TupleWriter() override = default;

  /**
   * @brief Appends one numeric tuple and refills the bounded page when necessary.
   * @param outputStrm Receives tuple values.
   * @param tupleIndex Identifies the source tuple.
   * @throws std::runtime_error When the source DataStore page cannot be read.
   * @pre tupleIndex is less than the source tuple count. The source has at least one component.
   *
   * Stream failures are not reported.
   */
  void write(std::ostream& outputStrm, usize tupleIndex) const override
  {
    if(tupleIndex < m_BufferStartTuple || tupleIndex >= m_BufferStartTuple + m_BufferTupleCount)
    {
      m_BufferStartTuple = tupleIndex;
      m_BufferTupleCount = std::min(m_TuplesPerBuffer, m_DataStore.getNumberOfTuples() - tupleIndex);
      // Sequential tuple requests perform one store read for each forward page.
      Result<> readResult = m_DataStore.copyIntoBuffer(tupleIndex * m_NumComps, nonstd::span<ScalarType>(m_Values.get(), m_BufferTupleCount * m_NumComps));
      if(readResult.invalid())
      {
        throw std::runtime_error(fmt::format("Failed to bulk-read data array '{}' while writing tuple {}.", m_Name, tupleIndex));
      }
    }

    const usize localTuple = tupleIndex - m_BufferStartTuple;
    for(usize comp = 0; comp < m_NumComps; comp++)
    {
      const ScalarType value = m_Values[localTuple * m_NumComps + comp];
      if constexpr(std::is_same_v<ScalarType, int8> || std::is_same_v<ScalarType, uint8>)
      {
        outputStrm << static_cast<int32>(value);
      }
      else if constexpr(std::is_same_v<ScalarType, float32>)
      {
        outputStrm << std::setprecision(8) << std::noshowpoint << value;
      }
      else if constexpr(std::is_same_v<ScalarType, float64>)
      {
        outputStrm << std::setprecision(16) << std::noshowpoint << value;
      }
      else
      {
        outputStrm << value;
      }
      if(comp < m_NumComps - 1)
      {
        outputStrm << m_Delimiter;
      }
    }
  }

  /**
   * @brief Appends one column name for each component.
   * @param outputStrm Receives header text.
   */
  void writeHeader(std::ostream& outputStrm) const override
  {
    if(m_NumComps == 1)
    {
      outputStrm << m_Name;
      return;
    }

    for(size_t index = 0; index < m_NumComps; index++)
    {
      outputStrm << m_Name << "_" << index;

      if(index < m_NumComps - 1)
      {
        outputStrm << m_Delimiter;
      }
    }
  }

private:
  const std::string m_Name;
  const AbstractDataStore<ScalarType>& m_DataStore;
  const std::string& m_Delimiter = ",";
  usize m_NumComps = 1;
  usize m_TuplesPerBuffer = 1;
  mutable std::unique_ptr<ScalarType[]> m_Values;
  mutable usize m_BufferStartTuple = std::numeric_limits<usize>::max();
  mutable usize m_BufferTupleCount = 0;
};

/**
 * @struct AddTupleWriter
 * @brief Creates the numeric TupleWriter selected by runtime type dispatch.
 */
struct AddTupleWriter
{
  /**
   * @brief Constructs and appends one typed tuple writer.
   * @tparam ScalarType Specifies the numeric array value type.
   * @param writers Receives the new writer.
   * @param iDataArray Supplies the source array.
   * @param delimiter Supplies the component delimiter and must outlive the writer.
   * @return Valid result.
   */
  template <typename ScalarType>
  Result<> operator()(std::vector<std::shared_ptr<ITupleWriter>>& writers, const IDataArray& iDataArray, const std::string& delimiter)
  {
    writers.push_back(std::make_shared<TupleWriter<ScalarType>>(iDataArray, delimiter));
    return {};
  }
};
} // namespace

namespace nx::core::OStreamUtilities
{
std::string DelimiterToString(uint64 delim)
{
  return k_DelimiterStrings[delim];
};

Result<> PrintDataSetsToMultipleFiles(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, const std::string& directoryPath, const IFilter::MessageHandler& mesgHandler,
                                      const std::atomic_bool& shouldCancel, const std::string& fileExtension, bool exportToBinary, const std::string& delimiter, bool includeIndex, bool includeHeaders,
                                      size_t tuplesPerLine, bool swapEndian)
{
  fs::path dirPath(directoryPath);
  if(!fs::is_directory(dirPath))
  {
    throw std::runtime_error(fmt::format("{}({}): Function {}: Error. OutputPath must be a directory. '{}'", "PrintDataSetsToMultipleFiles", __FILE__, __LINE__, directoryPath));
  }

  for(const auto& dataPath : objectPaths)
  {
    auto atomicFileResult = AtomicFile::Create(fmt::format("{}/{}{}", directoryPath, dataPath.getTargetName(), fileExtension));
    if(atomicFileResult.invalid())
    {
      return ConvertResult(std::move(atomicFileResult));
    }

    AtomicFile atomicFile = std::move(atomicFileResult.value());

    auto outputFilePath = atomicFile.tempFilePath().string();
    mesgHandler(IFilter::Message::Type::Info, fmt::format("Writing IArray ({}) to output file {}", dataPath.getTargetName(), outputFilePath));

    // Close the stream before AtomicFile renames its temporary file. Windows does
    // not permit that rename while this stream still owns the file handle.
    {
      std::ofstream outStrm(outputFilePath, std::ios_base::out | std::ios_base::binary);

      Result<> result;
      auto* dataArray = dataStructure.getDataAs<IDataArray>(dataPath);
      if(dataArray != nullptr)
      {
        if(exportToBinary)
        {
          result = ExecuteDataFunction(PrintBinaryDataArray{}, dataArray->getDataType(), outStrm, *dataArray, shouldCancel, swapEndian);
        }
        else
        {
          result = ExecuteDataFunction(PrintDataArray{}, dataArray->getDataType(), outStrm, *dataArray, mesgHandler, shouldCancel, delimiter, tuplesPerLine);
        }
      }
      auto* stringArray = dataStructure.getDataAs<StringArray>(dataPath);
      if(stringArray != nullptr)
      {
        PrintStringArray(outStrm, *stringArray, mesgHandler, shouldCancel, delimiter);
      }
      auto* neighborList = dataStructure.getDataAs<INeighborList>(dataPath);
      if(neighborList != nullptr)
      {
        if(exportToBinary)
        {
          throw std::runtime_error(
              fmt::format("{}({}): Function {}: Error. Cannot print a NeighborList to binary: '{}'", "PrintDataSetsToMultipleFiles", __FILE__, __LINE__, dataPath.getTargetName()));
        }
        ExecuteNeighborFunction(PrintNeighborList{}, neighborList->getDataType(), outStrm, neighborList, mesgHandler, shouldCancel, delimiter, includeIndex, includeHeaders);
      }
      if(result.invalid())
      {
        return result;
      }
    }
    if(shouldCancel)
    {
      return {};
    }
    Result<> commitResult = atomicFile.commit();
    if(commitResult.invalid())
    {
      return commitResult;
    }
  }

  return {};
}

void PrintSingleDataObject(std::ostream& outputStrm, const DataPath& objectPath, DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                           const std::string& delimiter, bool includeIndex, bool includeHeaders, size_t componentsPerLine)
{
  mesgHandler(IFilter::Message::Type::Info, fmt::format("Writing IArray ({}) to output stream", objectPath.getTargetName()));

  auto* dataArray = dataStructure.getDataAs<IDataArray>(objectPath);
  if(dataArray != nullptr)
  {
    ExecuteDataFunction(PrintDataArray{}, dataArray->getDataType(), outputStrm, *dataArray, mesgHandler, shouldCancel, delimiter, componentsPerLine);
  }
  auto* stringArray = dataStructure.getDataAs<StringArray>(objectPath);
  if(stringArray != nullptr)
  {
    PrintStringArray(outputStrm, *stringArray, mesgHandler, shouldCancel, delimiter);
  }
  auto* neighborList = dataStructure.getDataAs<INeighborList>(objectPath);
  if(neighborList != nullptr)
  {
    ExecuteNeighborFunction(PrintNeighborList{}, neighborList->getDataType(), outputStrm, neighborList, mesgHandler, shouldCancel, delimiter, includeIndex, includeHeaders);
  }
};

void PrintDataSetsToSingleFile(std::ostream& outputStrm, const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler,
                               const std::atomic_bool& shouldCancel, const std::string& delimiter, bool includeIndex, bool includeHeaders, bool writeFirstIndex, const std::string& indexName,
                               const std::vector<DataPath>& neighborLists, bool writeNumOfFeatures)
{
  const auto& firstDataArray = dataStructure.getDataRefAs<IArray>(objectPaths[0]);
  usize numTuples = firstDataArray.getNumberOfTuples();
  auto start = std::chrono::steady_clock::now();

  // Type-erased writers let one tuple loop interleave different array types.
  std::vector<std::shared_ptr<ITupleWriter>> writers;
  for(const auto& selectedArrayPath : objectPaths)
  {
    auto* dataArrayPtr = dataStructure.getDataAs<IDataArray>(selectedArrayPath);
    if(nullptr != dataArrayPtr)
    {
      const auto& iDataArrayRef = dataStructure.getDataRefAs<IDataArray>(selectedArrayPath);
      ExecuteDataFunction(AddTupleWriter{}, iDataArrayRef.getDataType(), writers, iDataArrayRef, delimiter);
    }
    auto* stringArrayPtr = dataStructure.getDataAs<StringArray>(selectedArrayPath);
    if(nullptr != stringArrayPtr)
    {
      const auto& iDataArrayRef = dataStructure.getDataRefAs<StringArray>(selectedArrayPath);
      writers.push_back(std::make_shared<StringTupleWriter>(iDataArrayRef, "'"));
    }
  }
  size_t writersCount = writers.size();

  if(shouldCancel)
  {
    return;
  }

  if(writeNumOfFeatures)
  {
    size_t featureCount = 0;

    featureCount += dataStructure.getDataRefAs<IArray>(objectPaths.at(0)).getNumberOfTuples();
    if(!writeFirstIndex)
    {
      featureCount--;
    }
    outputStrm << featureCount << "\n";
  }

  if(includeHeaders)
  {
    if(includeIndex)
    {
      outputStrm << indexName << delimiter;
    }
    for(size_t writerIndex = 0; writerIndex < writersCount; writerIndex++)
    {
      writers[writerIndex]->writeHeader(outputStrm);
      if(writerIndex != writersCount - 1)
      {
        outputStrm << delimiter;
      }
    }
    outputStrm << '\n';
  }

  if(shouldCancel)
  {
    return;
  }

  // Feature-oriented exports can omit background tuple zero.
  size_t writerIndexStart = 0;
  if(!writeFirstIndex)
  {
    writerIndexStart = 1;
  }
  for(usize tupleIndex = writerIndexStart; tupleIndex < numTuples; tupleIndex++)
  {
    auto now = std::chrono::steady_clock::now();
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
    {
      auto string = fmt::format("Printing tuples: {}% completed", static_cast<int32>(100 * static_cast<float>(tupleIndex) / static_cast<float>(numTuples)));
      mesgHandler(IFilter::Message::Type::Info, string);
      start = now;
      if(shouldCancel)
      {
        return;
      }
    }
    if(includeIndex)
    {
      outputStrm << tupleIndex << delimiter;
    }
    for(size_t writerIndex = 0; writerIndex < writersCount; writerIndex++)
    {
      writers[writerIndex]->write(outputStrm, tupleIndex);
      if(writerIndex != writersCount - 1)
      {
        outputStrm << delimiter;
      }
    }
    outputStrm << '\n';
  }

  if(!neighborLists.empty())
  {
    for(const auto& dataPath : neighborLists)
    {
      auto* neighborList = dataStructure.getDataAs<INeighborList>(dataPath);
      if(neighborList != nullptr)
      {
        ExecuteNeighborFunction(PrintNeighborList{}, neighborList->getDataType(), outputStrm, neighborList, mesgHandler, shouldCancel, delimiter, includeIndex, includeHeaders);
      }
      if(shouldCancel)
      {
        return;
      }
    }
  }
};
} // namespace nx::core::OStreamUtilities
