#include "OStreamUtilities.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <chrono>
#include <iomanip>
#include <ostream>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;

namespace // for nonmember functions
{
const std::array<std::string, 5> k_DelimiterStrings = {" ", ";", ",", ":", "\t"}; // Don't reorder

/**
 * @brief implicit writing of **NeighborList**'s elements to outputStrm
 * @tparam ScalarType The primitive type attacthed to **NeighborList**
 * @param outputStrm the ostream to write to
 * @param inputNeighborList The **NeighborList** that will have its values translated into strings
 * @param mesgHandler The message handler to dump progress updates to
 * // default parameters
 * @param delimiter The delimiter to insert between values
 * @param hasIndex bool to determine if index must be printed
 * @param hasHeaders bool to determine if headers must be printed
 */
struct PrintNeighborList
{
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
      outputStrm << "Element Count" << delimiter << inputNeighborList->getName() << "\n";
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
    else // no index
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
 * @brief implicit writing of **DataArray**'s elements to outputStrm
 * @tparam ScalarType The primitive type attacthed to **DataArray**
 * @param outputStrm the ostream to write to
 * @param inputDataArray The **DataArray** that will have its values translated into strings
 * @param mesgHandler The message handler to dump progress updates to
 * // default parameters
 * @param delimiter The delimiter to insert between values
 * @param componentsPerLine The number of components per line
 */
struct PrintDataArray
{
  template <typename ScalarType>
  Result<> operator()(std::ostream& outputStrm, IDataArray* inputDataArray, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const std::string& delimiter = ",",
                      int32 componentsPerLine = 0)
  {
    auto& dataArray = *dynamic_cast<DataArray<ScalarType>*>(inputDataArray);
    auto start = std::chrono::steady_clock::now();
    auto numTuples = dataArray.getNumberOfTuples();
    auto maxLine = static_cast<size_t>(componentsPerLine);
    if(componentsPerLine == 0)
    {
      maxLine = static_cast<size_t>(dataArray.getNumberOfComponents());
    }

    usize numComps = dataArray.getNumberOfComponents();
    for(size_t tuple = 0; tuple < numTuples; tuple++)
    {
      auto now = std::chrono::steady_clock::now();
      if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
      {
        auto string = fmt::format("Processing {}: {}% completed", dataArray.getName(), static_cast<int32>(100 * static_cast<float>(tuple) / static_cast<float>(numTuples)));
        mesgHandler(IFilter::Message::Type::Info, string);
        start = now;
        if(shouldCancel)
        {
          return {};
        }
      }
      for(size_t index = 0; index < numComps; index++)
      {
        if constexpr(std::is_same_v<ScalarType, int8> || std::is_same_v<ScalarType, uint8>)
        {
          outputStrm << static_cast<int32>(dataArray[tuple * numComps + index]);
        }
        else if constexpr(std::is_same_v<ScalarType, float32> || std::is_same_v<ScalarType, float64>)
        {
          outputStrm << fmt::format("{}", dataArray[tuple * numComps + index]);
        }
        else
        {
          outputStrm << dataArray[tuple * numComps + index];
        }
        if(index != maxLine - 1)
        {
          outputStrm << delimiter;
        }
        else
        {
          outputStrm << "\n";
        }
      }
    }
    return {};
  }
};

/**
 * @brief writing of **StringArray**'s elements to outputStrm
 * @param absoluteFilePath The output path to write to
 * @param inputStringArray The **StringArray** that will have its values translated into strings
 * @param mesgHandler The message handler to dump progress updates to
 * // default parameters
 * @param delimiter The delimiter to insert between values
 * @return A result object with any errors or warnings
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

class ITupleWriter
{
public:
  ITupleWriter() = default;
  virtual ~ITupleWriter() = default;
  virtual void write(std::ostream& outputStrm, usize tupleIndex) const = 0;
  virtual void writeHeader(std::ostream& outputStrm) const = 0;
};

class StringTupleWriter : public ITupleWriter
{
  using DataArrayType = StringArray;

public:
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

  void write(std::ostream& outputStrm, usize tupleIndex) const override
  {
    outputStrm << m_Delimiter << m_DataArray[tupleIndex] << m_Delimiter;
  }

  void writeHeader(std::ostream& outputStrm) const override
  {
    outputStrm << m_DataArray.getName();
  }

private:
  const DataArrayType& m_DataArray;
  const std::string m_Delimiter = "'";
};

template <typename ScalarType>
class TupleWriter : public ITupleWriter
{
  using DataArrayType = DataArray<ScalarType>;

public:
  TupleWriter(const IDataArray& iDataArray, const std::string& delimiter)
  : m_DataArray(dynamic_cast<const DataArray<ScalarType>&>(iDataArray))
  , m_Delimiter(delimiter)
  {
    m_NumComps = m_DataArray.getNumberOfComponents();
  }
  ~TupleWriter() override = default;

  void write(std::ostream& outputStrm, usize tupleIndex) const override
  {
    for(usize comp = 0; comp < m_NumComps; comp++)
    {
      if constexpr(std::is_same_v<ScalarType, int8> || std::is_same_v<ScalarType, uint8>)
      {
        outputStrm << static_cast<int32>(m_DataArray[tupleIndex * m_NumComps + comp]);
      }
      else if constexpr(std::is_same_v<ScalarType, float32>)
      {
        outputStrm << std::setprecision(8) << std::noshowpoint << m_DataArray[tupleIndex * m_NumComps + comp];
      }
      else if constexpr(std::is_same_v<ScalarType, float64>)
      {
        outputStrm << std::setprecision(16) << std::noshowpoint << m_DataArray[tupleIndex * m_NumComps + comp];
      }
      else
      {
        outputStrm << m_DataArray[tupleIndex * m_NumComps + comp];
      }
      if(comp < m_NumComps - 1)
      {
        outputStrm << m_Delimiter;
      }
    }
  }

  void writeHeader(std::ostream& outputStrm) const override
  {
    // If there is only 1 component then write the name of the array and return
    if(m_NumComps == 1)
    {
      outputStrm << m_DataArray.getName();
      return;
    }

    for(size_t index = 0; index < m_NumComps; index++)
    {
      outputStrm << m_DataArray.getName() << "_" << index;

      if(index < m_NumComps - 1)
      {
        outputStrm << m_Delimiter;
      }
    }
  }

private:
  const DataArrayType& m_DataArray;
  const std::string& m_Delimiter = ",";
  usize m_NumComps = 1;
};

struct AddTupleWriter
{
  template <typename ScalarType>
  Result<> operator()(std::vector<std::shared_ptr<ITupleWriter>>& writers, const IDataArray& iDataArray, const std::string& delimiter)
  {
    writers.push_back(std::make_shared<TupleWriter<ScalarType>>(iDataArray, delimiter));
    return {};
  }
};
} // namespace

namespace nx::core
{
namespace OStreamUtilities
{
/**
 * @brief turns the enum in this API to respective character as a string
 * @param delim the underlying value of the enum type
 */
std::string DelimiterToString(uint64 delim)
{
  return k_DelimiterStrings[delim];
};

/**
 * @brief [BINARY CAPABLE, unless neighborlist][Multiple File Output] | Writes out to multiple files | !!!!endianess must be addressed in calling class!!!!
 * @param objectPaths The vector of datapaths for respective dataObjects to be written out
 * @param dataStructure The simplnx datastructure where *objectPaths* datacontainers are stored
 * @param directoryPath The path to the directory to write files to | used to create outputStrm paths for ofstream
 * @param mesgHandler The handler to send progress updates to
 * @param shouldCancel The atomic boolean that determines cancel
 * //params with defaults
 * @param fileExtension The extension to create and write to files with
 * @param exportToBinary The boolean that determines if it writes out binary
 * @param delimiter The delimiter to be inserted into string | leave blank if binary is end output
 * @param includeIndex The boolean that determines if "Feature_IDs" are printed | leave blank if binary is end output
 * @param includeHeaders The boolean that determines if headers are printed | leave blank if binary is end output
 * @param componentsPerLine The amount of elements to be inserted before newline character | leave blank if binary is end output
 */
Result<> PrintDataSetsToMultipleFiles(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, const std::string& directoryPath, const IFilter::MessageHandler& mesgHandler,
                                      const std::atomic_bool& shouldCancel, const std::string& fileExtension, bool exportToBinary, const std::string& delimiter, bool includeIndex, bool includeHeaders,
                                      size_t componentsPerLine)
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

    // Scope file writer in code block to get around file lock on windows (enforce destructor order)
    {
      std::ofstream outStrm(outputFilePath, std::ios_base::out | std::ios_base::binary);

      std::pair<int32, std::string> result = {0, "PrintDataSetsToMultipleFiles default failure. If you are seeing this error something bad has happened."};
      auto* dataArray = dataStructure.getDataAs<IDataArray>(dataPath);
      if(dataArray != nullptr)
      {
        if(exportToBinary)
        {
          result = dataArray->getIDataStore()->writeBinaryFile(outputFilePath);
        }
        else
        {
          ExecuteDataFunction(PrintDataArray{}, dataArray->getDataType(), outStrm, dataArray, mesgHandler, shouldCancel, delimiter, componentsPerLine);
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
      if(result.first < 0)
      {
        mesgHandler(IFilter::Message::Type::Error, result.second);
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

/**
 * @brief [Single Output][Custom OStream] | Writes one IArray child to some OStream
 * @param outputStrm The already opened output string to write to
 * @param objectPath The datapath for respective dataObject to be written out
 * @param dataStructure The simplnx datastructure where *objectPath* datacontainer is stored
 * //params with defaults
 * @param delimiter The delimiter to be inserted into string
 * @param includeIndex The boolean that determines if "Feature_IDs" are printed
 * @param includeHeaders The boolean that determines if headers are printed
 * @param componentsPerLine The amount of elements to be inserted before newline character
 */
void PrintSingleDataObject(std::ostream& outputStrm, const DataPath& objectPath, DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                           const std::string& delimiter, bool includeIndex, bool includeHeaders, size_t componentsPerLine)
{
  mesgHandler(IFilter::Message::Type::Info, fmt::format("Writing IArray ({}) to output stream", objectPath.getTargetName()));

  auto* dataArray = dataStructure.getDataAs<IDataArray>(objectPath);
  if(dataArray != nullptr)
  {
    ExecuteDataFunction(PrintDataArray{}, dataArray->getDataType(), outputStrm, dataArray, mesgHandler, shouldCancel, delimiter, componentsPerLine);
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

/**
 * @brief [Single Output][Custom OStream] | writes out multiple arrays to ostream
 * @param outputStrm The already opened output string to write to
 * @param objectPaths The vector of datapaths for respective dataObjects to be written out
 * @param dataStructure The simplnx datastructure where *objectPaths* datacontainers are stored
 * @param mesgHandler The handler to send progress updates to
 * @param shouldCancel The atomic boolean that determines cancel
 * //params with defaults
 * @param delimiter The delimiter to be inserted into string
 * @param includeIndex The boolean that determines if "Feature_IDs" are printed
 * @param includeHeaders The boolean that determines if headers are printed
 * @param componentsPerLine The amount of elements to be inserted before newline character
 * @param neighborLists The list of dataPaths of neighborlists to include
 */
void PrintDataSetsToSingleFile(std::ostream& outputStrm, const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler,
                               const std::atomic_bool& shouldCancel, const std::string& delimiter, bool includeIndex, bool includeHeaders, bool writeFirstIndex, const std::string& indexName,
                               const std::vector<DataPath>& neighborLists, bool writeNumOfFeatures)
{
  const auto& firstDataArray = dataStructure.getDataRefAs<IArray>(objectPaths[0]);
  usize numTuples = firstDataArray.getNumberOfTuples();
  auto start = std::chrono::steady_clock::now();

  // Create our wrapper classes for each DataArray
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

  // Write out the header line
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

  // Loop on ever tuple using our predefined writer for each data array
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
} // namespace OStreamUtilities
} // namespace nx::core
